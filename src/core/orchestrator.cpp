/**
 * @file orchestrator.cpp
 * @brief 总控协调器实现
 *
 * @details
 * Orchestrator 实现了两阶段执行模式的完整流程：
 *
 * 阶段一（规划）：
 *   processQuery() → sendPlanningRequest() → LlmClient::sendPrompt()
 *   → 等待 LLM 响应 → onLlmResponse() → Planner::parsePlan()
 *   → 发射 planGenerated → TaskScheduler::setPlan() → 开始执行
 *
 * 阶段二（执行）：
 *   TaskScheduler::executeNextStep() → Agent::executeTool()
 *   → 发射 stepStarted/stepOutput/stepCompleted/stepFailed
 *   → Orchestrator 透传到 UI → 全部完成 → onSchedulerPlanCompleted()
 *   → generateSummary() → 发射 messageReceived
 *
 * 信号代理链：
 *   TaskScheduler 信号 → Orchestrator 槽 → Orchestrator 同名信号 → MainWindow 槽
 *   这种设计使 TaskScheduler 与 UI 完全解耦，Orchestrator 作为中间层统一管控。
 */

#include "orchestrator.h"  // 【引入】自己的头文件
#include "agent.h"         // 【引入】Agent基类头文件
#include "tool.h"          // 【引入】工具基类头文件
#include <QDebug>          // 【引入】Qt调试输出类
#include <QFileInfo>       // 【引入】Qt文件信息类，用于获取文件名、大小等
#include <QFile>           // 【引入】Qt文件操作类
#include <QTextStream>     // 【引入】Qt文本流类，用于读写文本文件
#include <QJsonObject>     // 【引入】Qt JSON对象类
#include <QJsonArray>      // 【引入】Qt JSON数组类

/**
 * @brief 构造函数
 * @param parent 父 QObject
 *
 * @implementation
 * 1. 初始化所有指针和状态标志
 * 2. 创建 TaskScheduler 子对象（ parenting 到 this，自动内存管理）
 * 3. 连接 TaskScheduler 的 6 个信号到 Orchestrator 的 6 个透传槽
 *    建立完整的信号代理链
 */
Orchestrator::Orchestrator(QObject *parent)
    : Agent(parent),               // 【初始化】调用父类Agent的构造函数，传入parent建立Qt对象树
      m_llmClient(nullptr),        // 【初始化】LLM客户端指针初始设为空，后续通过setLlmClient设置
      m_scheduler(new TaskScheduler(this)),  // 【初始化】创建任务调度器，以this为父对象，Qt会自动管理内存
      m_conversationManager(new ConversationManager(this)),  // 【初始化】创建对话管理器，同样以this为父对象
      m_isProcessing(false),       // 【初始化】初始状态：没有在处理请求
      m_isPlanningPhase(false),    // 【初始化】初始状态：不在规划阶段
      m_retryCount(0)              // 【初始化】重试计数器归零
{
    // 【信号连接】将TaskScheduler的步骤开始信号连接到本类的透传槽
    connect(m_scheduler, &TaskScheduler::stepStarted,
            this, &Orchestrator::onSchedulerStepStarted);
    // 【信号连接】将TaskScheduler的步骤输出信号连接到本类的透传槽
    connect(m_scheduler, &TaskScheduler::stepOutput,
            this, &Orchestrator::onSchedulerStepOutput);
    // 【信号连接】将TaskScheduler的步骤完成信号连接到本类的透传槽
    connect(m_scheduler, &TaskScheduler::stepCompleted,
            this, &Orchestrator::onSchedulerStepCompleted);
    // 【信号连接】将TaskScheduler的步骤失败信号连接到本类的透传槽
    connect(m_scheduler, &TaskScheduler::stepFailed,
            this, &Orchestrator::onSchedulerStepFailed);
    // 【信号连接】将TaskScheduler的计划完成信号连接到本类的处理槽
    connect(m_scheduler, &TaskScheduler::planCompleted,
            this, &Orchestrator::onSchedulerPlanCompleted);
    // 【信号连接】将TaskScheduler的执行停止信号连接到本类的处理槽
    connect(m_scheduler, &TaskScheduler::executionStopped,
            this, &Orchestrator::onSchedulerExecutionStopped);
}

/**
 * @brief 析构函数
 *
 * @note m_scheduler 是 this 的子对象，由 Qt 父对象机制自动析构。
 *       m_llmClient 由外部（MainWindow）管理，不在这里 delete。
 */
Orchestrator::~Orchestrator()
{
    // 【说明】由于m_scheduler和m_conversationManager的父对象都是this，
    //         Qt会在this被销毁时自动销毁它们，无需手动delete
}

/**
 * @brief 获取 Agent 名称
 * @return "Orchestrator"
 */
QString Orchestrator::name() const
{
    return "Orchestrator";  // 【返回】固定字符串，标识该Agent的名称
}

/**
 * @brief 获取 Agent 描述
 * @return 描述字符串
 */
QString Orchestrator::description() const
{
    return "总控Agent，负责生成执行计划并协调多Agent协作";  // 【返回】中文描述，用于UI展示和Prompt构建
}

/**
 * @brief 获取 Agent 类型
 * @return OrchestratorType
 */
AgentType Orchestrator::type() const
{
    return OrchestratorType;  // 【返回】固定类型枚举值，标识这是总控Agent
}

/**
 * @brief 获取所有可用工具
 * @return 所有子 Agent 工具的合并列表
 *
 * @implementation
 * 遍历所有已注册的子 Agent，收集每个 Agent 的工具列表，合并后返回。
 * 此方法用于 PromptBuilder 构造系统提示词时，让 LLM 了解所有可用工具。
 */
QList<Tool*> Orchestrator::tools() const
{
    QList<Tool*> allTools;       // 【创建】用于存储所有工具的临时列表
    for (Agent* agent : m_agents) {  // 【遍历】逐个访问已注册的所有子Agent
        allTools.append(agent->tools());  // 【添加】将当前Agent的工具列表追加到总列表
    }
    return allTools;             // 【返回】合并后的完整工具列表
}

/**
 * @brief 执行指定工具
 * @param toolName 工具名称
 * @param args 工具参数
 * @return 执行结果
 *
 * @implementation
 * 遍历所有已注册的 Agent，查找拥有该工具的 Agent 并执行。
 * 这是 Agent 基类的接口实现，但在 v2.0 中实际执行由 TaskScheduler 负责。
 */
QVariantMap Orchestrator::executeTool(const QString& toolName, const QVariantMap& args)
{
    for (Agent* agent : m_agents) {           // 【遍历】逐个检查所有子Agent
        QList<Tool*> tools = agent->tools();    // 【获取】当前Agent的工具列表
        for (Tool* tool : tools) {              // 【遍历】检查当前Agent的每个工具
            if (tool->name() == toolName) {     // 【匹配】如果找到名称匹配的工具
                return agent->executeTool(toolName, args);  // 【执行】委托给该Agent执行并返回结果
            }
        }
    }

    QVariantMap result;                         // 【创建】构造失败结果
    result["success"] = false;                  // 【设置】标记执行失败
    result["error"] = "Tool not found: " + toolName;  // 【设置】错误信息：找不到该工具
    return result;                              // 【返回】失败结果
}

/**
 * @brief 设置 LLM 客户端
 * @param client LlmClient 实例指针
 *
 * @implementation
 * 保存客户端指针，并连接两个关键信号：
 * - responseReceived → onLlmResponse：处理 LLM 正常返回的响应
 * - responseError → onLlmError：处理网络错误或 API 错误
 */
