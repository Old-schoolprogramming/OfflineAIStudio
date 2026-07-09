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

#include "orchestrator.h"
#include "agent.h"
#include "tool.h"
#include <QDebug>

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
    : Agent(parent),
      m_llmClient(nullptr),
      m_scheduler(new TaskScheduler(this)),
      m_conversationManager(new ConversationManager(this)),
      m_isProcessing(false),
      m_isPlanningPhase(false),
      m_retryCount(0)
{
    connect(m_scheduler, &TaskScheduler::stepStarted,
            this, &Orchestrator::onSchedulerStepStarted);
    connect(m_scheduler, &TaskScheduler::stepOutput,
            this, &Orchestrator::onSchedulerStepOutput);
    connect(m_scheduler, &TaskScheduler::stepCompleted,
            this, &Orchestrator::onSchedulerStepCompleted);
    connect(m_scheduler, &TaskScheduler::stepFailed,
            this, &Orchestrator::onSchedulerStepFailed);
    connect(m_scheduler, &TaskScheduler::planCompleted,
            this, &Orchestrator::onSchedulerPlanCompleted);
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
}

/**
 * @brief 获取 Agent 名称
 * @return "Orchestrator"
 */
QString Orchestrator::name() const
{
    return "Orchestrator";
}

/**
 * @brief 获取 Agent 描述
 * @return 描述字符串
 */
QString Orchestrator::description() const
{
    return "总控Agent，负责生成执行计划并协调多Agent协作";
}

/**
 * @brief 获取 Agent 类型
 * @return OrchestratorType
 */
Agent::AgentType Orchestrator::type() const
{
    return OrchestratorType;
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
    QList<Tool*> allTools;
    for (Agent* agent : m_agents) {
        allTools.append(agent->tools());
    }
    return allTools;
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
    for (Agent* agent : m_agents) {
        QList<Tool*> tools = agent->tools();
        for (Tool* tool : tools) {
            if (tool->name() == toolName) {
                return agent->executeTool(toolName, args);
            }
        }
    }

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Tool not found: " + toolName;
    return result;
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
    m_llmClient = client;
    connect(m_llmClient, &LlmClient::responseReceived, this, &Orchestrator::onLlmResponse);
    connect(m_llmClient, &LlmClient::responseError, this, &Orchestrator::onLlmError);
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
    m_agents.append(agent);
    m_scheduler->registerAgent(agent);
}

/**
 * @brief 判断当前是否正在处理请求
 * @return true 当处理中
 */
bool Orchestrator::isProcessing() const
{
    return m_isProcessing;
}

/**
 * @brief 获取当前执行计划的副本
 * @return 当前 TaskPlan
 */
TaskPlan Orchestrator::currentPlan() const
{
    return m_scheduler->currentPlan();
}

QJsonArray Orchestrator::conversationHistory() const
{
    return m_conversationManager->toJson();
}

void Orchestrator::loadConversationHistory(const QJsonArray& history)
{
    m_conversationManager->loadFromJson(history);
}

void Orchestrator::switchConversation(int index)
{
    m_conversationManager->switchConversation(index);
}

int Orchestrator::currentConversationIndex() const
{
    return m_conversationManager->currentConversationIndex();
}

void Orchestrator::createNewConversation()
{
    m_conversationManager->createNewConversation();
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
    if (!m_llmClient || m_isProcessing) {
        return;
    }

    m_isProcessing = true;
    m_isPlanningPhase = true;
    m_currentQuery = query;

    m_conversationManager->addUserMessage(query);

    sendPlanningRequest(query);
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
    if (m_scheduler->isRunning()) {
        m_scheduler->stopExecution();
    }
    m_isProcessing = false;
    m_isPlanningPhase = false;
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
    QString systemPrompt = m_promptBuilder.buildPlanningSystemPrompt(m_agents);
    QString userPrompt = m_promptBuilder.buildPlanningUserPrompt(query);

    QJsonArray messages = m_conversationManager->buildMessageHistory(systemPrompt);

    if (!messages.isEmpty()) {
        QJsonObject lastMsg = messages.last().toObject();
        if (lastMsg["role"].toString() == "user") {
            QJsonObject userMsg;
            userMsg["role"] = "user";
            userMsg["content"] = query;
            messages.replace(messages.size() - 1, userMsg);
        } else {
            QJsonObject userMsg;
            userMsg["role"] = "user";
            userMsg["content"] = query;
            messages.append(userMsg);
        }
    } else {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = systemPrompt;
        messages.append(systemMsg);

        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = query;
        messages.append(userMsg);
    }

    m_llmClient->sendMessages(messages, false);
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
    emit responseChunkReceived(chunk);
    emit logMessage("llm", chunk);
}

