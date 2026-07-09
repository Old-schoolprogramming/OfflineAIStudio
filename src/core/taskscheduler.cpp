/**
 * @file taskscheduler.cpp
 * @brief C++ 任务调度引擎实现
 *
 * @details
 * TaskScheduler 是本系统的核心执行引擎。它通过递归调用 executeNextStep() 实现
 * 顺序执行，利用 Qt 事件循环的自动调度避免阻塞 UI。
 *
 * 关键设计决策：
 * 1. 递归而非循环：使用递归调用 executeNextStep()，每次步骤执行完成后
 *    通过 Qt 事件循环将下一次调用排入队列，UI 有机会更新
 * 2. 同步执行：Agent::executeTool() 在当前线程同步调用，简化状态管理
 * 3. 失败继续：单个步骤失败不影响后续步骤，最大化任务完成度
 * 4. 按工具路由：通过工具名称查找 Agent，而非按 Agent 名称，降低 LLM 认知负担
 */

#include "taskscheduler.h"
#include "tool.h"
#include <QDateTime>
#include <QDebug>

/**
 * @brief 构造函数
 * @param parent 父 QObject
 *
 * 初始化所有内部状态为默认值：
 * - m_currentStepIndex = 0: 尚未开始执行任何步骤
 * - m_isRunning = false: 当前没有计划正在执行
 * - m_shouldStop = false: 用户没有请求停止
 */
TaskScheduler::TaskScheduler(QObject *parent)
    : QObject(parent),
      m_currentStepIndex(0),
      m_isRunning(false),
      m_shouldStop(false),
      m_maxRetries(2)
{
}

/**
 * @brief 析构函数
 *
 * @note TaskScheduler 不拥有 Agent 的所有权，析构时不会 delete 已注册的 Agent。
 *       Agent 的生命周期由创建者（通常是 MainWindow 或 Orchestrator）管理。
 */
TaskScheduler::~TaskScheduler()
{
}

/**
 * @brief 注册一个 Agent 到调度系统
 * @param agent 要注册的 Agent 实例指针
 *
 * @implementation
 * 简单地将 Agent 指针添加到 m_agents 列表末尾。
 * 后续 findAgentForTool() 会按注册顺序查找第一个拥有目标工具的 Agent。
 */
void TaskScheduler::registerAgent(Agent* agent)
{
    m_agents.append(agent);
}

/**
 * @brief 设置当前要执行的计划
 * @param plan 由 Planner 解析的 TaskPlan
 *
 * @implementation
 * 1. 复制 plan 到 m_currentPlan（深拷贝，因为 TaskPlan 不含指针）
 * 2. 重置步骤索引为 0
 * 3. 发射 planReceived 信号，通知 UI 层初始化任务列表
 */
void TaskScheduler::setPlan(const TaskPlan& plan)
{
    m_currentPlan = plan;
    m_currentStepIndex = 0;
    emit planReceived(m_currentPlan);
}

/**
 * @brief 获取当前计划的副本
 * @return 当前执行计划的副本（包含最新的步骤状态）
 *
 * @note 返回副本而非引用，防止外部代码意外修改调度器内部状态。
 *       Orchestrator 通过此方法向 UI 暴露当前计划信息（如步骤描述查询）。
 */
TaskPlan TaskScheduler::currentPlan() const
{
    return m_currentPlan;
}

/**
 * @brief 判断当前是否正在执行计划
 * @return true 当有执行中的计划
 */
bool TaskScheduler::isRunning() const
{
    return m_isRunning;
}

/**
 * @brief 设置全局默认最大重试次数
 * @param count 最大重试次数
 */
void TaskScheduler::setMaxRetries(int count)
{
    m_maxRetries = qMax(0, count);
}

/**
 * @brief 获取全局默认最大重试次数
 * @return 最大重试次数
 */
int TaskScheduler::maxRetries() const
{
    return m_maxRetries;
}

