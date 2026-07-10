/**
 * @file taskscheduler.h
 * @brief C++ 任务调度引擎 —— Multi-Agent 执行的核心调度器
 *
 * @details
 * TaskScheduler 是整个 Multi-Agent 架构的"执行引擎"。
 *
 * 它的核心职责是：接收一个由大模型生成的 TaskPlan，按顺序执行每一个步骤，
 * 将工具调用路由到正确的 Agent，收集执行结果，并通过 Qt 信号通知 UI 层。
 *
 * 调度流程：
 * 1. 注册所有可用的 Agent（registerAgent）
 * 2. 接收执行计划（setPlan）
 * 3. 启动执行（startExecution）
 * 4. 逐个步骤执行（executeNextStep）
 *    a. 更新步骤状态为 Running
 *    b. 查找拥有该工具的 Agent
 *    c. 调用 Agent::executeTool()
 *    d. 根据返回结果更新状态（Completed / Failed）
 *    e. 发射信号通知 UI
 *    f. 递归调用 executeNextStep() 执行下一步
 * 5. 全部完成后发射 planCompleted
 *
 * 执行特点：
 * - 同步执行：每个步骤在当前线程同步执行（利用 Qt 事件循环避免阻塞 UI）
 * - 顺序执行：严格按 step_id 顺序，不考虑并行（为后续 DAG 扩展预留）
 * - 可中断：支持 stopExecution() 立即停止后续步骤
 * - 失败继续：单个步骤失败不会终止整个计划，后续步骤仍会继续执行
 */

#ifndef TASKSCHEDULER_H  // 【条件编译】防止头文件被重复包含
#define TASKSCHEDULER_H  // 【宏定义】标记该文件已被包含

#include "task.h"    // 【引入】任务数据结构头文件（TaskPlan, TaskStep, StepStatus）
#include "agent.h"   // 【引入】Agent基类头文件
#include <QObject>   // 【引入】Qt对象基类
#include <QList>     // 【引入】Qt列表容器
#include <QString>   // 【引入】Qt字符串类

/**
 * @brief C++ 任务调度引擎
 *
 * TaskScheduler 继承自 QObject，使用 Qt 的信号槽机制与 UI 层通信。
 * 它是纯调度逻辑，不直接与用户交互，所有状态变更都通过信号发射。
 *
 * 线程模型：
 * - 调度逻辑在当前线程（通常是 GUI 线程）同步执行
 * - 每个步骤的 Agent 执行也是同步的
 * - UI 更新通过信号槽自动排队，不会阻塞界面
 *
 * @note 如果 Agent 的执行可能耗时较长（如大文件读写、复杂命令），
 *       后续可以考虑将 Agent 执行移至独立工作线程。
 */
class TaskScheduler : public QObject  // 【类声明】继承QObject以使用信号槽
{
    Q_OBJECT  // 【Qt宏】启用元对象系统

public:
    /**
     * @brief 构造函数
     * @param parent 父 QObject，用于 Qt 内存管理
     *
     * 初始化所有内部状态：清空 Agent 列表、重置步骤索引、标记为未运行。
     */
    explicit TaskScheduler(QObject *parent = nullptr);  // 【构造函数】explicit防止隐式转换

    /**
     * @brief 析构函数
     */
    ~TaskScheduler();  // 【析构函数】释放资源

    /**
     * @brief 注册一个可用的 Agent
     * @param agent Agent 实例指针。TaskScheduler 不接管所有权（不 delete）。
     *
     * @details
     * 注册的 Agent 会被纳入调度系统的工具路由表中。
     * 当执行某个步骤时，TaskScheduler 会遍历所有注册的 Agent，
     * 查找拥有该步骤 tool 名称的 Agent 来执行。
     *
     * @note 通常在应用启动时由 Orchestrator 调用，将所有 Agent 注册进来。
     */
    void registerAgent(Agent* agent);  // 【方法】注册Agent到调度系统

    /**
     * @brief 设置当前要执行的计划
     * @param plan 由 Planner 解析生成的 TaskPlan
     *
     * @details
     * 设置计划后会重置步骤索引为 0，并发射 planReceived 信号。
     * 此时计划尚未开始执行，需要调用 startExecution() 启动。
     */
    void setPlan(const TaskPlan& plan);  // 【方法】设置执行计划

    /**
     * @brief 启动执行当前计划
     *
     * @details
     * 只有在满足以下条件时才会启动：
     * 1. 当前没有在运行其他计划（!m_isRunning）
     * 2. 当前计划非空（m_currentPlan.steps 不为空）
     *
     * 启动后会重置步骤索引为 0，标记为运行中，然后调用 executeNextStep()。
     */
    void startExecution();  // 【方法】启动计划执行

    /**
     * @brief 停止执行
     *
     * @details
     * 设置 m_shouldStop 标志，标记 m_isRunning 为 false，
     * 并发射 executionStopped 信号。
     *
     * 注意：此方法不会中断正在执行的步骤（因为步骤在同步执行），
     * 只会阻止后续步骤的执行。当前正在执行的步骤会继续完成。
     */
    void stopExecution();  // 【方法】停止后续步骤执行

    /**
     * @brief 判断当前是否正在执行计划
     * @return true 当有计划正在执行中
     */
    bool isRunning() const;  // 【方法】查询执行状态

