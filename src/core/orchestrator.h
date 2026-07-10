/**
 * @file orchestrator.h
 * @brief 总控协调器 —— Multi-Agent 系统的"大脑"
 *
 * @details
 * Orchestrator 是连接 LLM 规划层、C++ 调度层和 UI 展示层的枢纽组件。
 * 它实现了"两阶段执行模式"：
 *
 * 阶段一 —— 规划（Planning）：
 *   用户输入 → PromptBuilder 构造规划 Prompt → LLMClient 发送请求
 *   → 接收 JSON 响应 → Planner 解析为 TaskPlan → 发射 planGenerated 信号
 *
 * 阶段二 —— 执行（Execution）：
 *   TaskPlan → TaskScheduler.setPlan() → TaskScheduler.startExecution()
 *   → 逐个步骤调度 Agent → 转发所有信号到 UI → 计划完成后生成总结
 *
 * Orchestrator 本身不直接执行任何工具操作，所有执行都委托给 TaskScheduler。
 * 它的核心价值在于协调各组件的交互时序和信号转发。
 *
 * 与 v1.0 的区别：
 * - v1.0：Orchestrator 直接解析 LLM 的文本响应，提取工具调用格式（如 <tool(args)>）
 * - v2.0：Orchestrator 通过 Planner 解析 JSON 计划，交给 TaskScheduler 调度执行
 *         大模型不直接调用工具，而是生成结构化计划，由 C++ 层全权执行
 */

#ifndef ORCHESTRATOR_H  // 【条件编译】防止头文件被重复包含，这是C/C++的标准做法
#define ORCHESTRATOR_H  // 【宏定义】定义ORCHESTRATOR_H宏，标记该文件已被包含

#include "agent.h"              // 【引入】Agent基类头文件，Orchestrator继承自Agent
#include "llmclient.h"          // 【引入】LLM客户端头文件，用于与大模型通信
#include "promptbuilder.h"      // 【引入】Prompt构建器头文件，用于构造发送给大模型的提示词
#include "planner.h"            // 【引入】计划解析器头文件，用于解析大模型返回的JSON计划
#include "taskscheduler.h"      // 【引入】任务调度器头文件，用于执行解析后的计划
#include "task.h"               // 【引入】任务数据结构头文件，定义TaskPlan和TaskStep
#include "conversationmanager.h" // 【引入】对话管理器头文件，用于管理多轮对话历史
#include <QList>                 // 【引入】Qt列表容器类
#include <QString>               // 【引入】Qt字符串类

/**
 * @brief 总控协调器
 *
 * Orchestrator 继承自 Agent 基类，这意味着它本身也可以被视为一个特殊的 Agent。
 * 但在 v2.0 架构中，Orchestrator 的角色已经从"直接执行工具"转变为"协调调度"，
 * 其 Agent 接口的实现对实际调度逻辑影响有限。
 *
 * 信号设计：
 * - planGenerated / stepStarted / stepOutput / stepCompleted / stepFailed / planCompleted
 *   这些信号从 TaskScheduler 透传而来，供 UI 层订阅
 * - messageReceived / errorOccurred
 *   这些信号用于普通对话模式和错误报告
 */
class Orchestrator : public Agent  // 【类声明】Orchestrator继承自Agent，使其成为Qt对象并可发送信号
{
    Q_OBJECT  // 【Qt宏】启用Qt的元对象系统，支持信号槽、反射等特性，必须放在类声明中

public:
    /**
     * @brief 构造函数
     * @param parent 父 QObject
     *
     * 初始化所有成员指针为 nullptr，创建 TaskScheduler 子对象，
     * 并连接 TaskScheduler 的所有信号到 Orchestrator 的对应槽函数。
     */
    explicit Orchestrator(QObject *parent = nullptr);  // 【构造函数】explicit防止隐式类型转换，parent用于Qt对象树内存管理

    /**
     * @brief 析构函数
     */
    ~Orchestrator();  // 【析构函数】释放资源，m_scheduler由Qt父对象机制自动销毁

    // Agent 基类纯虚接口的实现
    QString name() const override;                              // 【重写】返回Agent名称"Orchestrator"
    QString description() const override;                       // 【重写】返回Agent描述
    AgentType type() const override;                            // 【重写】返回Agent类型为总控类型
    QList<Tool*> tools() const override;                        // 【重写】汇总所有子Agent的工具列表
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;  // 【重写】委托给对应子Agent执行工具