void Orchestrator::setLlmClient(LlmClient* client)
{
    m_llmClient = client;  // 【保存】记录LLM客户端指针
    // 【信号连接】LLM正常响应 → 本类的响应处理槽
    connect(m_llmClient, &LlmClient::responseReceived, this, &Orchestrator::onLlmResponse);
    // 【信号连接】LLM错误 → 本类的错误处理槽
    connect(m_llmClient, &LlmClient::responseError, this, &Orchestrator::onLlmError);
    // 【信号连接】LLM流式片段 → 本类的流式处理槽
    connect(m_llmClient, &LlmClient::responseChunk, this, &Orchestrator::onLlmResponseChunk);
}

/**
 * @brief 添加子 Agent
 * @param agent 要添加的 Agent
 *
 * @implementation
 * 同时完成两件事：
 * 1. m_agents.append(agent) — 用于 tools() 汇总和 PromptBuilder 生成提示词
 * 2. m_scheduler->registerAgent(agent) — 用于执行时的工具路由
 */
void Orchestrator::addAgent(Agent* agent)
{
    m_agents.append(agent);              // 【添加】将Agent加入本地列表，供tools()使用
    m_scheduler->registerAgent(agent);   // 【注册】将Agent注册到调度器，供执行时使用
}

/**
 * @brief 判断当前是否正在处理请求
 * @return true 当处理中
 */
bool Orchestrator::isProcessing() const
{
    return m_isProcessing;  // 【返回】当前处理状态标志
}

/**
 * @brief 获取当前执行计划的副本
 * @return 当前 TaskPlan
 */
TaskPlan Orchestrator::currentPlan() const
{
    return m_scheduler->currentPlan();  // 【委托】直接返回调度器中的当前计划副本
}

QJsonArray Orchestrator::conversationHistory() const
{
    return m_conversationManager->toJson();  // 【委托】获取对话管理器中的所有对话JSON
}

void Orchestrator::loadConversationHistory(const QJsonArray& history)
{
    m_conversationManager->loadFromJson(history);  // 【委托】将JSON数据加载到对话管理器
}

void Orchestrator::switchConversation(int index)
{
    m_conversationManager->switchConversation(index);  // 【委托】切换到指定索引的对话
}

int Orchestrator::currentConversationIndex() const
{
    return m_conversationManager->currentConversationIndex();  // 【委托】获取当前对话索引
}

void Orchestrator::createNewConversation()
{
    m_conversationManager->createNewConversation();  // 【委托】创建新对话
}

void Orchestrator::deleteConversation(int index)
{
    m_conversationManager->deleteConversation(index);  // 【委托】删除指定对话
}

/**
 * @brief 处理用户查询 —— 主入口
 * @param query 用户输入
 *
 * @implementation
 * 这是用户与系统交互的主入口。流程：
 * 1. 前置检查：LLM 客户端已设置 && 当前未在处理中
 * 2. 设置处理标志：m_isProcessing = true
 * 3. 设置规划标志：m_isPlanningPhase = true（影响 onLlmResponse 的处理分支）
 * 4. 保存用户输入：m_currentQuery = query（后续用于填充 plan.goal）
 * 5. 调用 sendPlanningRequest() 向 LLM 发送规划请求
 *
 * 如果当前正在处理其他任务，直接返回，防止并发冲突。
 */
void Orchestrator::processQuery(const QString& query)
{
    processQueryWithFiles(query, QStringList());  // 【委托】调用带文件版本，文件列表为空
}

void Orchestrator::processQueryWithFiles(const QString& query, const QStringList& files)
{
    // 【前置检查】如果没有设置LLM客户端，或者当前正在处理中，直接返回不处理
    if (!m_llmClient || m_isProcessing) {
        return;
    }

    // 【任务类型检测】通过关键词匹配分析用户的真实意图
    TaskType taskType = detectTaskType(query, files);
    // 【分支判断】如果是文件处理任务且用户上传了文件，走特殊处理流程
    if (isFileProcessingTask(taskType) && !files.isEmpty()) {
        handleFileProcessingTask(query, files, taskType);  // 【调用】文件处理专用流程
        return;  // 【返回】文件处理流程内部会管理状态，这里直接返回
    }

    m_isProcessing = true;        // 【状态设置】标记为正在处理，防止并发请求
    m_isPlanningPhase = true;     // 【状态设置】标记为规划阶段，影响后续LLM响应的处理方式
    m_currentQuery = query;       // 【保存】记录用户原始输入，用于填充plan.goal

    QString fullQuery = query;    // 【创建】构造完整的查询字符串，初始为原始输入

    // 【文件信息附加】如果用户上传了文件，附加文件信息到查询中
    if (!files.isEmpty()) {
        QString fileInfo;         // 【创建】用于存储文件信息的字符串
        for (const QString& filePath : files) {  // 【遍历】处理每个上传的文件
            QFileInfo info(filePath);  // 【创建】获取文件信息对象
            QString sizeStr;      // 【创建】用于存储格式化后的大小字符串
            // 【分支】根据文件大小选择合适的单位显示
            if (info.size() < 1024) {
                sizeStr = QString("%1 B").arg(info.size());  // 【格式化】小于1KB显示字节数
            } else if (info.size() < 1024 * 1024) {
                sizeStr = QString("%1 KB").arg(info.size() / 1024);  // 【格式化】小于1MB显示KB
            } else {
                sizeStr = QString("%1 MB").arg(static_cast<double>(info.size()) / (1024 * 1024), 0, 'f', 2);  // 【格式化】大于1MB显示MB，保留2位小数
            }
            // 【拼接】将文件名、大小、修改时间拼接到文件信息中
            fileInfo += QString("- 文件: %1 (%2, 修改时间: %3)\n").arg(info.fileName()).arg(sizeStr).arg(info.lastModified().toString("yyyy-MM-dd HH:mm:ss"));
        }
        // 【拼接】将文件信息附加到用户查询末尾，告知LLM参考这些文件
        fullQuery += QString("\n\n附加文件信息：\n%1\n请参考这些文件的内容来回答问题。").arg(fileInfo);
    }

    m_conversationManager->addUserMessage(fullQuery);  // 【记录】将用户消息加入对话历史

    sendPlanningRequest(fullQuery);  // 【调用】向LLM发送规划请求
}

/**
 * @brief 停止当前执行
 *
 * @implementation
 * 1. 如果 TaskScheduler 正在运行，调用 stopExecution()
 * 2. 重置 Orchestrator 的处理状态标志
 */
void Orchestrator::stopExecution()
{
    if (m_scheduler->isRunning()) {     // 【检查】如果调度器正在运行
        m_scheduler->stopExecution();   // 【调用】通知调度器停止后续步骤
    }
    m_isProcessing = false;             // 【重置】清除处理中标志
    m_isPlanningPhase = false;          // 【重置】清除规划阶段标志
}

/**
 * @brief 向 LLM 发送规划请求
 * @param query 用户原始输入
 *
 * @implementation
 * 1. 调用 PromptBuilder::buildPlanningSystemPrompt(m_agents) 构造规划专用系统提示词
 *    该提示词包含所有 Agent/工具的 JSON Schema 描述和格式要求
 * 2. 调用 PromptBuilder::buildPlanningUserPrompt(query) 构造用户提示词
 * 3. 通过 LlmClient::sendPrompt() 发送 HTTP 请求
 */