    /**
     * @brief 获取当前正在执行的计划
     * @return 当前计划的副本（包含最新的步骤状态）
     *
     * @note 返回的是副本，修改返回值不会影响调度器内部状态。
     */
    TaskPlan currentPlan() const;  // 【方法】获取当前计划副本

signals:  // 【Qt关键字】信号声明
    /**
     * @brief 收到新的执行计划
     * @param plan 解析后的 TaskPlan
     *
     * 在 setPlan() 被调用后发射，UI 层可据此初始化任务列表。
     */
    void planReceived(const TaskPlan& plan);  // 【信号】通知UI收到新计划

    /**
     * @brief 某个步骤开始执行
     * @param stepId 开始执行的步骤编号
     *
     * 在步骤状态被设为 Running 后发射，UI 可将对应步骤图标更新为"执行中"。
     */
    void stepStarted(int stepId);  // 【信号】通知UI步骤开始

    /**
     * @brief 某个步骤产生输出
     * @param stepId 产生输出的步骤编号
     * @param output 输出文本（可能是步骤头信息、Agent 执行输出、错误信息等）
     *
     * 在步骤执行过程中多次发射：
     * - 首次发射步骤头信息（包含 Agent、Tool、参数等）
     * - 随后发射 Agent 的实际执行输出
     */
    void stepOutput(int stepId, const QString& output);  // 【信号】通知UI步骤输出内容

    /**
     * @brief 某个步骤成功完成
     * @param stepId 完成的步骤编号
     * @param result Agent 返回的执行结果（包含 success=true、result、error 等）
     */
    void stepCompleted(int stepId, const QVariantMap& result);  // 【信号】通知UI步骤成功完成

    /**
     * @brief 某个步骤执行失败
     * @param stepId 失败的步骤编号
     * @param error 错误描述信息
     */
    void stepFailed(int stepId, const QString& error);  // 【信号】通知UI步骤执行失败

    /**
     * @brief 整个计划执行完成（所有步骤已处理）
     * @param plan 执行完毕的 TaskPlan（包含所有步骤的最终状态）
     *
     * 无论步骤成功或失败，所有步骤处理完毕后都会发射此信号。
     */
    void planCompleted(const TaskPlan& plan);  // 【信号】通知UI所有步骤执行完毕

    /**
     * @brief 执行被用户中断
     *
     * 在 stopExecution() 被调用后发射。
     */
    void executionStopped();  // 【信号】通知UI执行被用户停止

private slots:  // 【Qt关键字】私有槽函数
    /**
     * @brief 执行下一个步骤
     *
     * @details
     * 这是调度引擎的核心方法，采用递归调用方式实现顺序执行：
     *
     * 1. 检查停止标志（m_shouldStop）和步骤索引边界
     * 2. 取出当前步骤，更新状态为 Running，填充 startTime
     * 3. 发射 stepStarted 和 stepOutput（步骤头信息）
     * 4. 通过 findAgentForTool() 查找拥有该工具的 Agent
     * 5. 如果找不到 Agent：标记为 Failed，发射 stepFailed，递归执行下一步
     * 6. 如果找到 Agent：调用 executeTool() 同步执行
     * 7. 根据返回结果更新状态：
     *    - success=true → Completed，填充 output 和 endTime，发射 stepCompleted
     *    - success=false → Failed，填充 error 和 endTime，发射 stepFailed
     * 8. 递增步骤索引，递归调用 executeNextStep()
     *
     * 递归终止条件：
     * - 所有步骤执行完毕（m_currentStepIndex >= steps.size()）
     * - 用户调用了 stopExecution()（m_shouldStop 为 true）
     */
    void executeNextStep();  // 【槽】核心调度逻辑，递归执行每个步骤

private:  // 【访问修饰符】私有成员
    /**
     * @brief 按 Agent 名称查找已注册的 Agent
     * @param agentName 要查找的 Agent 名称（不区分大小写）
     * @return Agent 指针，如果找不到返回 nullptr
     *
     * @note 当前未被使用，因为调度器采用"按工具路由"策略。
     *       保留此方法为未来扩展（如大模型指定特定 Agent 时使用）。
     */
    Agent* findAgentByName(const QString& agentName);  // 【私有方法】按名称查找Agent

    /**
     * @brief 按工具名称查找拥有该工具的 Agent
     * @param toolName 工具名称（不区分大小写）
     * @return Agent 指针，如果找不到返回 nullptr
     *
     * @implementation
     * 遍历所有已注册的 Agent，对每个 Agent 调用 tools() 获取其工具列表，
     * 比较 tool->name() 与 toolName（忽略大小写）。
     *
     * @note 如果多个 Agent 拥有同名工具，返回第一个匹配的 Agent。
     *       为避免歧义，建议不同 Agent 的工具名称保持唯一。
     */
    Agent* findAgentForTool(const QString& toolName);  // 【私有方法】按工具查找Agent

    /**
     * @brief 构建步骤执行的日志头信息
     * @param step 当前步骤
     * @return 格式化的日志头字符串，包含步骤编号、描述、Agent、工具、参数
     *
     * @note 此字符串会被 emit stepOutput() 发送到 UI 的输出面板展示。
     */
    QString buildStepLogHeader(const TaskStep& step);  // 【私有方法】构建步骤日志头

    QList<Agent*> m_agents;      // 【成员变量】已注册的 Agent 列表（TaskScheduler 不拥有所有权）
    TaskPlan m_currentPlan;      // 【成员变量】当前正在执行的计划
    int m_currentStepIndex;      // 【成员变量】当前执行的步骤索引（从0开始）
    bool m_isRunning;            // 【成员变量】是否正在执行中
    bool m_shouldStop;           // 【成员变量】用户是否请求停止（中断标志）
};

#endif // TASKSCHEDULER_H  // 【条件编译结束】