    /**
     * @brief 设置 LLM 客户端
     * @param client LlmClient 实例指针
     *
     * @details
     * 设置 LLM 客户端并连接其信号：
     * - responseReceived → onLlmResponse（处理 LLM 返回的响应）
     * - responseError → onLlmError（处理网络/API 错误）
     *
     * @note Orchestrator 不拥有 LlmClient 的所有权，不 delete。
     */
    void setLlmClient(LlmClient* client);  // 【方法】设置与大模型通信的客户端，外部管理其生命周期

    /**
     * @brief 添加一个子 Agent 到系统
     * @param agent 要添加的 Agent 实例
     *
     * @details
     * 同时做两件事：
     * 1. 将 Agent 添加到 Orchestrator 的 m_agents 列表（用于 tools() 汇总）
     * 2. 将 Agent 注册到 TaskScheduler（用于执行时路由）
     */
    void addAgent(Agent* agent);  // 【方法】注册子Agent，扩展系统能力

    /**
     * @brief 处理用户查询 —— 主入口函数
     * @param query 用户的自然语言输入
     *
     * @details
     * 触发两阶段执行模式的阶段一（规划）：
     * 1. 检查前置条件（LLM 客户端已设置、当前未在处理中）
     * 2. 标记 m_isProcessing = true，m_isPlanningPhase = true
     * 3. 保存用户输入到 m_currentQuery
     * 4. 调用 sendPlanningRequest() 向 LLM 发送规划请求
     *
     * @note 如果当前正在处理其他任务，此方法会直接返回，防止并发请求。
     */
    void processQuery(const QString& query);                      // 【方法】处理不带文件的用户查询
    void processQueryWithFiles(const QString& query, const QStringList& files);  // 【方法】处理带文件附件的用户查询

    /**
     * @brief 停止当前执行
     *
     * @details
     * 如果 TaskScheduler 正在运行，调用其 stopExecution()。
     * 同时重置 Orchestrator 的处理状态标志。
     */
    void stopExecution();  // 【方法】用户点击停止按钮时调用，中断后续执行

    /**
     * @brief 判断当前是否正在处理请求
     * @return true 当有请求正在处理中（规划或执行阶段）
     */
    bool isProcessing() const;  // 【方法】供UI层查询当前状态，用于禁用/启用按钮

    /**
     * @brief 获取当前执行计划的副本
     * @return 当前 TaskPlan 的副本
     *
     * @note 供 UI 层查询当前计划的详细信息（如步骤描述）。
     */
    TaskPlan currentPlan() const;  // 【方法】返回当前计划的副本，避免外部直接修改内部状态

    QJsonArray conversationHistory() const;      // 【方法】获取当前对话历史的JSON数组
    void loadConversationHistory(const QJsonArray& history);  // 【方法】从JSON加载对话历史
    void switchConversation(int index);          // 【方法】切换到指定索引的对话
    int currentConversationIndex() const;        // 【方法】获取当前对话的索引
    void createNewConversation();                // 【方法】创建新对话
    void deleteConversation(int index);          // 【方法】删除指定索引的对话

signals:  // 【Qt关键字】以下声明的是信号，用于通知UI层状态变化
    void responseChunkReceived(const QString& chunk);  // 【信号】收到LLM的流式响应片段

    /**
     * @brief 实时日志输出
     * @param logType 日志类型: "llm" | "agent" | "system" | "execution"
     * @param message 日志内容
     *
     * 用于实时显示执行过程中的关键信息：
     * - llm: LLM 返回的原始响应
     * - agent: Agent 调用详情
     * - system: 系统状态变化
     * - execution: 执行进度
     */
    void logMessage(const QString& logType, const QString& message);  // 【信号】发送日志消息到UI的日志面板

    /**
     * @brief 计划生成完毕
     * @param plan 解析后的 TaskPlan
     *
     * 在阶段一（规划）完成后发射，UI 可据此初始化任务列表。
     */
    void planGenerated(const TaskPlan& plan);  // 【信号】通知UI计划已生成，可以展示步骤列表

    /**
     * @brief 某个步骤开始执行
     * @param stepId 步骤编号
     *
     * 从 TaskScheduler 透传，供 UI 更新步骤状态图标。
     */
    void stepStarted(int stepId);  // 【信号】通知UI某个步骤开始执行，更新状态为"运行中"