void Orchestrator::sendPlanningRequest(const QString& query)
{
    // 【构造】生成规划专用的系统提示词，包含所有Agent和工具的说明
    QString systemPrompt = m_promptBuilder.buildPlanningSystemPrompt(m_agents);
    // 【构造】生成规划专用的用户提示词
    QString userPrompt = m_promptBuilder.buildPlanningUserPrompt(query);

    // 【获取】从对话管理器构建消息历史数组
    QJsonArray messages = m_conversationManager->buildMessageHistory(systemPrompt);

    // 【逻辑】确保消息列表的最后一条是用户消息，内容为当前查询
    if (!messages.isEmpty()) {
        QJsonObject lastMsg = messages.last().toObject();  // 【获取】最后一条消息
        if (lastMsg["role"].toString() == "user") {        // 【判断】如果最后是用户消息
            QJsonObject userMsg;                           // 【创建】新的用户消息对象
            userMsg["role"] = "user";                      // 【设置】角色为用户
            userMsg["content"] = query;                    // 【设置】内容为当前查询
            messages.replace(messages.size() - 1, userMsg); // 【替换】更新最后一条消息
        } else {
            QJsonObject userMsg;                           // 【创建】新的用户消息对象
            userMsg["role"] = "user";                      // 【设置】角色为用户
            userMsg["content"] = query;                    // 【设置】内容为当前查询
            messages.append(userMsg);                      // 【追加】添加到消息列表末尾
        }
    } else {
        // 【空列表处理】如果对话历史为空，构造完整的消息列表
        QJsonObject systemMsg;                             // 【创建】系统消息对象
        systemMsg["role"] = "system";                      // 【设置】角色为系统
        systemMsg["content"] = systemPrompt;               // 【设置】内容为系统提示词
        messages.append(systemMsg);                        // 【追加】添加系统消息

        QJsonObject userMsg;                               // 【创建】用户消息对象
        userMsg["role"] = "user";                          // 【设置】角色为用户
        userMsg["content"] = query;                        // 【设置】内容为当前查询
        messages.append(userMsg);                          // 【追加】添加用户消息
    }

    m_llmClient->sendMessages(messages, false);  // 【发送】通过LLM客户端发送消息，false表示非流式模式
}

/**
 * @brief LLM 响应到达的处理
 * @param response LLM 返回的文本
 *
 * @implementation
 * 新 Prompt 强制 LLM 输出 {"type":"chat"/"plan", ...}。
 * Planner::parseResponse() 自动识别并分流：
 *
 * Chat → 直接作为对话消息展示，重置处理状态
 * Plan → 将 goal 设置为用户原始输入，emit planGenerated，
 *         交 TaskScheduler 执行
 * 其他 → 降级为 Chat，展示原始文本
 */
void Orchestrator::onLlmResponseChunk(const QString& chunk)
{
    emit responseChunkReceived(chunk);  // 【透传】将LLM流式片段转发给UI
    emit logMessage("llm", chunk);      // 【日志】记录到日志面板，类型为llm
}

void Orchestrator::onLlmResponse(const QString& response)
{
    if (m_isPlanningPhase) {  // 【分支】如果当前处于规划阶段
        emit logMessage("system", "LLM 响应已收到，正在解析...");  // 【日志】通知UI正在解析
        // 【日志】输出LLM响应的前200个字符，过长则显示省略号
        emit logMessage("llm", "LLM原始响应: " + response.left(200) + (response.length() > 200 ? "..." : ""));

        // 【解析】调用Planner解析LLM响应，自动识别是chat还是plan
        ParsedResponse parsed = m_planner.parseResponse(response);

        // 【分支A】如果是普通对话响应
        if (parsed.type == ParsedResponseType::Chat) {
            m_conversationManager->addAssistantMessage(parsed.chatResponse);  // 【记录】将AI回复加入对话历史
            emit messageReceived(parsed.chatResponse);  // 【信号】通知UI显示AI回复
            m_isProcessing = false;      // 【重置】清除处理中标志
            m_isPlanningPhase = false;   // 【重置】清除规划阶段标志
            return;                      // 【返回】处理完毕
        }

        // 【分支B】如果是计划且步骤非空
        if (parsed.type == ParsedResponseType::Plan && !parsed.plan.steps.isEmpty()) {
            parsed.plan.goal = m_currentQuery;  // 【填充】将用户原始输入设置为计划目标
            emit planGenerated(parsed.plan);    // 【信号】通知UI计划已生成

            // 【日志】输出计划步骤数量
            emit logMessage("system", QString("计划生成成功，共 %1 个步骤").arg(parsed.plan.steps.size()));
            // 【日志】逐个输出每个步骤的信息
            for (const auto& step : parsed.plan.steps) {
                emit logMessage("agent", QString("步骤 %1: %2.%3").arg(step.stepId).arg(step.agent).arg(step.tool));
            }

            m_scheduler->setPlan(parsed.plan);      // 【设置】将计划交给调度器
            m_isPlanningPhase = false;               // 【状态】规划阶段结束
            m_scheduler->startExecution();           // 【启动】开始执行计划
            return;                                  // 【返回】执行由调度器接管
        }

        // 【兜底】不是有效chat也不是有效plan，当作纯文本展示
        m_conversationManager->addAssistantMessage(response);  // 【记录】加入对话历史
        emit messageReceived(response);           // 【信号】通知UI显示原文
        m_isProcessing = false;                   // 【重置】清除处理中标志
        m_isPlanningPhase = false;                // 【重置】清除规划阶段标志
    } else {
        // 【非规划阶段】检查是否是内容补充或文档扩写的响应
        if (!m_lastWrittenFilePath.isEmpty()) {   // 【判断】如果有待补充的文件
            QFile file(m_lastWrittenFilePath);    // 【创建】文件对象
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 【尝试】以只写文本模式打开文件
                QTextStream out(&file);           // 【创建】文本流用于写入
                out << response;                  // 【写入】将LLM响应写入文件
                file.close();                     // 【关闭】关闭文件

                // 【判断】判断是生成还是扩写操作
                QString operation = m_lastWrittenContent.isEmpty() ? "生成" : "扩写";
                // 【日志】通知文件操作完成
                emit logMessage("system", QString("文档%1完成，已保存到: %2").arg(operation).arg(m_lastWrittenFilePath));
                // 【信号】通知UI显示完成信息，包含文件路径和内容长度
                emit messageReceived(QString(
                    "✅ 文档%1完成！\n\n"
                    "文件: %2\n"
                    "%3"
                    "内容长度: %4 字符"
                ).arg(operation)
                 .arg(m_lastWrittenFilePath)
                 .arg(m_lastWrittenContent.isEmpty() ? "" : QString("原始长度: %1 字符\n").arg(m_lastWrittenContent.length()))
                 .arg(response.length()));
            } else {
                // 【失败】文件无法打开保存
                emit messageReceived(QString("❌ 无法保存文档到: %1").arg(m_lastWrittenFilePath));
            }
            m_lastWrittenFilePath.clear();        // 【清理】清除记录的文件路径
            m_lastWrittenContent.clear();         // 【清理】清除记录的内容摘要
            m_isProcessing = false;               // 【重置】清除处理中标志
            return;                               // 【返回】处理完毕
        }
        m_conversationManager->addAssistantMessage(response);  // 【记录】加入对话历史
        emit messageReceived(response);           // 【信号】通知UI显示回复
    }
}

/**
 * @brief LLM 请求出错处理
 * @param error 错误信息
 *
 * @implementation
 * 发射 errorOccurred 信号并重置所有处理状态，允许用户发起新的请求。
 */
void Orchestrator::onLlmError(const QString& error)
{
    emit errorOccurred("LLM错误: " + error);  // 【信号】通知UI显示错误，前缀标注来源
    m_isProcessing = false;                   // 【重置】清除处理中标志，允许新请求
    m_isPlanningPhase = false;                // 【重置】清除规划阶段标志
}

// ========================================================================
// 以下五个槽函数是 TaskScheduler 信号的"透传代理"
// 它们不做任何业务逻辑处理，直接将信号转发为 Orchestrator 的同名信号
// 这种设计实现了 TaskScheduler → Orchestrator → UI 的解耦信号链
// ========================================================================

/**
 * @brief 透传：步骤开始
 * @param stepId 步骤编号
 */
