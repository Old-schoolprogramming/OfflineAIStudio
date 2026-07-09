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
 * @brief LLM 响应解析结果
 *
 * 根据 response JSON 的 "type" 字段分为 Chat / Plan 两种模式：
 * - Chat：简单对话，直接取 "response" 字段作为回复文本
 * - Plan：复杂任务，解析 goal + steps 交给 TaskScheduler 执行
 */
struct ParsedResponse {
    enum Type { Chat, Plan, Invalid };
    Type type = Invalid;
    QString chatResponse;
    TaskPlan plan;
};

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
     * @brief 解析 LLM 响应（二合一入口）
     * @param llmResponse 大模型返回的原始文本
     * @return ParsedResponse，type 指示 Chat/Plan/Invalid
     *
     * 新 Prompt 强制 LLM 输出 {"type":"chat"/"plan", ...} 格式，
     * 此方法根据 type 字段分流：
     * - chat → 提取 response 文本
     * - plan → 提取 goal + steps，构造 TaskPlan
     */
    ParsedResponse parseResponse(const QString& llmResponse);

    /**
     * @brief 从 QJsonObject 解析 TaskPlan（从 parsePlan 中拆出的复用逻辑）
     * @param obj 顶层 JSON 对象（必须含 goal + steps 字段）
     * @return 解析后的 TaskPlan
     */
    TaskPlan parsePlanFromJson(const QJsonObject& obj);

    /**
     * @brief 解析 LLM 返回的文本，生成 TaskPlan（兼容旧格式）
     * @param llmResponse 大模型返回的原始文本
     * @return 解析后的 TaskPlan
     */
    TaskPlan parsePlan(const QString& llmResponse);

    /**
     * @brief 从任意文本中提取 JSON 块
     * @param text 原始文本
     * @return 提取到的 JSON 字符串
     */
    QString extractJsonBlock(const QString& text);

    /**
     * @brief 校验 TaskPlan 的有效性
     * @param plan 待校验的执行计划
     * @return true 当计划有效
     */
    bool validatePlan(const TaskPlan& plan);

private:
    /**
     * @brief 将单个 JSON 对象解析为 TaskStep
     * @param stepObj QJsonObject
     * @return 解析后的 TaskStep
     */
    TaskStep parseStep(const QJsonObject& stepObj);
};

#endif // PLANNER_H