    /**
     * @brief 某个步骤产生输出
     * @param stepId 步骤编号
     * @param output 输出文本
     *
     * 从 TaskScheduler 透传，供 UI 输出面板追加日志。
     */
    void stepOutput(int stepId, const QString& output);  // 【信号】通知UI在输出面板追加该步骤的输出内容

    /**
     * @brief 某个步骤成功完成
     * @param stepId 步骤编号
     * @param result 执行结果
     *
     * 从 TaskScheduler 透传，供 UI 更新步骤状态为"成功"。
     */
    void stepCompleted(int stepId, const QVariantMap& result);  // 【信号】通知UI步骤执行成功，更新状态为"完成"

    /**
     * @brief 某个步骤执行失败
     * @param stepId 步骤编号
     * @param error 错误信息
     *
     * 从 TaskScheduler 透传，供 UI 更新步骤状态为"失败"。
     */
    void stepFailed(int stepId, const QString& error);  // 【信号】通知UI步骤执行失败，展示错误信息

    /**
     * @brief 整个计划执行完成
     * @param plan 包含最终状态的 TaskPlan
     *
     * 从 TaskScheduler 透传，供 UI 显示完成总结。
     */
    void planCompleted(const TaskPlan& plan);  // 【信号】通知UI所有步骤已执行完毕

    /**
     * @brief 收到 AI 的普通消息（非计划模式）
     * @param message AI 的文本回复
     *
     * 当 LLM 返回纯文本回答（不是 JSON 计划）时发射。
     * 也用于计划执行完毕后的总结消息。
     */
    void messageReceived(const QString& message);  // 【信号】通知UI显示AI的普通文本回复

    /**
     * @brief 发生错误
     * @param error 错误描述
     *
     * LLM 网络错误、API 错误等异常情况时发射。
     */
    void errorOccurred(const QString& error);  // 【信号】通知UI显示错误提示

private slots:  // 【Qt关键字】以下声明的是私有槽函数，用于接收信号并处理
    void onLlmResponseChunk(const QString& chunk);  // 【槽】处理LLM流式响应片段

    /**
     * @brief LLM 响应到达的处理槽
     * @param response LLM 返回的文本
     *
     * @details
     * 根据当前所处阶段采取不同处理：
     *
     * 规划阶段（m_isPlanningPhase == true）：
     *   1. 调用 Planner::parsePlan() 解析 JSON
     *   2. 如果 steps 为空（LLM 返回了纯文本回答）：
     *      - 发射 messageReceived（降级为普通对话）
     *      - 重置处理状态
     *   3. 如果 steps 非空（成功生成计划）：
     *      - 设置 plan.goal = 用户原始输入
     *      - 发射 planGenerated
     *      - 将计划交给 TaskScheduler 并开始执行
     *      - 标记规划阶段结束
     *
     * 非规划阶段：
     *   - 直接发射 messageReceived（用于计划执行后的总结对话）
     */
    void onLlmResponse(const QString& response);  // 【槽】处理LLM完整响应，是核心调度逻辑

    /**
     * @brief LLM 请求出错的处理槽
     * @param error 错误信息
     *
     * 发射 errorOccurred 信号并重置所有处理状态标志。
     */
    void onLlmError(const QString& error);  // 【槽】处理LLM请求出错，清理状态

    // TaskScheduler 信号的透传槽函数
    // 这些槽函数不做任何处理，直接将信号转发到 Orchestrator 的同名信号
    // 实现了 TaskScheduler → Orchestrator → UI 的信号代理链

    /**
     * @brief 透传：步骤开始
     * @param stepId 步骤编号
     */
    void onSchedulerStepStarted(int stepId);  // 【槽】接收TaskScheduler的步骤开始信号并透传给UI

    /**
     * @brief 透传：步骤输出
     * @param stepId 步骤编号
     * @param output 输出文本
     */
    void onSchedulerStepOutput(int stepId, const QString& output);  // 【槽】接收步骤输出并透传

    /**
     * @brief 透传：步骤完成
     * @param stepId 步骤编号
     * @param result 执行结果
     */
    void onSchedulerStepCompleted(int stepId, const QVariantMap& result);  // 【槽】接收步骤完成信号并透传