void Orchestrator::onSchedulerStepStarted(int stepId)
{
    emit stepStarted(stepId);  // 【透传】将步骤开始信号转发给UI
    emit logMessage("execution", QString("步骤 %1 开始执行").arg(stepId));  // 【日志】记录执行日志
}

/**
 * @brief 透传：步骤输出
 * @param stepId 步骤编号
 * @param output 输出文本
 */
void Orchestrator::onSchedulerStepOutput(int stepId, const QString& output)
{
    emit stepOutput(stepId, output);  // 【透传】将步骤输出信号转发给UI
    // 【日志】记录输出前100个字符，过长则截断
    emit logMessage("execution", QString("步骤 %1 输出: %2").arg(stepId).arg(output.left(100)));
}

/**
 * @brief 透传：步骤完成
 * @param stepId 步骤编号
 * @param result 执行结果
 */
void Orchestrator::onSchedulerStepCompleted(int stepId, const QVariantMap& result)
{
    emit stepCompleted(stepId, result);  // 【透传】将步骤完成信号转发给UI
    bool success = result.value("success", false).toBool();  // 【获取】从结果中提取成功标志，默认false
    if (success) {
        emit logMessage("execution", QString("步骤 %1 执行成功").arg(stepId));  // 【日志】成功记录
    } else {
        emit logMessage("execution", QString("步骤 %1 执行失败: %2").arg(stepId).arg(result.value("error", "未知错误").toString()));  // 【日志】失败记录
    }
}

/**
 * @brief 透传：步骤失败
 * @param stepId 步骤编号
 * @param error 错误信息
 */
void Orchestrator::onSchedulerStepFailed(int stepId, const QString& error)
{
    emit stepFailed(stepId, error);  // 【透传】将步骤失败信号转发给UI
    emit logMessage("execution", QString("步骤 %1 失败: %2").arg(stepId).arg(error));  // 【日志】记录失败原因
}

/**
 * @brief 计划执行完毕处理
 * @param plan 包含最终状态的 TaskPlan
 *
 * @implementation
 * 1. 透传 planCompleted 信号到 UI
 * 2. 重置 m_isProcessing 标志，允许新的用户请求
 * 3. 调用 generateSummary() 生成执行统计，通过 messageReceived 展示
 */
void Orchestrator::onSchedulerPlanCompleted(const TaskPlan& plan)
{
    emit planCompleted(plan);  // 【透传】通知UI计划执行完毕
    m_isProcessing = false;    // 【重置】清除处理中标志

    int failCount = 0;         // 【计数】统计失败步骤数量
    for (const auto& step : plan.steps) {  // 【遍历】检查每个步骤的状态
        if (step.status == StepStatus::Failed) failCount++;  // 【计数】失败步骤+1
    }

    // 【反思机制】如果所有步骤都失败且未达到最大重试次数，尝试一次反思重试
    if (failCount > 0 && failCount == plan.steps.size() && m_retryCount < MAX_RETRY) {
        m_retryCount++;        // 【递增】重试计数+1
        retryPlanWithFeedback(plan);  // 【调用】带上失败反馈重新规划
        return;                // 【返回】重试流程会重新进入规划阶段
    }

    // 【内容补充检测】如果任务是内容生成且写入的文件只有大纲，自动补充完整内容
    if (isContentGenerationTask(m_currentQuery)) {  // 【判断】是否是内容生成任务
        for (const auto& step : plan.steps) {       // 【遍历】检查每个步骤
            // 【判断】步骤成功完成且是写文件或生成代码工具
            if (step.status == StepStatus::Completed &&
                (step.tool == "writeFile" || step.tool == "generateCode")) {
                QString filePath = step.args.value("path").toString();    // 【提取】文件路径参数
                QString content = step.args.value("content").toString();  // 【提取】文件内容参数
                // 【判断】如果文件路径有效且内容只有大纲
                if (!filePath.isEmpty() && isOutlineOnlyContent(content)) {
                    m_lastWrittenFilePath = filePath;    // 【记录】保存文件路径供补充流程使用
                    m_lastWrittenContent = content;      // 【记录】保存原始内容
                    emit logMessage("system", "检测到文档内容只有大纲，正在补充生成完整内容...");  // 【日志】
                    supplementContentForFile(filePath, content, m_currentQuery);  // 【调用】发起补充请求
                    m_retryCount = 0;                    // 【重置】重试计数归零
                    return;                              // 【返回】补充流程会再次调用LLM
                }
            }
        }
    }

    m_retryCount = 0;          // 【重置】重试计数归零
    generateSummary(plan);     // 【调用】生成执行总结
}

void Orchestrator::retryPlanWithFeedback(const TaskPlan& failedPlan)
{
    // 【收集】收集所有失败步骤的错误信息
    QStringList failures;      // 【创建】存储失败信息的字符串列表
    for (const auto& step : failedPlan.steps) {  // 【遍历】检查每个步骤
        if (step.status == StepStatus::Failed) {   // 【判断】只收集失败的步骤
            failures << QString("- 步骤 %1 (%2): %3")  // 【格式化】步骤ID、描述、错误信息
                .arg(step.stepId)
                .arg(step.description)
                .arg(step.error.isEmpty() ? "未知错误" : step.error);
        }
    }

    // 【构造】构建反思反馈提示词，要求LLM分析错误并生成新计划
    QString feedback = QString(
        "上次的执行计划全部失败，请分析错误并生成新的计划。\n"
        "用户原始需求：%1\n\n"
        "失败原因：\n%2\n\n"
        "请使用合适的工具（如 writeFile 而不是 echo+shell），生成新计划。"
    ).arg(m_currentQuery, failures.join("\n"));  // 【填充】插入用户原始需求和失败原因

    m_conversationManager->addUserMessage(feedback);  // 【记录】将反馈加入对话历史
    m_isProcessing = true;        // 【设置】重新标记为处理中
    m_isPlanningPhase = true;     // 【设置】重新进入规划阶段
    sendPlanningRequest(feedback); // 【调用】发送反思后的规划请求
}

void Orchestrator::onSchedulerExecutionStopped()
{
    m_isProcessing = false;       // 【重置】清除处理中标志
    m_isPlanningPhase = false;    // 【重置】清除规划阶段标志
    m_retryCount = 0;             // 【重置】重试计数归零
}

/**
 * @brief 生成执行总结
 * @param plan 执行完毕的 TaskPlan
 *
 * @implementation
 * 遍历所有步骤，统计成功数和失败数，构造总结文本。
 * 示例输出："任务执行完成。共 3 个步骤，成功 2 个，失败 1 个。"
 *
 * @note 总结通过 messageReceived 信号发送到 UI，展示在输出面板中。
 */
void Orchestrator::generateSummary(const TaskPlan& plan)
{
    int successCount = 0;         // 【计数】成功步骤数
    int failCount = 0;            // 【计数】失败步骤数
    for (const auto& step : plan.steps) {  // 【遍历】统计每个步骤的结果
        if (step.status == StepStatus::Completed) successCount++;  // 【计数】成功+1
        else if (step.status == StepStatus::Failed) failCount++;   // 【计数】失败+1
    }

    // 【构造】生成总结文本字符串
    QString summary = QString("任务执行完成。共 %1 个步骤，成功 %2 个，失败 %3 个。")
        .arg(plan.totalSteps()).arg(successCount).arg(failCount);

    m_conversationManager->addAssistantMessage(summary);  // 【记录】将总结加入对话历史
    emit messageReceived(summary);  // 【信号】通知UI显示总结
}

/**
 * @brief 判断用户查询是否是内容生成类任务
 * @param query 用户输入
 * @return true 如果是写文档、文章、报告、代码等内容生成任务
 */
