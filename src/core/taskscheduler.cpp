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

#include "taskscheduler.h"  // 【引入】自己的头文件
#include "tool.h"           // 【引入】工具基类头文件
#include <QDateTime>        // 【引入】Qt日期时间类，用于记录执行时间
#include <QDebug>           // 【引入】Qt调试输出类

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
    : QObject(parent),            // 【初始化】调用父类构造函数
      m_currentStepIndex(0),      // 【初始化】步骤索引从0开始
      m_isRunning(false),         // 【初始化】初始不在执行中
      m_shouldStop(false)         // 【初始化】初始未请求停止
{
    // 【说明】构造函数体为空，所有初始化在初始化列表完成
}

/**
 * @brief 析构函数
 *
 * @note TaskScheduler 不拥有 Agent 的所有权，析构时不会 delete 已注册的 Agent。
 *       Agent 的生命周期由创建者（通常是 MainWindow 或 Orchestrator）管理。
 */
TaskScheduler::~TaskScheduler()
{
    // 【说明】m_agents列表中的指针由外部管理，此处不释放
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
    m_agents.append(agent);  // 【添加】将Agent指针追加到列表
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
    m_currentPlan = plan;         // 【复制】保存计划副本
    m_currentStepIndex = 0;       // 【重置】步骤索引归零
    emit planReceived(m_currentPlan);  // 【信号】通知UI收到新计划
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
    return m_currentPlan;  // 【返回】计划副本
}

/**
 * @brief 判断当前是否正在执行计划
 * @return true 当有执行中的计划
 */
bool TaskScheduler::isRunning() const
{
    return m_isRunning;  // 【返回】执行状态标志
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
    if (m_isRunning || m_currentPlan.steps.isEmpty()) {  // 【前置检查】已在运行或计划为空
        return;  // 【返回】不启动
    }

    m_isRunning = true;        // 【设置】标记为运行中
    m_shouldStop = false;      // 【重置】清除停止标志
    m_currentStepIndex = 0;    // 【重置】步骤索引归零

    executeNextStep();         // 【调用】开始执行第一个步骤
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
    m_shouldStop = true;       // 【设置】置停止标志
    m_isRunning = false;       // 【设置】标记为不在运行
    emit executionStopped();   // 【信号】通知UI执行已停止
}