    /**
     * @brief 透传：步骤失败
     * @param stepId 步骤编号
     * @param error 错误信息
     */
    void onSchedulerStepFailed(int stepId, const QString& error);  // 【槽】接收步骤失败信号并透传

    /**
     * @brief 计划执行完毕的处理
     * @param plan 包含最终状态的 TaskPlan
     *
     * @details
     * 1. 透传 planCompleted 信号到 UI
     * 2. 调用 generateSummary() 生成执行统计总结
     * 3. 重置 m_isProcessing 标志
     */
    void onSchedulerPlanCompleted(const TaskPlan& plan);  // 【槽】计划全部执行完毕后的处理

    /**
     * @brief 执行被用户停止的处理
     */
    void onSchedulerExecutionStopped();  // 【槽】用户手动停止执行后的清理

private:  // 【访问修饰符】以下成员为私有，仅类内部可访问
    void sendPlanningRequest(const QString& query);           // 【私有方法】向LLM发送规划请求
    void retryPlanWithFeedback(const TaskPlan& failedPlan);   // 【私有方法】计划全部失败时进行反思重试
    void generateSummary(const TaskPlan& plan);               // 【私有方法】生成执行总结报告

    LlmClient* m_llmClient;       // 【成员变量】LLM 客户端指针（外部管理生命周期）
    QList<Agent*> m_agents;       // 【成员变量】所有子 Agent 列表，用于工具汇总和执行路由
    PromptBuilder m_promptBuilder; // 【成员变量】Prompt 构建器（构造规划/普通对话 Prompt）
    Planner m_planner;            // 【成员变量】计划解析器（JSON → TaskPlan）
    TaskScheduler* m_scheduler;   // 【成员变量】任务调度引擎（核心执行器），是QObject子对象
    ConversationManager* m_conversationManager; // 【成员变量】对话历史管理器，支持多轮对话

    QString m_currentQuery;       // 【成员变量】当前正在处理的用户输入（用于填充 plan.goal）
    bool m_isProcessing;          // 【成员变量】是否正在处理请求（防止并发）
    bool m_isPlanningPhase;       // 【成员变量】当前是否处于规划阶段（影响 onLlmResponse 的处理逻辑）
    int m_retryCount;             // 【成员变量】反思重试次数计数器
    static const int MAX_RETRY = 1; // 【静态常量】最多重试次数，防止无限循环

    QString m_lastWrittenFilePath; // 【成员变量】最后写入的文件路径（用于内容补充检测）
    QString m_lastWrittenContent;  // 【成员变量】最后写入的文件内容摘要（用于判断是否需要补充）

    bool isContentGenerationTask(const QString& query) const;  // 【私有方法】判断是否是内容生成类任务
    bool isOutlineOnlyContent(const QString& content) const;   // 【私有方法】判断内容是否只有大纲
    void supplementContentForFile(const QString& filePath, const QString& originalContent, const QString& userQuery);  // 【私有方法】为文件补充完整内容

    /**
     * @brief 任务类型枚举
     *
     * 覆盖所有可能的用户需求场景，确保"来者不拒"：
     *
     * 【文件处理类】- 读取→LLM处理→写入模式
     * - DocumentExpansion: 文档扩写、丰富内容
     * - DocumentModification: 文档修改、润色、翻译、格式转换
     * - CodeAddition: 代码添加新功能
     * - CodeFix: 代码修复bug
     * - CodeRefactor: 代码重构、优化
     * - ConfigUpdate: 配置更新
     * - GenericEdit: 通用文件编辑
     *
     * 【代码开发类】- 需要编译/运行验证
     * - CodeGeneration: 从零生成代码文件
     * - ProjectCreation: 创建新项目结构
     * - CodeCompilation: 编译代码
     * - CodeExecution: 运行程序并调试
     * - CodeAnalysis: 代码分析、审查
     *
     * 【文本处理类】- 轻量级文本操作
     * - TextTransformation: 文本转换（编码、格式、翻译）
     * - DataExtraction: 从文本中提取信息
     * - TextAnalysis: 文本分析（统计、情感、关键词）
     *
     * 【系统操作类】- 需要执行系统命令
     * - SystemInfo: 获取系统信息
     * - ProcessManagement: 进程管理
     * - EnvironmentSetup: 环境配置、依赖安装
     * - FileSystemOperation: 文件系统操作（复制、移动、批量处理）
     *
     * 【搜索查询类】- 需要搜索或查找
     * - LocalFileSearch: 本地文件搜索
     * - CodeSearch: 代码搜索
     * - TextSearch: 文本内容搜索
     * - KnowledgeQuery: 知识问答（纯文本）
     */
    enum class TaskType {  // 【枚举类】C++11强类型枚举，避免命名冲突
        None,  // 【枚举值】无类型/未识别