bool Orchestrator::isContentGenerationTask(const QString& query) const
{
    // 【定义】文档类关键词列表
    QStringList docKeywords = {
        "写", "编写", "撰写", "生成", "创建", "设计", "制作",
        "文档", "文章", "报告", "技术文档", "论文", "说明书",
        "README", "readme", "总结", "笔记", "手册", "指南",
        "doc", "document", "report", "essay", "article", "guide"
    };

    // 【定义】代码类关键词列表
    QStringList codeKeywords = {
        "实现", "代码", "程序", "脚本", "函数", "类", "模块",
        "python", "cpp", "java", "javascript", "typescript", "go",
        "游戏", "应用", "工具", "算法", "界面", "功能"
    };

    // 【定义】配置类关键词列表
    QStringList configKeywords = {
        "配置", "config", "json", "yaml", "xml", "ini",
        "设置文件", "配置文件"
    };

    // 【检查】遍历文档关键词，不区分大小写匹配
    for (const QString& kw : docKeywords) {
        if (query.contains(kw, Qt::CaseInsensitive)) {
            return true;  // 【返回】匹配到文档关键词，是内容生成任务
        }
    }
    // 【检查】遍历代码关键词
    for (const QString& kw : codeKeywords) {
        if (query.contains(kw, Qt::CaseInsensitive)) {
            return true;  // 【返回】匹配到代码关键词
        }
    }
    // 【检查】遍历配置关键词
    for (const QString& kw : configKeywords) {
        if (query.contains(kw, Qt::CaseInsensitive)) {
            return true;  // 【返回】匹配到配置关键词
        }
    }
    return false;  // 【返回】未匹配到任何关键词，不是内容生成任务
}

/**
 * @brief 判断内容是否只有大纲/模板（缺少实质正文）
 * @param content 文件内容
 * @return true 如果内容主要是标题行/注释，缺少详细内容
 */
bool Orchestrator::isOutlineOnlyContent(const QString& content) const
{
    if (content.length() < 200) {  // 【快速判断】如果内容长度小于200字符，直接认为是大纲
        return true;
    }

    int totalLines = 0;       // 【计数】非空总行数
    int headingLines = 0;     // 【计数】标题行数
    int commentLines = 0;     // 【计数】注释行数
    int codeStubLines = 0;    // 【计数】代码占位符行数

    QStringList lines = content.split("\n");  // 【分割】按换行符分割内容为行列表
    for (const QString& line : lines) {       // 【遍历】逐行分析
        QString trimmed = line.trimmed();     // 【处理】去除首尾空白
        if (trimmed.isEmpty()) {              // 【判断】跳过空行
            continue;
        }
        totalLines++;                         // 【计数】非空行+1

        // 【判断】Markdown标题行：以#开头或===、---分隔线
        if (trimmed.startsWith("#") || trimmed.startsWith("===") || trimmed.startsWith("---")) {
            headingLines++;
        }
        // 【判断】注释行：各种编程语言的注释格式
        else if (trimmed.startsWith("//") || trimmed.startsWith("/*") ||
                 trimmed.startsWith("*") || trimmed.startsWith("# ") ||
                 trimmed.startsWith("--") || trimmed.startsWith("<!--")) {
            commentLines++;
        }
        // 【判断】代码占位符：省略号、TODO、FIXME等
        else if (trimmed == "..." || trimmed == "// ..." ||
                 trimmed.contains("TODO") || trimmed.contains("FIXME") ||
                 trimmed == "pass" || trimmed == "// implementation") {
            codeStubLines++;
        }
    }

    if (totalLines == 0) return true;  // 【保护】如果没有非空行，认为是大纲

    // 【判断】如果标题+注释+占位符占比超过50%，认为内容不完整
    int nonContentLines = headingLines + commentLines + codeStubLines;
    if (nonContentLines * 100 / totalLines > 50) {
        return true;
    }

    // 【判断】如果平均每行少于25个字符（代码除外），认为内容不充实
    int avgLineLength = content.length() / qMax(1, totalLines);
    if (avgLineLength < 25 && commentLines < totalLines * 0.3) {
        return true;
    }

    // 【判断】检查是否只有函数签名没有实现（花括号内平均少于3行）
    int openBraces = content.count('{');   // 【统计】左花括号数量
    int closeBraces = content.count('}');  // 【统计】右花括号数量
    if (openBraces > 0 && closeBraces > 0) {
        int avgLinesPerBlock = totalLines / qMax(1, openBraces);
        if (avgLinesPerBlock < 3) {
            return true;  // 【返回】每个代码块平均少于3行，认为是大纲
        }
    }

    return false;  // 【返回】内容充实，不是大纲
}

/**
 * @brief 为文件补充完整内容
 * @param filePath 文件路径
 * @param originalContent 原始内容（大纲/框架）
 * @param userQuery 用户原始请求
 *
 * @details
 * 根据文件类型选择不同的补充策略：
 * - Markdown/文档：生成详细段落
 * - Python/JS：生成完整可运行代码
 * - C++/Qt：生成完整实现
 */
void Orchestrator::supplementContentForFile(const QString& filePath, const QString& originalContent, const QString& userQuery)
{
    QFileInfo fileInfo(filePath);              // 【创建】获取文件信息
    QString suffix = fileInfo.suffix().toLower();  // 【获取】文件扩展名并转为小写

    QString systemPrompt;                      // 【声明】系统提示词变量
    QString userPrompt;                        // 【声明】用户提示词变量

    // 【分支】根据文件扩展名选择不同的补充策略
    if (suffix == "md" || suffix == "markdown" || suffix == "txt" || suffix.isEmpty()) {
        // 【文档类型】Markdown、纯文本或无扩展名
        systemPrompt = "你是一个专业的技术文档撰写专家。你的任务是根据大纲生成完整、详细、有深度的文档内容。"
                       "每个章节都必须包含充实的正文段落，不能只有标题。内容要专业、具体、有深度。";
        // 【构造】文档补充的用户提示词
        userPrompt = QString(
            "请根据以下大纲，生成一份完整、详细、充实的技术文档。\n"
            "要求：\n"
            "1. 每个章节必须有详细的段落说明，不能只有标题\n"
            "2. 内容要专业、具体、有深度，适合用于正式汇报\n"
            "3. 使用 Markdown 格式\n"
            "4. 直接输出文档内容，不要添加前言或解释\n\n"
            "用户原始需求：%1\n\n"
            "大纲：\n%2"
        ).arg(userQuery, originalContent);
    }
    else if (suffix == "py") {
        // 【Python代码】
        systemPrompt = "你是一个 Python 专家。你的任务是根据代码框架生成完整可运行的代码。"
                       "所有函数必须有完整实现，不能只有 pass 或注释。代码要健壮、有错误处理。";
        userPrompt = QString(
            "请根据以下 Python 代码框架，生成完整可运行的代码。\n"
            "要求：\n"
            "1. 所有函数必须有完整实现，不能只有 pass 或 TODO\n"
            "2. 添加必要的错误处理\n"
            "3. 代码要健壮、高效\n"
            "4. 直接输出完整代码，不要添加解释\n\n"
            "用户原始需求：%1\n\n"
            "代码框架：\n%2"
        ).arg(userQuery, originalContent);
    }
    else if (suffix == "cpp" || suffix == "h" || suffix == "hpp") {
        // 【C++代码】
        systemPrompt = "你是一个 C++ 专家。你的任务是根据代码框架生成完整可编译的代码。"
                       "所有函数必须有完整实现，不能只有声明或 TODO。";
        userPrompt = QString(
            "请根据以下 C++ 代码框架，生成完整可编译的代码。\n"
            "要求：\n"
            "1. 所有函数必须有完整实现\n"
            "2. 包含必要的头文件\n"
            "3. 代码要高效、健壮\n"
            "4. 直接输出完整代码，不要添加解释\n\n"
            "用户原始需求：%1\n\n"
            "代码框架：\n%2"
        ).arg(userQuery, originalContent);
    }
    else if (suffix == "js" || suffix == "ts") {
        // 【JavaScript/TypeScript】
        systemPrompt = "你是一个 JavaScript/TypeScript 专家。你的任务是根据代码框架生成完整可运行的代码。"
                       "所有函数必须有完整实现。";
        userPrompt = QString(
            "请根据以下代码框架，生成完整可运行的 JavaScript/TypeScript 代码。\n"
            "要求：\n"
            "1. 所有函数必须有完整实现\n"
            "2. 添加必要的错误处理\n"
            "3. 代码要清晰、高效\n"
            "4. 直接输出完整代码\n\n"
            "用户原始需求：%1\n\n"
            "代码框架：\n%2"
        ).arg(userQuery, originalContent);
    }
    else {
        // 【其他类型】通用处理
        systemPrompt = "你是一个内容创作专家。请根据框架生成完整详细的内容。";
        userPrompt = QString(
            "请根据以下框架，生成完整详细的内容。\n\n"
            "用户原始需求：%1\n\n"
            "框架：\n%2"
        ).arg(userQuery, originalContent);
    }

    m_isProcessing = true;        // 【设置】标记为处理中，防止并发

    QJsonArray messages;          // 【创建】消息数组
    QJsonObject systemMsg;        // 【创建】系统消息对象
    systemMsg["role"] = "system"; // 【设置】角色为系统
    systemMsg["content"] = systemPrompt;  // 【设置】系统提示词内容
    messages.append(systemMsg);   // 【追加】添加系统消息

    QJsonObject userMsg;          // 【创建】用户消息对象
    userMsg["role"] = "user";     // 【设置】角色为用户
    userMsg["content"] = userPrompt;  // 【设置】用户提示词内容
    messages.append(userMsg);     // 【追加】添加用户消息

    m_llmClient->sendMessages(messages, false);  // 【发送】调用LLM进行内容补充
}