/**
 * @brief 启动执行当前设置的计划
 *
 * @implementation
 * 启动前检查两个条件：
 * 1. !m_isRunning — 防止重复启动
 * 2. !m_currentPlan.steps.isEmpty() — 空计划无需执行
 *
 * 满足条件后：
 * - 设置 m_isRunning = true
 * - 清除 m_shouldStop 标志
 * - 重置步骤索引为 0
 * - 调用 executeNextStep() 开始执行第一个步骤
 */
void TaskScheduler::startExecution()
{
    if (m_isRunning || m_currentPlan.steps.isEmpty()) {
        return;
    }

    m_isRunning = true;
    m_shouldStop = false;
    m_currentStepIndex = 0;

    executeNextStep();
}

/**
 * @brief 停止执行后续步骤
 *
 * @implementation
 * 设置 m_shouldStop = true，标记 m_isRunning = false，发射 executionStopped 信号。
 *
 * @note 此方法不会中断正在执行的步骤（因为步骤执行是同步的），
 *       只会在当前步骤完成后，通过 m_shouldStop 标志阻止下一步的执行。
 *       这是"合作式中断"模式，而非"强制中断"。
 */
void TaskScheduler::stopExecution()
{
    m_shouldStop = true;
    m_isRunning = false;
    emit executionStopped();
}

/**
 * @brief 执行下一个步骤（核心调度逻辑）
 *
 * @implementation
 * 这是调度引擎的心脏，采用递归方式实现顺序执行。
 * 每次调用处理一个步骤，处理完毕后递归调用自身处理下一个。
 *
 * 增强的协作能力：
 * 1. 失败重试：步骤失败后自动重试（受maxRetries限制）
 * 2. 多Agent备份：首选Agent失败后，尝试其他拥有相同工具的Agent
 * 3. 上下文传递：前一步的结果可作为后一步的context参数
 *
 * 步骤处理流程：
 * 1. 终止检查：如果 m_shouldStop 或索引越界，标记完成并返回
 * 2. 状态更新：将当前步骤设为 Running，记录开始时间
 * 3. 上下文注入：如果useContext为true，将前一步结果注入参数
 * 4. 通知 UI：发射 stepStarted + stepOutput（日志头）
 * 5. Agent 查找：通过 findAgentForTool() 查找拥有该工具的 Agent
 * 6. 找不到 Agent：标记为 Failed，填充 error，发射 stepFailed，递归下一步
 * 7. 找到 Agent：调用 executeTool() 同步执行
 * 8. 结果处理：
 *    - success=true → Completed，填充 output 和 endTime，发射 stepCompleted
 *    - success=false → 检查是否可重试/切换Agent，不行则Failed
 * 9. 递归：递增索引，调用 executeNextStep()
 *
 * 递归终止条件：
 * - m_shouldStop 为 true（用户停止）
 * - m_currentStepIndex >= steps.size()（全部执行完毕）
 */
