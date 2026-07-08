/**
 * @file planner.h
 * @brief 计划解析器 —— 将大模型返回的文本解析为结构化的 TaskPlan
 *
 * @details
 * Planner 是 LLM 规划层与 C++ 调度层之间的"翻译官"。
 *
 * 大模型返回的文本可能是：
 * - 纯 JSON 对象
 * - 包裹在 markdown 代码块中的 JSON
 * - 混有解释文本和 JSON 的混合文本
 *
 * Planner 负责从这些文本中提取 JSON 块，解析为 TaskPlan 数据结构。
 * 解析失败时，返回空计划，系统会自动降级为普通对话模式。
 *
 * 解析流程：
 * 1. extractJsonBlock() — 从文本中提取 JSON 字符串
 * 2. parsePlan()        — 将 JSON 解析为 TaskPlan
 * 3. validatePlan()     — 校验计划是否有效（至少有一个步骤，每个步骤有 tool）
 */

#ifndef PLANNER_H
#define PLANNER_H

#include "task.h"
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief 计划解析器类
 *
 * Planner 不持有状态，所有方法都是纯函数式操作（除了解析过程中创建的对象外）。
 * 这意味着 Planner 实例可以被多个线程安全使用，也可以被复用解析多个计划。
 */
class Planner
{
public:
    /**
     * @brief 默认构造函数
     *
     * Planner 无内部状态需要初始化。
     */
    Planner();

    /**
     * @brief 析构函数
     */
    ~Planner();

    /**
     * @brief 解析 LLM 返回的文本，生成 TaskPlan
     * @param llmResponse 大模型返回的原始文本（可能包含 JSON、markdown 等）
     * @return 解析后的 TaskPlan。如果解析失败，返回 steps 为空的 TaskPlan
     *
     * @details
     * 解析流程：
     * 1. 调用 extractJsonBlock() 提取 JSON 字符串
     * 2. 使用 QJsonDocument 解析 JSON
     * 3. 提取 goal 字段作为计划目标
     * 4. 遍历 steps 数组，逐个解析为 TaskStep
     * 5. 为每个步骤填充 step_id、agent、tool、args、description
     * 6. 使用 QUuid 生成唯一的 planId
     *
     * @note 如果 LLM 返回的是纯文本回答（不需要工具），此方法会返回空步骤列表，
     *       Orchestrator 会据此判断为普通对话模式。
     */
    TaskPlan parsePlan(const QString& llmResponse);

    /**
     * @brief 从任意文本中提取 JSON 块
     * @param text 原始文本，可能包含 markdown 代码块或混有解释文字
     * @return 提取到的 JSON 字符串。如果没有找到 JSON，返回原文本
     *
     * @details
     * 使用正则表达式匹配第一个 `{...}` 结构。支持：
     * - ```json\n{...}\n``` 格式的 markdown 代码块
     * - 直接以 `{` 开头的 JSON 文本
     * - 混有前缀文本的 JSON（如 "Here is the plan: {...}"）
     *
     * @note 此方法假设 LLM 返回的 JSON 是合法的（即只有一个顶层 JSON 对象）。
     *       如果存在多个 JSON 对象，只会提取第一个。
     */
    QString extractJsonBlock(const QString& text);

    /**
     * @brief 校验 TaskPlan 的有效性
     * @param plan 待校验的执行计划
     * @return true 当计划包含至少一个步骤，且每个步骤都有非空的 tool 字段
     *
     * @details
     * 校验规则：
     * 1. steps 列表不能为空（空计划意味着 LLM 没有生成可执行步骤）
     * 2. 每个步骤必须有 tool 名称（tool 是路由到 Agent 的关键字段）
     *
     * @note 此方法目前不校验 Agent 名称是否存在，该检查由 TaskScheduler 在执行时完成。
     */
    bool validatePlan(const TaskPlan& plan);

private:
    /**
     * @brief 将单个 JSON 对象解析为 TaskStep
     * @param stepObj QJsonObject，对应 steps 数组中的一个元素
     * @return 解析后的 TaskStep
     *
     * @details
     * 解析的字段映射：
     * - step_id / id → stepId
     * - agent         → agent
     * - tool          → tool
     * - args          → args（QJsonObject 转为 QVariantMap）
     * - description / name → description
     *
     * 不存在的字段保持为空或默认值。
     */
    TaskStep parseStep(const QJsonObject& stepObj);
};

#endif // PLANNER_H