/**
 * @brief 判断任务类型是否属于文件处理任务（需要读取→LLM处理→写入模式）
 * @param type 任务类型
 * @return true 如果是文件处理任务
 */
bool Orchestrator::isFileProcessingTask(TaskType type) const
{
    switch (type) {  // 【分支】根据任务类型判断
    case TaskType::DocumentExpansion:    // 【匹配】文档扩写
    case TaskType::DocumentModification: // 【匹配】文档修改
    case TaskType::CodeAddition:         // 【匹配】代码添加
    case TaskType::CodeFix:              // 【匹配】代码修复
    case TaskType::CodeRefactor:         // 【匹配】代码重构
    case TaskType::ConfigUpdate:         // 【匹配】配置更新
    case TaskType::GenericEdit:          // 【匹配】通用编辑
        return true;                     // 【返回】属于文件处理任务
    default:
        return false;                    // 【返回】不属于文件处理任务
    }
}

/**
 * @brief 检测用户查询对应的任务类型
 * @param query 用户输入
 * @param files 用户上传的文件列表
 * @return 任务类型
 *
 * @details
 * 通过关键词匹配识别用户的意图，覆盖所有可能的场景：
 *
 * 文件处理类（读取→LLM处理→写入）：
 * - DocumentExpansion: 扩写、丰富、扩充、扩展、完善
 * - DocumentModification: 修改、润色、翻译、改写、重写、调整
 * - CodeAddition: 添加功能、增加功能、加入、新增、实现
 * - CodeFix: 修复、修bug、debug、fix、解决、排查
 * - CodeRefactor: 重构、优化代码、改进代码
 * - ConfigUpdate: 更新配置、修改配置
 * - GenericEdit: 编辑、改、调整
 *
 * 代码开发类：
 * - CodeGeneration: 生成代码、写代码、创建代码文件
 * - ProjectCreation: 创建项目、新建项目、项目模板
 * - CodeCompilation: 编译、构建、make、build
 * - CodeExecution: 运行、执行、run、调试
 * - CodeAnalysis: 分析代码、代码审查、code review
 *
 * 文本处理类：
 * - TextTransformation: 转换、格式化、编码、解码、加密、解密
 * - DataExtraction: 提取、解析、获取信息
 * - TextAnalysis: 统计、分析文本、情感分析、关键词提取
 *
 * 系统操作类：
 * - SystemInfo: 系统信息、电脑信息、硬件信息
 * - ProcessManagement: 进程、任务管理器、结束进程
 * - EnvironmentSetup: 安装、配置环境、依赖安装
 * - FileSystemOperation: 复制、移动、批量处理、删除
 *
 * 搜索查询类：
 * - LocalFileSearch: 搜索文件、查找文件、文件搜索
 * - CodeSearch: 搜索代码、查找函数、查找类
 * - TextSearch: 搜索文本、查找内容
 * - KnowledgeQuery: 问答、解释、知识、原理、概念
 */
