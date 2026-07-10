/**
 * @file task.h
 * @brief 任务数据结构 —— 整个系统的数据契约
 *
 * @details
 * 本文件定义了 Multi-Agent 系统中所有核心数据结构，
 * 是 LLM、Planner、TaskScheduler、UI 之间的"数据契约"。
 *
 * 核心数据结构：
 * 1. StepStatus —— 步骤执行状态枚举（Pending/Running/Completed/Failed）
 * 2. TaskStep —— 单个执行步骤（包含 agent、tool、args、状态、时间等）
 * 3. TaskPlan —— 完整的执行计划（包含目标、步骤列表）
 *
 * 设计特点：
 * - 使用 struct 而非 class：这些数据结构是纯数据载体，没有复杂行为
 * - 使用 Qt 类型（QString、QVariantMap、QDateTime）：便于与 Qt 框架集成
 * - 可安全复制：不含指针，深拷贝和浅拷贝效果相同
 * - 可序列化：便于日志记录、网络传输、状态持久化
 *
 * @note 这些数据结构是"值语义"的，可以自由复制和传递。
 */

#ifndef TASK_H  // 【条件编译】防止头文件被重复包含
#define TASK_H  // 【宏定义】标记该文件已被包含

#include <QString>     // 【引入】Qt字符串类
#include <QVariantMap> // 【引入】Qt变体映射类，用于键值对参数
#include <QDateTime>   // 【引入】Qt日期时间类，用于记录执行时间
#include <QList>       // 【引入】Qt列表容器

/**
 * @brief 步骤执行状态枚举
 *
 * 描述一个 TaskStep 在其生命周期中的当前状态。
 * 状态转换图：
 *   Pending → Running → Completed
 *                      → Failed
 *
 * Pending：步骤已创建，尚未开始执行
 * Running：步骤正在执行中
 * Completed：步骤已成功完成
 * Failed：步骤执行失败（错误信息保存在 TaskStep::error 中）
 */
enum class StepStatus {  // 【枚举类】C++11强类型枚举
    Pending,    // 【枚举值】待执行：步骤已创建，等待调度
    Running,    // 【枚举值】执行中：步骤正在被Agent执行
    Completed,  // 【枚举值】已完成：步骤执行成功
    Failed      // 【枚举值】已失败：步骤执行出错
};

/**
 * @brief 单条执行步骤
 *
 * TaskStep 是执行计划的最小单元，对应大模型规划 JSON 中的一个对象：
 * { "step_id": 1, "agent": "ComputerAgent", "tool": "runCommand",
 *   "args": {"command": "mkdir test"}, "description": "创建目录" }
 *
 * 除规划信息外，还包含执行过程中的运行时状态：
 * - status：当前执行状态
 * - startTime / endTime：执行起止时间
 * - output：执行输出结果
 * - error：错误信息（如果失败）
 */
struct TaskStep {  // 【结构体】单个执行步骤
    int stepId = 0;              // 【成员】步骤编号，从1开始递增，用于标识和排序
    QString agent;               // 【成员】执行该步骤的Agent名称（如"ComputerAgent"）
    QString tool;                // 【成员】要调用的工具名称（如"runCommand"）
    QVariantMap args;            // 【成员】工具参数，键值对形式（如{"command": "mkdir test"}）
    QString description;         // 【成员】步骤描述，用于UI展示（如"创建项目目录"）

    StepStatus status = StepStatus::Pending;  // 【成员】当前执行状态，默认为Pending
    QDateTime startTime;         // 【成员】开始执行时间，执行前为空
    QDateTime endTime;           // 【成员】执行结束时间，执行前为空
    QString output;              // 【成员】执行输出结果，成功后填充
    QString error;               // 【成员】错误信息，失败后填充

    /**
     * @brief 判断步骤是否已完成（无论成功或失败）
     * @return true 如果状态是 Completed 或 Failed
     */
    bool isFinished() const {  // 【方法】检查步骤是否已结束
        return status == StepStatus::Completed || status == StepStatus::Failed;  // 【返回】是否已完成或失败
    }
};

/**
 * @brief 完整的执行计划
 *
 * TaskPlan 包含一个目标（goal）和一系列有序的步骤（steps）。
 * 它对应大模型规划 JSON 的完整内容，是 Planner 的输出、TaskScheduler 的输入。
 *
 * 使用示例：
 *   TaskPlan plan;
 *   plan.goal = "创建一个Qt项目";
 *   plan.steps.append(step1);
 *   plan.steps.append(step2);
 */
struct TaskPlan {  // 【结构体】完整执行计划
    QString goal;                // 【成员】计划目标，通常为用户原始输入或任务描述
    QList<TaskStep> steps;       // 【成员】有序步骤列表，按stepId顺序执行

    /**
     * @brief 获取计划的总步骤数
     * @return 步骤数量
     */
    int totalSteps() const {  // 【方法】获取总步骤数
        return steps.size();  // 【返回】步骤列表的大小
    }

    /**
     * @brief 获取已完成（成功或失败）的步骤数
     * @return 已完成步骤数量
     */
    int completedSteps() const {  // 【方法】获取已完成步骤数
        int count = 0;  // 【计数】初始化计数器
        for (const auto& step : steps) {  // 【遍历】所有步骤
            if (step.isFinished()) count++;  // 【判断】如果步骤已结束，计数+1
        }
        return count;  // 【返回】已完成步骤数
    }

    /**
     * @brief 判断计划是否全部执行完毕
     * @return true 如果所有步骤都已完成
     */
    bool isComplete() const {  // 【方法】检查计划是否全部完成
        return completedSteps() == totalSteps();  // 【返回】已完成数是否等于总数
    }
};

#endif // TASK_H  // 【条件编译结束】