        // 文件处理类
        DocumentExpansion,   // 【枚举值】文档扩写：丰富、扩充、完善已有文档
        DocumentModification, // 【枚举值】文档修改：润色、翻译、改写、重写
        CodeAddition,        // 【枚举值】代码添加：在现有代码中添加新功能
        CodeFix,             // 【枚举值】代码修复：修复bug、debug
        CodeRefactor,        // 【枚举值】代码重构：优化、改进代码结构
        ConfigUpdate,        // 【枚举值】配置更新：修改配置文件
        GenericEdit,         // 【枚举值】通用编辑：宽泛的文件编辑请求

        // 代码开发类
        CodeGeneration,      // 【枚举值】代码生成：从零生成新代码文件
        ProjectCreation,     // 【枚举值】项目创建：创建完整项目结构
        CodeCompilation,     // 【枚举值】代码编译：编译、构建项目
        CodeExecution,       // 【枚举值】代码执行：运行程序、调试
        CodeAnalysis,        // 【枚举值】代码分析：审查、统计、质量分析

        // 文本处理类
        TextTransformation,  // 【枚举值】文本转换：编码、格式转换、加密解密
        DataExtraction,      // 【枚举值】数据提取：从文本中解析信息
        TextAnalysis,        // 【枚举值】文本分析：统计、情感、关键词

        // 系统操作类
        SystemInfo,          // 【枚举值】系统信息：获取硬件、系统配置
        ProcessManagement,   // 【枚举值】进程管理：查看、结束进程
        EnvironmentSetup,    // 【枚举值】环境配置：安装依赖、配置环境变量
        FileSystemOperation, // 【枚举值】文件系统操作：复制、移动、批量处理

        // 搜索查询类
        LocalFileSearch,     // 【枚举值】本地文件搜索：查找本地文件
        CodeSearch,          // 【枚举值】代码搜索：查找函数、类定义
        TextSearch,          // 【枚举值】文本搜索：在文件中搜索内容
        KnowledgeQuery,      // 【枚举值】知识问答：纯文本问答，不需要工具

        // 聊天类
        Chat                 // 【枚举值】普通聊天：问候、闲聊、简单问答
    };

    /**
     * @brief 判断任务类型是否属于文件处理任务（需要读取→LLM处理→写入模式）
     * @param type 任务类型
     * @return true 如果是文件处理任务
     */
    bool isFileProcessingTask(TaskType type) const;  // 【私有方法】判断是否需要走文件处理流程

    /**
     * @brief 检测用户查询对应的任务类型
     * @param query 用户输入
     * @param files 用户上传的文件列表（可能为空）
     * @return 任务类型
     */
    TaskType detectTaskType(const QString& query, const QStringList& files) const;  // 【私有方法】通过关键词匹配识别用户意图

    /**
     * @brief 处理文件处理任务（读取→LLM处理→写入模式）
     * @param query 用户原始请求
     * @param files 用户上传的文件列表
     * @param type 任务类型
     */
    void handleFileProcessingTask(const QString& query, const QStringList& files, TaskType type);  // 【私有方法】执行文件处理工作流

    /**
     * @brief 构建文件处理任务的System Prompt
     * @param type 任务类型
     * @return 对应类型的System Prompt
     */
    QString buildFileProcessingSystemPrompt(TaskType type) const;  // 【私有方法】根据任务类型生成系统提示词

    /**
     * @brief 构建文件处理任务的User Prompt
     * @param type 任务类型
     * @param query 用户原始请求
     * @param originalContent 原始文件内容
     * @param fileName 文件名
     * @return 对应类型的User Prompt
     */
    QString buildFileProcessingUserPrompt(TaskType type, const QString& query, const QString& originalContent, const QString& fileName) const;  // 【私有方法】构建用户提示词
};

#endif // ORCHESTRATOR_H  // 【条件编译结束】与开头的#ifndef配对，结束头文件内容