Orchestrator::TaskType Orchestrator::detectTaskType(const QString& query, const QStringList& files) const
{
    QString lowerQuery = query.toLower();  // 【转换】将查询转为小写，实现不区分大小写匹配

    // ========== 代码修复类（最高优先级）==========
    QStringList codeFixKeywords = {
        "修复", "修bug", "debug", "fix", "解决bug", "排查", "找错",
        "repair", "fix bug", "debug", "troubleshoot", "resolve issue",
        "错误", "异常", "崩溃", "闪退", "报错", "error", "exception", "crash"
    };
    for (const QString& kw : codeFixKeywords) {  // 【遍历】检查每个关键词
        if (lowerQuery.contains(kw.toLower())) {  // 【匹配】如果查询包含该关键词
            return files.isEmpty() ? TaskType::CodeAnalysis : TaskType::CodeFix;  // 【返回】有文件则修复，无文件则分析
        }
    }

    // ========== 代码重构类 ==========
    QStringList codeRefactorKeywords = {
        "重构", "优化代码", "改进代码", "代码优化", "代码重构",
        "refactor", "optimize code", "improve code", "code cleanup",
        "简化代码", "代码瘦身", "代码整理"
    };
    for (const QString& kw : codeRefactorKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::CodeAnalysis : TaskType::CodeRefactor;
        }
    }

    // ========== 代码添加功能类 ==========
    QStringList codeAdditionKeywords = {
        "添加功能", "增加功能", "加入", "新增功能", "实现功能", "添加新功能",
        "add feature", "implement", "add function", "new feature", "implement feature",
        "新增", "添加", "实现", "开发"
    };
    for (const QString& kw : codeAdditionKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::CodeGeneration : TaskType::CodeAddition;
        }
    }

    // ========== 文档扩写类 ==========
    QStringList docExpansionKeywords = {
        "扩写", "丰富", "扩充", "扩展", "完善", "充实",
        "expand", "enrich", "elaborate", "flesh out", "extend",
        "详细说明", "详细介绍", "深入分析"
    };
    for (const QString& kw : docExpansionKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::CodeGeneration : TaskType::DocumentExpansion;
        }
    }

    // ========== 文档修改类 ==========
    QStringList docModificationKeywords = {
        "修改", "润色", "翻译", "改写", "重写", "调整", "更新", "升级",
        "modify", "polish", "translate", "rewrite", "revise", "update", "refine",
        "格式转换", "markdown", "html", "word", "pdf"
    };
    for (const QString& kw : docModificationKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::TextTransformation : TaskType::DocumentModification;
        }
    }

    // ========== 配置更新类 ==========
    QStringList configKeywords = {
        "更新配置", "修改配置", "改配置", "调整配置",
        "update config", "modify config", "change config",
        "配置文件", "设置", "参数调整"
    };
    for (const QString& kw : configKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::CodeGeneration : TaskType::ConfigUpdate;
        }
    }

    // ========== 项目创建类 ==========
    QStringList projectCreationKeywords = {
        "创建项目", "新建项目", "项目模板", "初始化项目",
        "create project", "new project", "project template", "init project",
        "工程", "新项目", "脚手架"
    };
    for (const QString& kw : projectCreationKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::ProjectCreation;
        }
    }

    // ========== 代码编译类 ==========
    QStringList codeCompilationKeywords = {
        "编译", "构建", "make", "build", "compile",
        "cmake", "gcc", "g++", "msbuild", "mvn", "gradle",
        "构建项目", "编译代码"
    };
    for (const QString& kw : codeCompilationKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::CodeCompilation;
        }
    }

    // ========== 代码执行类 ==========
    QStringList codeExecutionKeywords = {
        "运行", "执行", "run", "调试", "test",
        "运行程序", "执行脚本", "启动", "运行代码"
    };
    for (const QString& kw : codeExecutionKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::CodeExecution;
        }
    }

    // ========== 代码分析类 ==========
    QStringList codeAnalysisKeywords = {
        "分析代码", "代码审查", "code review", "代码检查",
        "静态分析", "代码质量", "代码统计", "复杂度分析"
    };
    for (const QString& kw : codeAnalysisKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::CodeAnalysis;
        }
    }

    // ========== 文本转换类 ==========
    QStringList textTransformationKeywords = {
        "转换", "格式化", "编码", "解码", "加密", "解密",
        "转换格式", "格式化文本", "base64", "url编码", "json格式化",
        "xml", "yaml", "csv", "数据转换"
    };
    for (const QString& kw : textTransformationKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::TextTransformation;
        }
    }

    // ========== 数据提取类 ==========
    QStringList dataExtractionKeywords = {
        "提取", "解析", "获取信息", "提取信息",
        "parse", "extract", "提取数据", "解析数据"
    };
    for (const QString& kw : dataExtractionKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::TextAnalysis : TaskType::DataExtraction;
        }
    }

    // ========== 文本分析类 ==========
    QStringList textAnalysisKeywords = {
        "统计", "分析文本", "情感分析", "关键词提取",
        "字数统计", "词频分析", "文本分析", "内容分析"
    };
    for (const QString& kw : textAnalysisKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::TextAnalysis;
        }
    }

    // ========== 系统信息类 ==========
    QStringList systemInfoKeywords = {
        "系统信息", "电脑信息", "硬件信息", "配置信息",
        "system info", "hardware", "cpu", "memory", "磁盘",
        "操作系统", "版本信息", "环境信息"
    };
    for (const QString& kw : systemInfoKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::SystemInfo;
        }
    }

    // ========== 进程管理类 ==========
    QStringList processManagementKeywords = {
        "进程", "任务管理器", "结束进程", "kill",
        "process", "task manager", "终止进程", "查看进程"
    };
    for (const QString& kw : processManagementKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::ProcessManagement;
        }
    }

    // ========== 环境配置类 ==========
    QStringList environmentSetupKeywords = {
        "安装", "配置环境", "依赖安装", "环境变量",
        "install", "setup", "npm install", "pip install",
        "配置开发环境", "环境搭建"
    };
    for (const QString& kw : environmentSetupKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::EnvironmentSetup;
        }
    }

    // ========== 文件系统操作类 ==========
    QStringList fileSystemOperationKeywords = {
        "复制", "移动", "批量处理", "删除",
        "copy", "move", "rename", "批量重命名",
        "批量删除", "文件操作", "目录操作"
    };
    for (const QString& kw : fileSystemOperationKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::FileSystemOperation;
        }
    }

    // ========== 本地文件搜索类 ==========
    QStringList localFileSearchKeywords = {
        "搜索文件", "查找文件", "文件搜索",
        "find file", "search file", "定位文件"
    };
    for (const QString& kw : localFileSearchKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::LocalFileSearch;
        }
    }

    // ========== 代码搜索类 ==========
    QStringList codeSearchKeywords = {
        "搜索代码", "查找函数", "查找类",
        "search code", "find function", "find class",
        "查找定义", "搜索变量"
    };
    for (const QString& kw : codeSearchKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::CodeSearch;
        }
    }

    // ========== 文本搜索类 ==========
    QStringList textSearchKeywords = {
        "搜索文本", "查找内容", "grep",
        "search text", "find text", "查找字符串"
    };
    for (const QString& kw : textSearchKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::TextSearch;
        }
    }

    // ========== 知识问答类 ==========
    QStringList knowledgeQueryKeywords = {
        "什么是", "为什么", "原理", "概念",
        "解释", "说明", "问答", "知识",
        "how", "why", "what is", "explain", "describe",
        "定义", "含义", "区别", "对比", "比较"
    };
    for (const QString& kw : knowledgeQueryKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return TaskType::KnowledgeQuery;
        }
    }

    // ========== 代码生成类 ==========
    QStringList codeGenerationKeywords = {
        "生成代码", "写代码", "创建代码文件",
        "generate code", "write code", "code",
        "编程", "写程序", "代码"
    };
    for (const QString& kw : codeGenerationKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::CodeGeneration : TaskType::GenericEdit;
        }
    }

    // ========== 通用编辑类（最宽泛）==========
    QStringList genericEditKeywords = {
        "编辑", "改", "调整", "改动",
        "edit", "change", "adjust", "modify file",
        "处理", "操作", "弄一下", "帮忙"
    };
    for (const QString& kw : genericEditKeywords) {
        if (lowerQuery.contains(kw.toLower())) {
            return files.isEmpty() ? TaskType::Chat : TaskType::GenericEdit;
        }
    }

    // 默认返回Chat
    return TaskType::Chat;  // 【默认】如果没有任何关键词匹配，当作普通聊天
}

/**
 * @brief 构建文件处理任务的System Prompt
 * @param type 任务类型
 * @return 对应类型的System Prompt
 */
QString Orchestrator::buildFileProcessingSystemPrompt(TaskType type) const
{
    switch (type) {  // 【分支】根据任务类型返回对应的系统提示词
    case TaskType::DocumentExpansion:
        return "你是一个专业的技术文档撰写专家。你的任务是根据用户要求和原始文档内容，生成一份完整、详细、专业的扩写版本。"
               "你必须输出完整的文档内容，不能只输出修改部分或差异。保持原文档的结构，但大幅丰富每个章节的内容。"
               "使用 Markdown 格式。直接输出文档内容，不要添加前言或解释。";

    case TaskType::DocumentModification:
        return "你是一个专业的文档编辑专家。你的任务是根据用户要求修改原始文档。"
               "你必须输出完整的修改后文档内容，不能只输出修改部分或差异。"
               "使用 Markdown 格式。直接输出文档内容，不要添加前言或解释。";

    case TaskType::CodeAddition:
        return "你是一个资深的软件工程师。你的任务是在现有代码基础上添加新功能。"
               "你必须输出完整的代码文件内容，不能只输出新增部分或差异。"
               "代码要健壮、有错误处理、符合最佳实践。直接输出完整代码，不要添加解释。";

    case TaskType::CodeFix:
        return "你是一个资深的软件工程师和Bug修复专家。你的任务是修复代码中的问题。"
               "你必须输出完整的修复后代码文件内容，不能只输出修改部分。"
               "修复要彻底，不能引入新问题。添加必要的注释说明修复了什么。直接输出完整代码。";

    case TaskType::CodeRefactor:
        return "你是一个资深的软件架构师。你的任务是对代码进行重构和优化。"
               "你必须输出完整的重构后代码文件内容，不能只输出修改部分。"
               "重构后代码要更清晰、更高效、更易维护。保持原有功能不变。直接输出完整代码。";

    case TaskType::ConfigUpdate:
        return "你是一个系统配置专家。你的任务是根据用户要求更新配置文件。"
               "你必须输出完整的更新后配置文件内容，不能只输出修改部分。"
               "确保配置格式正确、语法无误。直接输出完整配置内容。";

    case TaskType::GenericEdit:
    default:
        return "你是一个文件编辑专家。你的任务是根据用户要求编辑文件内容。"
               "你必须输出完整的编辑后文件内容，不能只输出修改部分或差异。"
               "直接输出完整内容，不要添加前言或解释。";
    }
}