void TaskScheduler::executeNextStep()
{
    // 终止条件1：用户请求停止
    // 终止条件2：所有步骤已执行完毕
    if (m_shouldStop || m_currentStepIndex >= m_currentPlan.steps.size()) {
        m_isRunning = false;
        emit planCompleted(m_currentPlan);
        return;
    }

    // 取出当前步骤的引用（注意：这里使用引用是为了就地修改状态）
    TaskStep& step = m_currentPlan.steps[m_currentStepIndex];

    // 如果步骤已经完成（可能是重试后的状态），跳过
    if (step.status == StepStatus::Completed) {
        m_currentStepIndex++;
        executeNextStep();
        return;
    }

    // 更新步骤状态为 Running，记录开始时间
    step.status = StepStatus::Running;
    step.startTime = QDateTime::currentDateTime();

    // 获取前一步的输出（用于上下文传递）
    QString previousOutput;
    if (m_currentStepIndex > 0) {
        previousOutput = m_currentPlan.steps[m_currentStepIndex - 1].output;
    }

    // 如果启用了上下文传递，将前一步结果注入当前步骤参数
    if (step.useContext && !previousOutput.isEmpty()) {
        injectContext(step, previousOutput);
    }

    // 如果是重试，输出重试提示
    if (step.retryCount > 0) {
        emit stepOutput(step.stepId, QString("=== 第 %1 次重试 ===").arg(step.retryCount));
    }

    // 通知 UI：步骤开始执行
    emit stepStarted(step.stepId);
    // 通知 UI：输出步骤头信息（包含 Agent、Tool、参数等）
    emit stepOutput(step.stepId, buildStepLogHeader(step));

    // 查找所有能执行该工具的Agent（用于多Agent协作备份）
    QList<Agent*> candidateAgents = findAllAgentsForTool(step.tool);

    // 场景A：找不到能执行该工具的 Agent
    if (candidateAgents.isEmpty()) {
        step.status = StepStatus::Failed;
        step.error = "找不到执行工具的Agent: " + step.tool;
        step.endTime = QDateTime::currentDateTime();
        emit stepOutput(step.stepId, "错误: " + step.error);
        emit stepFailed(step.stepId, step.error);
        m_currentStepIndex++;
        executeNextStep();
        return;
    }

    // 确定实际使用的Agent索引（重试时尝试下一个Agent）
    int agentIndex = qMin(step.retryCount, candidateAgents.size() - 1);
    Agent* agent = candidateAgents[agentIndex];

    // 如果切换了Agent，输出提示
    if (agentIndex > 0) {
        emit stepOutput(step.stepId, QString("尝试使用备用Agent: %1").arg(agent->name()));
    }

    // 场景B：找到 Agent，同步执行工具
    QVariantMap result = agent->executeTool(step.tool, step.args);

    // 根据 Agent 返回的结果更新步骤状态
    if (result.value("success", false).toBool()) {
        // 执行成功
        step.status = StepStatus::Completed;
        step.output = result.value("result", "").toString();
        step.endTime = QDateTime::currentDateTime();
        emit stepOutput(step.stepId, "输出:\n" + step.output);
        emit stepCompleted(step.stepId, result);
    } else {
        // 执行失败，检查是否可以重试或切换Agent
        int maxRet = step.maxRetries > 0 ? step.maxRetries : m_maxRetries;
        bool canRetry = step.retryCount < maxRet;
        bool hasAltAgent = agentIndex + 1 < candidateAgents.size();

        if (canRetry || hasAltAgent) {
            // 可以重试或切换Agent
            step.retryCount++;
            QString errorMsg = result.value("error", "Unknown error").toString();
            emit stepOutput(step.stepId, QString("失败: %1（即将重试...）").arg(errorMsg));

            // 重置状态为Pending，重新执行当前步骤
            step.status = StepStatus::Pending;
            step.error = "";

            // 延迟一点再重试（让UI有机会更新）
            // 通过递归调用立即重试（简化实现）
            executeNextStep();
            return;
        }

        // 无法重试，标记为最终失败
        step.status = StepStatus::Failed;
        step.error = result.value("error", "Unknown error").toString();
        step.endTime = QDateTime::currentDateTime();
        emit stepOutput(step.stepId, "失败: " + step.error);
        emit stepFailed(step.stepId, step.error);
    }

    // 递增步骤索引，递归执行下一步
    // Qt 事件循环会在当前函数返回后处理信号槽，UI 因此有机会更新
    m_currentStepIndex++;
    executeNextStep();
}

/**
 * @brief 按 Agent 名称查找已注册的 Agent
 * @param agentName 要查找的 Agent 名称
 * @return Agent 指针，找不到返回 nullptr
 *
 * @implementation
 * 遍历 m_agents 列表，使用不区分大小写的字符串比较匹配 name()。
 *
 * @note 当前未被主调度逻辑使用，保留为未来扩展（如 LLM 明确指定 Agent 时）。
 */
