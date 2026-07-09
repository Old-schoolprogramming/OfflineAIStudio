/**
 * @file task.h
 * @brief Multi-Agent 执行引擎的数据结构定义
 *
 * @details
 * 本文件定义了 Multi-Agent 调度系统的核心数据结构，包括：
 * - StepStatus: 步骤执行状态枚举，描述一个步骤在生命周期中的不同阶段
 * - TaskStep:  单个执行步骤的完整描述，包含工具调用信息、参数、执行结果等
 * - TaskPlan:  由大模型生成的完整执行计划，包含一组有序的 TaskStep
 *
 * 这些数据结构是"大模型决策 + C++ 调度执行"架构的数据载体。
 * LLM 生成的 JSON 计划被解析为 TaskPlan，然后由 TaskScheduler 逐步执行每个 TaskStep。
 */

#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QDateTime>

/**
 * @brief 步骤执行状态枚举
 *
 * 每个 TaskStep 在其生命周期中会依次经历以下状态之一：
 * - Pending:   初始状态，等待调度器执行
 * - Running:   当前正在被某个 Agent 执行
 * - Completed: Agent 成功完成该步骤，结果已收集
 * - Failed:    Agent 执行失败，error 字段包含失败原因
 * - Skipped:   该步骤被跳过（当前未使用，为后续条件分支预留）
 */
enum class StepStatus {
    Pending,    ///< 等待执行
    Running,    ///< 正在执行
    Completed,  ///< 执行成功完成
    Failed,     ///< 执行失败
    Skipped     ///< 被跳过（预留）
};

/**
 * @brief 单个执行步骤的数据结构
 *
 * @details
 * TaskStep 是 Multi-Agent 执行的最小单元。一个大模型生成的任务会被分解为多个
 * TaskStep，每个 TaskStep 指定了：
 * 1. 使用哪个工具（tool）
 * 2. 传递给工具的参数（args）
 * 3. 由哪个 Agent 执行（agent，作为建议字段）
 * 4. 步骤描述（description，用于 UI 展示）
 *
 * 当 TaskScheduler 执行该步骤时，会填充 output、error、startTime、endTime 等
 * 运行时字段，并通过信号通知 UI 层更新展示。
 */
struct TaskStep {
    int         stepId;       ///< 步骤编号，从 1 开始递增，用于 UI 列表中的标识
    QString     agent;        ///< 建议使用的 Agent 名称，由大模型在生成计划时指定
    QString     tool;         ///< 工具名称，TaskScheduler 通过此字段查找对应的 Agent
    QVariantMap args;         ///< 工具参数，键值对形式，由大模型根据工具定义生成
    QString     description;  ///< 步骤描述，中文说明该步骤的目标，用于 UI 展示
    StepStatus  status;       ///< 当前执行状态，调度器在执行过程中动态更新
    QString     output;       ///< 执行成功后的输出结果，由 Agent 的 executeTool() 返回
    QString     error;        ///< 执行失败时的错误信息，由 Agent 或调度器填充
    QDateTime   startTime;    ///< 步骤开始执行的时间戳，调度器在执行前填充
    QDateTime   endTime;      ///< 步骤执行完成的时间戳，调度器在完成后填充

    /**
     * @brief 默认构造函数
     *
     * 将 stepId 初始化为 0，status 初始化为 Pending。
     * 其他字段由 Planner 在解析 JSON 时填充。
     */
    TaskStep()
        : stepId(0), status(StepStatus::Pending) {}

    /**
     * @brief 将 StepStatus 枚举转换为可读的字符串
     * @return 状态对应的英文小写字符串（pending/running/completed/failed/skipped）
     *
     * @note 此方法主要用于日志记录和调试输出，UI 层使用 statusIcon() 和 statusColor() 展示
     */
    QString statusToString() const {
        switch (status) {
            case StepStatus::Pending:   return "pending";
            case StepStatus::Running:   return "running";
            case StepStatus::Completed: return "completed";
            case StepStatus::Failed:    return "failed";
            case StepStatus::Skipped:   return "skipped";
        }
        return "unknown";
    }
};

/**
 * @brief 完整执行计划的数据结构
 *
 * @details
 * TaskPlan 是 LLM 规划层的输出产物，由 Planner 从 JSON 解析而来。
 * 一个 TaskPlan 代表一个完整的用户任务，包含一组有序的执行步骤。
 *
 * TaskScheduler 接收 TaskPlan 后，按 steps 列表的顺序逐个执行，
 * 并在执行过程中通过信号通知 UI 层更新 TaskListPanel 和 OutputPanel。
 */
struct TaskPlan {
    QString         planId;     ///< 计划唯一标识符，由 Planner 使用 QUuid 自动生成
    QString         goal;       ///< 任务目标描述，通常取用户原始输入作为目标
    QList<TaskStep> steps;      ///< 执行步骤列表，按 step_id 顺序排列
    QDateTime       createdAt;  ///< 计划创建时间，用于日志和性能分析

    /**
     * @brief 默认构造函数
     *
     * 自动将 createdAt 设置为当前系统时间。
     * planId 和 goal 由 Planner 在解析 JSON 后填充。
     */
    TaskPlan() : createdAt(QDateTime::currentDateTime()) {}

    /**
     * @brief 获取总步骤数
     * @return steps 列表的大小
     */
    int totalSteps() const { return steps.size(); }

    /**
     * @brief 获取已完成（包括成功和失败）的步骤数
     * @return 状态为 Completed 或 Failed 的步骤数量
     *
     * @note 此方法用于计算 UI 进度条的百分比
     */
    int completedSteps() const {
        int count = 0;
        for (const auto& step : steps) {
            if (step.status == StepStatus::Completed || step.status == StepStatus::Failed)
                count++;
        }
        return count;
    }

    /**
     * @brief 判断计划是否全部执行完毕
     * @return true 当 completedSteps() == totalSteps()
     */
    bool isComplete() const { return completedSteps() == totalSteps(); }
};

#endif // TASK_H