/**
 * @brief 执行下一个步骤（核心调度逻辑）
 *
 * @implementation
 * 这是调度引擎的心脏，采用递归方式实现顺序执行。
 * 每次调用处理一个步骤，处理完毕后递归调用自身处理下一个。
 *
 * 步骤处理流程：
 * 1. 终止检查：如果 m_shouldStop 或索引越界，标记完成并返回
 * 2. 状态更新：将当前步骤设为 Running，填充 startTime
 * 3. 通知 UI：发射 stepStarted + stepOutput（日志头）
 * 4. Agent 查找：通过 findAgentForTool() 查找拥有该工具的 Agent
 * 5. 找不到 Agent：标记为 Failed，填充 error，发射 stepFailed，递归下一步
 * 6. 找到 Agent：调用 executeTool() 同步执行
 * 7. 结果处理：
 *    - success=true → Completed，填充 output 和 endTime，发射 stepCompleted
 *    - success=false → Failed，填充 error 和 endTime，发射 stepFailed
 * 8. 递归：递增索引，调用 executeNextStep()
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
        m_isRunning = false;           // 【设置】标记为不在运行
        emit planCompleted(m_currentPlan);  // 【信号】通知UI计划执行完毕
        return;                        // 【返回】递归终止
    }

    // 取出当前步骤的引用（注意：这里使用引用是为了就地修改状态）
    TaskStep& step = m_currentPlan.steps[m_currentStepIndex];  // 【获取】当前步骤的引用

    // 更新步骤状态为 Running，记录开始时间
    step.status = StepStatus::Running;                          // 【设置】状态为运行中
    step.startTime = QDateTime::currentDateTime();              // 【记录】当前时间作为开始时间

    // 通知 UI：步骤开始执行
    emit stepStarted(step.stepId);                              // 【信号】通知UI步骤开始
    // 通知 UI：输出步骤头信息（包含 Agent、Tool、参数等）
    emit stepOutput(step.stepId, buildStepLogHeader(step));     // 【信号】发送步骤头信息到输出面板

    // 按工具名称查找拥有该工具的 Agent
    Agent* agent = findAgentForTool(step.tool);                 // 【查找】找到能执行该工具的Agent

    // 场景A：找不到能执行该工具的 Agent
    if (!agent) {
        step.status = StepStatus::Failed;                       // 【设置】状态为失败
        step.error = "找不到执行工具的Agent: " + step.tool;       // 【设置】错误信息
        step.endTime = QDateTime::currentDateTime();            // 【记录】结束时间
        emit stepOutput(step.stepId, "错误: " + step.error);     // 【信号】输出错误信息
        emit stepFailed(step.stepId, step.error);               // 【信号】通知UI步骤失败
        m_currentStepIndex++;                                   // 【递增】步骤索引+1
        executeNextStep();                                      // 【递归】执行下一步
        return;
    }

    emit stepOutput(step.stepId, QString("调用 Agent: %1").arg(agent->name()));  // 【信号】输出调用的Agent名称

    // 场景B：找到 Agent，同步执行工具
    QVariantMap result = agent->executeTool(step.tool, step.args);  // 【执行】调用Agent的工具

    // 根据 Agent 返回的结果更新步骤状态
    if (result.value("success", false).toBool()) {              // 【判断】执行是否成功
        // 执行成功
        step.status = StepStatus::Completed;                    // 【设置】状态为已完成
        step.output = result.value("result", "").toString();    // 【保存】执行输出结果
        step.endTime = QDateTime::currentDateTime();            // 【记录】结束时间
        emit stepOutput(step.stepId, "输出:\n" + step.output);   // 【信号】输出执行结果
        emit stepCompleted(step.stepId, result);                // 【信号】通知UI步骤成功完成
    } else {
        // 执行失败
        step.status = StepStatus::Failed;                       // 【设置】状态为失败
        step.error = result.value("error", "Unknown error").toString();  // 【保存】错误信息
        step.endTime = QDateTime::currentDateTime();            // 【记录】结束时间
        emit stepOutput(step.stepId, "失败: " + step.error);     // 【信号】输出失败信息
        emit stepFailed(step.stepId, step.error);               // 【信号】通知UI步骤失败
    }

    // 递增步骤索引，递归执行下一步
    // Qt 事件循环会在当前函数返回后处理信号槽，UI 因此有机会更新
    m_currentStepIndex++;                                       // 【递增】步骤索引+1
    executeNextStep();                                          // 【递归】执行下一步骤
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
    for (Agent* agent : m_agents) {  // 【遍历】所有已注册的Agent
        if (agent->name().compare(agentName, Qt::CaseInsensitive) == 0) {  // 【比较】不区分大小写匹配名称
            return agent;  // 【返回】找到匹配的Agent
        }
    }
    return nullptr;  // 【返回】未找到
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
    for (Agent* agent : m_agents) {              // 【遍历】所有已注册的Agent
        QList<Tool*> tools = agent->tools();      // 【获取】当前Agent的工具列表
        for (Tool* tool : tools) {                // 【遍历】当前Agent的每个工具
            if (tool->name().compare(toolName, Qt::CaseInsensitive) == 0) {  // 【比较】不区分大小写匹配工具名
                return agent;                     // 【返回】找到拥有该工具的Agent
            }
        }
    }
    return nullptr;  // 【返回】未找到拥有该工具的Agent
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
    // 【构造】步骤标题行：步骤编号 + 描述
    QString header = QString("=== 步骤 %1: %2 ===\n").arg(step.stepId).arg(step.description);
    // 【构造】Agent和工具信息行
    header += QString("Agent: %1 | Tool: %2\n").arg(step.agent, step.tool);

    // 如果有参数，格式化输出参数列表
    if (!step.args.isEmpty()) {  // 【判断】是否有参数
        header += "参数: ";
        QStringList argList;     // 【创建】参数字符串列表
        for (auto it = step.args.begin(); it != step.args.end(); ++it) {  // 【遍历】所有参数
            argList.append(QString("%1=%2").arg(it.key(), it.value().toString()));  // 【格式化】键=值
        }
        header += argList.join(", ") + "\n";  // 【拼接】用逗号连接所有参数
    }

    return header;  // 【返回】格式化后的日志头
}