Agent* TaskScheduler::findAgentByName(const QString& agentName)
{
    for (Agent* agent : m_agents) {
        if (agent->name().compare(agentName, Qt::CaseInsensitive) == 0) {
            return agent;
        }
    }
    return nullptr;
}

/**
 * @brief 按工具名称查找拥有该工具的 Agent
 * @param toolName 工具名称
 * @return Agent 指针，找不到返回 nullptr
 *
 * @implementation
 * 遍历所有已注册的 Agent：
 * 1. 对每个 Agent 调用 tools() 获取其工具列表
 * 2. 遍历工具列表，比较 tool->name() 与 toolName（忽略大小写）
 * 3. 返回第一个匹配的 Agent
 *
 * 时间复杂度：O(N * M)，N 为 Agent 数量，M 为每个 Agent 的平均工具数量。
 * 由于 N 和 M 都很小（通常 < 10），实际性能可忽略。
 *
 * @note 如果多个 Agent 拥有同名工具，返回第一个匹配的。
 *       建议各 Agent 的工具名称保持全局唯一以避免歧义。
 */
Agent* TaskScheduler::findAgentForTool(const QString& toolName)
{
    for (Agent* agent : m_agents) {
        QList<Tool*> tools = agent->tools();
        for (Tool* tool : tools) {
            if (tool->name().compare(toolName, Qt::CaseInsensitive) == 0) {
                return agent;
            }
        }
    }
    return nullptr;
}

/**
 * @brief 构建步骤执行的日志头信息
 * @param step 当前步骤
 * @return 格式化的多行字符串，用于 UI 输出面板展示
 *
 * @implementation
 * 生成的格式示例：
 * === 步骤 1: 创建项目目录 ===
 * Agent: ComputerAgent | Tool: runCommand
 * 参数: command=mkdir D:\project
 *
 * @note 此字符串通过 stepOutput 信号发送到 OutputPanel，展示在输出面板中。
 */
QString TaskScheduler::buildStepLogHeader(const TaskStep& step)
{
    QString header = QString("=== 步骤 %1: %2 ===\n").arg(step.stepId).arg(step.description);
    header += QString("Agent: %1 | Tool: %2\n").arg(step.agent, step.tool);

    // 如果有参数，格式化输出参数列表
    if (!step.args.isEmpty()) {
        header += "参数: ";
        QStringList argList;
        for (auto it = step.args.begin(); it != step.args.end(); ++it) {
            argList.append(QString("%1=%2").arg(it.key(), it.value().toString()));
        }
        header += argList.join(", ") + "\n";
    }

    return header;
}

/**
 * @brief 查找所有拥有指定工具的Agent
 * @param toolName 工具名称
 * @return Agent指针列表
 *
 * @details
 * 遍历所有已注册的Agent，收集所有拥有该工具的Agent。
 * 用于失败时切换到备用Agent，提升系统的容错能力。
 */
QList<Agent*> TaskScheduler::findAllAgentsForTool(const QString& toolName)
{
    QList<Agent*> result;
    for (Agent* agent : m_agents) {
        QList<Tool*> tools = agent->tools();
        for (Tool* tool : tools) {
            if (tool->name().compare(toolName, Qt::CaseInsensitive) == 0) {
                result.append(agent);
                break;
            }
        }
    }
    return result;
}

/**
 * @brief 将前一步的结果注入到当前步骤的参数中
 * @param step 当前步骤（会被修改）
 * @param previousOutput 前一步的输出结果
 *
 * @details
 * 实现步骤间的上下文传递机制。将前一步的输出结果
 * 作为"context"参数注入到当前步骤的args中。
 * 这样后续步骤可以利用前面步骤的结果进行处理。
 */
void TaskScheduler::injectContext(TaskStep& step, const QString& previousOutput)
{
    step.args["context"] = previousOutput;
}