/**
 * @brief 构建文件处理任务的User Prompt
 * @param type 任务类型
 * @param query 用户原始请求
 * @param originalContent 原始文件内容
 * @param fileName 文件名
 * @return 对应类型的User Prompt
 */
QString Orchestrator::buildFileProcessingUserPrompt(TaskType type, const QString& query, const QString& originalContent, const QString& fileName) const
{
    // 【基础模板】定义用户提示词的基本结构
    QString baseTemplate =
        "请根据以下要求处理文件：\n\n"
        "用户要求：%1\n"
        "文件名：%2\n\n"
        "原始文件内容：\n```\n%3\n```\n\n";

    QString suffix;  // 【声明】存储不同类型任务的附加要求
    switch (type) {  // 【分支】根据任务类型设置不同要求
    case TaskType::DocumentExpansion:
        suffix =
            "请输出扩写后的完整文档内容。要求：\n"
            "1. 保持原文档的整体结构和章节顺序\n"
            "2. 每个章节都要大幅丰富内容，添加详细的段落说明\n"
            "3. 内容要专业、具体、有深度\n"
            "4. 直接输出完整文档，不要添加任何解释\n"
            "5. 使用 Markdown 格式";
        break;

    case TaskType::DocumentModification:
        suffix =
            "请输出修改后的完整文档内容。要求：\n"
            "1. 根据用户要求进行修改\n"
            "2. 保持文档整体结构（除非修改要求改变结构）\n"
            "3. 直接输出完整文档，不要添加任何解释\n"
            "4. 使用 Markdown 格式";
        break;

    case TaskType::CodeAddition:
        suffix =
            "请输出添加功能后的完整代码内容。要求：\n"
            "1. 在现有代码基础上添加用户要求的功能\n"
            "2. 保持原有代码结构，不要破坏现有功能\n"
            "3. 新代码要与原有代码风格一致\n"
            "4. 添加必要的错误处理和边界检查\n"
            "5. 直接输出完整代码，不要添加解释";
        break;

    case TaskType::CodeFix:
        suffix =
            "请输出修复后的完整代码内容。要求：\n"
            "1. 修复代码中的所有问题\n"
            "2. 保持原有功能不变\n"
            "3. 添加必要的注释说明修复了什么\n"
            "4. 直接输出完整代码，不要添加解释";
        break;

    case TaskType::CodeRefactor:
        suffix =
            "请输出重构后的完整代码内容。要求：\n"
            "1. 对代码进行重构和优化\n"
            "2. 保持原有功能不变\n"
            "3. 代码要更清晰、更高效、更易维护\n"
            "4. 直接输出完整代码，不要添加解释";
        break;

    case TaskType::ConfigUpdate:
        suffix =
            "请输出更新后的完整配置内容。要求：\n"
            "1. 根据用户要求更新配置\n"
            "2. 保持配置格式正确\n"
            "3. 直接输出完整配置，不要添加解释";
        break;

    case TaskType::GenericEdit:
    default:
        suffix =
            "请输出编辑后的完整文件内容。要求：\n"
            "1. 根据用户要求进行编辑\n"
            "2. 直接输出完整内容，不要添加解释";
        break;
    }

    // 【填充】使用arg方法将参数插入模板，返回最终提示词
    return QString(baseTemplate + suffix).arg(query, fileName, originalContent);
}

/**
 * @brief 通用文件处理任务处理
 * @param query 用户原始请求
 * @param files 用户上传的文件列表
 * @param type 文件处理任务类型
 *
 * @details
 * 通用"读取→LLM处理→写入"工作流：
 * 1. 读取文件内容
 * 2. 根据任务类型构建对应的Prompt
 * 3. 发送给LLM处理
 * 4. LLM返回处理后的完整内容
 * 5. 写入原文件
 *
 * 覆盖所有文件内容变更场景：文档扩写/修改、代码添加功能/修复/重构、配置更新等。
 * 避免7B模型在planning阶段需要生成完整内容的问题。
 */
void Orchestrator::handleFileProcessingTask(const QString& query, const QStringList& files, TaskType type)
{
    if (files.isEmpty()) {  // 【检查】文件处理任务必须有文件
        emit errorOccurred("文件处理任务需要提供文件");  // 【信号】通知UI错误
        return;
    }

    QString filePath = files.first();  // 【获取】取第一个文件（当前只处理单个文件）
    QFileInfo fileInfo(filePath);      // 【创建】获取文件信息

    if (!fileInfo.exists()) {  // 【检查】文件必须存在
        emit errorOccurred("文件不存在: " + filePath);  // 【信号】通知UI错误
        return;
    }

    // 读取文件内容
    QFile file(filePath);  // 【创建】文件对象
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 【尝试】以只读文本模式打开
        emit errorOccurred("无法读取文件: " + filePath);  // 【信号】通知UI错误
        return;
    }

    QString originalContent = QTextStream(&file).readAll();  // 【读取】一次性读取全部文本内容
    file.close();  // 【关闭】关闭文件

    QString typeName;  // 【声明】用于存储任务类型的中文名称
    switch (type) {  // 【分支】将枚举转为中文显示名称
    case TaskType::DocumentExpansion: typeName = "扩写"; break;
    case TaskType::DocumentModification: typeName = "修改"; break;
    case TaskType::CodeAddition: typeName = "添加功能"; break;
    case TaskType::CodeFix: typeName = "修复"; break;
    case TaskType::CodeRefactor: typeName = "重构"; break;
    case TaskType::ConfigUpdate: typeName = "更新配置"; break;
    case TaskType::GenericEdit: typeName = "编辑"; break;
    default: typeName = "处理"; break;
    }

    // 【日志】通知UI文件读取成功和任务类型
    emit logMessage("system", QString("正在读取文件: %1 (%2 字符)").arg(fileInfo.fileName()).arg(originalContent.length()));
    emit logMessage("system", QString("检测到%1任务，正在调用LLM...").arg(typeName));

    // 【构造】根据任务类型生成系统提示词和用户提示词
    QString systemPrompt = buildFileProcessingSystemPrompt(type);
    QString userPrompt = buildFileProcessingUserPrompt(type, query, originalContent, fileInfo.fileName());

    m_isProcessing = true;           // 【设置】标记为处理中
    m_lastWrittenFilePath = filePath; // 【记录】保存文件路径，供后续LLM响应处理使用
    m_lastWrittenContent = originalContent;  // 【记录】保存原始内容

    QJsonArray messages;             // 【创建】消息数组
    QJsonObject systemMsg;           // 【创建】系统消息对象
    systemMsg["role"] = "system";    // 【设置】角色为系统
    systemMsg["content"] = systemPrompt;  // 【设置】系统提示词
    messages.append(systemMsg);      // 【追加】添加系统消息

    QJsonObject userMsg;             // 【创建】用户消息对象
    userMsg["role"] = "user";        // 【设置】角色为用户
    userMsg["content"] = userPrompt; // 【设置】用户提示词（包含原始文件内容）
    messages.append(userMsg);        // 【追加】添加用户消息

    m_llmClient->sendMessages(messages, false);  // 【发送】调用LLM处理文件内容
}
