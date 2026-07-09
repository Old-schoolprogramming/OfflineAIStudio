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

#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "agent.h"
#include "llmclient.h"
#include "promptbuilder.h"
#include "planner.h"
#include "taskscheduler.h"
#include "task.h"
#include "conversationmanager.h"
#include <QList>
#include <QString>

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
class Orchestrator : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父 QObject
     *
     * 初始化所有成员指针为 nullptr，创建 TaskScheduler 子对象，
     * 并连接 TaskScheduler 的所有信号到 Orchestrator 的对应槽函数。
     */
    explicit Orchestrator(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Orchestrator();

    // Agent 基类纯虚接口的实现
    QString name() const override;
    QString description() const override;
    AgentType type() const override;
    QList<Tool*> tools() const override;
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

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
    void setLlmClient(LlmClient* client);

    /**
     * @brief 添加一个子 Agent 到系统
     * @param agent 要添加的 Agent 实例
     *
     * @details
     * 同时做两件事：
     * 1. 将 Agent 添加到 Orchestrator 的 m_agents 列表（用于 tools() 汇总）
     * 2. 将 Agent 注册到 TaskScheduler（用于执行时路由）
     */
    void addAgent(Agent* agent);

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
    void processQuery(const QString& query);

    /**
     * @brief 停止当前执行
     *
     * @details
     * 如果 TaskScheduler 正在运行，调用其 stopExecution()。
     * 同时重置 Orchestrator 的处理状态标志。
     */
    void stopExecution();

    /**
     * @brief 判断当前是否正在处理请求
     * @return true 当有请求正在处理中（规划或执行阶段）
     */
    bool isProcessing() const;

    /**
     * @brief 获取当前执行计划的副本
     * @return 当前 TaskPlan 的副本
     *
     * @note 供 UI 层查询当前计划的详细信息（如步骤描述）。
     */
    TaskPlan currentPlan() const;

    QJsonArray conversationHistory() const;
    void loadConversationHistory(const QJsonArray& history);
    void switchConversation(int index);
    int currentConversationIndex() const;
    void createNewConversation();

signals:
    void responseChunkReceived(const QString& chunk);

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
    void logMessage(const QString& logType, const QString& message);

    /**
     * @brief 计划生成完毕
     * @param plan 解析后的 TaskPlan
     *
     * 在阶段一（规划）完成后发射，UI 可据此初始化任务列表。
     */
    void planGenerated(const TaskPlan& plan);

    /**
     * @brief 某个步骤开始执行
     * @param stepId 步骤编号
     *
     * 从 TaskScheduler 透传，供 UI 更新步骤状态图标。
     */
    void stepStarted(int stepId);

    /**
     * @brief 某个步骤产生输出
     * @param stepId 步骤编号
     * @param output 输出文本
     *
     * 从 TaskScheduler 透传，供 UI 输出面板追加日志。
     */
    void stepOutput(int stepId, const QString& output);

    /**
     * @brief 某个步骤成功完成
     * @param stepId 步骤编号
     * @param result 执行结果
     *
     * 从 TaskScheduler 透传，供 UI 更新步骤状态为"成功"。
     */
    void stepCompleted(int stepId, const QVariantMap& result);

    /**
     * @brief 某个步骤执行失败
     * @param stepId 步骤编号
     * @param error 错误信息
     *
     * 从 TaskScheduler 透传，供 UI 更新步骤状态为"失败"。
     */
    void stepFailed(int stepId, const QString& error);

    /**
     * @brief 整个计划执行完成
     * @param plan 包含最终状态的 TaskPlan
     *
     * 从 TaskScheduler 透传，供 UI 显示完成总结。
     */
    void planCompleted(const TaskPlan& plan);

    /**
     * @brief 收到 AI 的普通消息（非计划模式）
     * @param message AI 的文本回复
     *
     * 当 LLM 返回纯文本回答（不是 JSON 计划）时发射。
     * 也用于计划执行完毕后的总结消息。
     */
    void messageReceived(const QString& message);

    /**
     * @brief 发生错误
     * @param error 错误描述
     *
     * LLM 网络错误、API 错误等异常情况时发射。
     */
    void errorOccurred(const QString& error);

private slots:
    void onLlmResponseChunk(const QString& chunk);

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
    void onLlmResponse(const QString& response);

    /**
     * @brief LLM 请求出错的处理槽
     * @param error 错误信息
     *
     * 发射 errorOccurred 信号并重置所有处理状态标志。
     */
    void onLlmError(const QString& error);

    // TaskScheduler 信号的透传槽函数
    // 这些槽函数不做任何处理，直接将信号转发到 Orchestrator 的同名信号
    // 实现了 TaskScheduler → Orchestrator → UI 的信号代理链

    /**
     * @brief 透传：步骤开始
     * @param stepId 步骤编号
     */
    void onSchedulerStepStarted(int stepId);

    /**
     * @brief 透传：步骤输出
     * @param stepId 步骤编号
     * @param output 输出文本
     */
    void onSchedulerStepOutput(int stepId, const QString& output);

    /**
     * @brief 透传：步骤完成
     * @param stepId 步骤编号
     * @param result 执行结果
     */
    void onSchedulerStepCompleted(int stepId, const QVariantMap& result);

    /**
     * @brief 透传：步骤失败
     * @param stepId 步骤编号
     * @param error 错误信息
     */
    void onSchedulerStepFailed(int stepId, const QString& error);

    /**
     * @brief 计划执行完毕的处理
     * @param plan 包含最终状态的 TaskPlan
     *
     * @details
     * 1. 透传 planCompleted 信号到 UI
     * 2. 调用 generateSummary() 生成执行统计总结
     * 3. 重置 m_isProcessing 标志
     */
    void onSchedulerPlanCompleted(const TaskPlan& plan);

    /**
     * @brief 执行被用户停止的处理
     */
    void onSchedulerExecutionStopped();

private:
    void sendPlanningRequest(const QString& query);
    void retryPlanWithFeedback(const TaskPlan& failedPlan);
    void generateSummary(const TaskPlan& plan);

    LlmClient* m_llmClient;       ///< LLM 客户端指针（外部管理生命周期）
    QList<Agent*> m_agents;       ///< 所有子 Agent 列表
    PromptBuilder m_promptBuilder; ///< Prompt 构建器（构造规划/普通对话 Prompt）
    Planner m_planner;            ///< 计划解析器（JSON → TaskPlan）
    TaskScheduler* m_scheduler;   ///< 任务调度引擎（核心执行器）
    ConversationManager* m_conversationManager; ///< 对话历史管理器

    QString m_currentQuery;       ///< 当前正在处理的用户输入（用于填充 plan.goal）
    bool m_isProcessing;          ///< 是否正在处理请求（防止并发）
    bool m_isPlanningPhase;       ///< 当前是否处于规划阶段（影响 onLlmResponse 的处理逻辑）
    int m_retryCount;             ///< 反思重试次数
    static const int MAX_RETRY = 1; ///< 最多重试次数
};

#endif // ORCHESTRATOR_H