void Orchestrator::onLlmResponse(const QString& response)
{
    if (m_isPlanningPhase) {
        emit logMessage("system", "LLM 响应已收到，正在解析...");
        emit logMessage("llm", "LLM原始响应: " + response.left(200) + (response.length() > 200 ? "..." : ""));

        ParsedResponse parsed = m_planner.parseResponse(response);

        if (parsed.type == ParsedResponse::Chat) {
            m_conversationManager->addAssistantMessage(parsed.chatResponse);
            emit messageReceived(parsed.chatResponse);
            m_isProcessing = false;
            m_isPlanningPhase = false;
            return;
        }

        if (parsed.type == ParsedResponse::Plan && !parsed.plan.steps.isEmpty()) {
            parsed.plan.goal = m_currentQuery;
            emit planGenerated(parsed.plan);

            emit logMessage("system", QString("计划生成成功，共 %1 个步骤").arg(parsed.plan.steps.size()));
            for (const auto& step : parsed.plan.steps) {
                emit logMessage("agent", QString("步骤 %1: %2.%3").arg(step.stepId).arg(step.agent).arg(step.tool));
            }

            m_scheduler->setPlan(parsed.plan);
            m_isPlanningPhase = false;
            m_scheduler->startExecution();
            return;
        }

        // 兜底：不是有效 chat 也不是有效 plan，当纯文本展示
        m_conversationManager->addAssistantMessage(response);
        emit messageReceived(response);
        m_isProcessing = false;
        m_isPlanningPhase = false;
    } else {
        m_conversationManager->addAssistantMessage(response);
        emit messageReceived(response);
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
    emit errorOccurred("LLM错误: " + error);
    m_isProcessing = false;
    m_isPlanningPhase = false;
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
    emit stepStarted(stepId);
    emit logMessage("execution", QString("步骤 %1 开始执行").arg(stepId));
}

/**
 * @brief 透传：步骤输出
 * @param stepId 步骤编号
 * @param output 输出文本
 */
void Orchestrator::onSchedulerStepOutput(int stepId, const QString& output)
{
    emit stepOutput(stepId, output);
    emit logMessage("execution", QString("步骤 %1 输出: %2").arg(stepId).arg(output.left(100)));
}

/**
 * @brief 透传：步骤完成
 * @param stepId 步骤编号
 * @param result 执行结果
 */
void Orchestrator::onSchedulerStepCompleted(int stepId, const QVariantMap& result)
{
    emit stepCompleted(stepId, result);
    bool success = result.value("success", false).toBool();
    if (success) {
        emit logMessage("execution", QString("步骤 %1 执行成功").arg(stepId));
    } else {
        emit logMessage("execution", QString("步骤 %1 执行失败: %2").arg(stepId).arg(result.value("error", "未知错误").toString()));
    }
}

/**
 * @brief 透传：步骤失败
 * @param stepId 步骤编号
 * @param error 错误信息
 */
void Orchestrator::onSchedulerStepFailed(int stepId, const QString& error)
{
    emit stepFailed(stepId, error);
    emit logMessage("execution", QString("步骤 %1 失败: %2").arg(stepId).arg(error));
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
    emit planCompleted(plan);
    m_isProcessing = false;

    int failCount = 0;
    for (const auto& step : plan.steps) {
        if (step.status == StepStatus::Failed) failCount++;
    }

    // 反思机制：plan 完全失败时，尝试一次反思重试
    if (failCount > 0 && failCount == plan.steps.size() && m_retryCount < MAX_RETRY) {
        m_retryCount++;
        retryPlanWithFeedback(plan);
        return;
    }

    m_retryCount = 0;
    generateSummary(plan);
}

void Orchestrator::retryPlanWithFeedback(const TaskPlan& failedPlan)
{
    // 收集所有失败步骤的错误信息
    QStringList failures;
    for (const auto& step : failedPlan.steps) {
        if (step.status == StepStatus::Failed) {
            failures << QString("- 步骤 %1 (%2): %3")
                .arg(step.stepId)
                .arg(step.description)
                .arg(step.error.isEmpty() ? "未知错误" : step.error);
        }
    }

    QString feedback = QString(
        "上次的执行计划全部失败，请分析错误并生成新的计划。\n"
        "用户原始需求：%1\n\n"
        "失败原因：\n%2\n\n"
        "请使用合适的工具（如 writeFile 而不是 echo+shell），生成新计划。"
    ).arg(m_currentQuery, failures.join("\n"));

    m_conversationManager->addUserMessage(feedback);
    m_isProcessing = true;
    m_isPlanningPhase = true;
    sendPlanningRequest(feedback);
}

void Orchestrator::onSchedulerExecutionStopped()
{
    m_isProcessing = false;
    m_isPlanningPhase = false;
    m_retryCount = 0;
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
    int successCount = 0;
    int failCount = 0;
    for (const auto& step : plan.steps) {
        if (step.status == StepStatus::Completed) successCount++;
        else if (step.status == StepStatus::Failed) failCount++;
    }

    QString summary = QString("任务执行完成。共 %1 个步骤，成功 %2 个，失败 %3 个。")
        .arg(plan.totalSteps()).arg(successCount).arg(failCount);

    m_conversationManager->addAssistantMessage(summary);
    emit messageReceived(summary);
}
