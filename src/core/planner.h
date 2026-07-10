/**
 * @file planner.h
 * @brief 计划解析器 —— 从 JSON 到 C++ 数据结构的转换器
 *
 * @details
 * Planner 负责将大模型返回的 JSON 响应解析为 C++ 可以直接操作的数据结构（TaskPlan）。
 * 它是"文本世界"到"代码世界"的桥梁，解析的准确性直接影响整个执行流程。
 *
 * 支持两种解析模式：
 * 1. parsePlan(const QString&) —— 解析计划 JSON 数组 → TaskPlan
 * 2. parseResponse(const QString&) —— 智能识别：LLM 返回的是 plan 还是 chat
 *
 * 关键设计：
 * - 使用 Qt 的 QJsonDocument/QJsonObject 进行 JSON 解析
 * - 字段缺失或类型不匹配时提供默认值（容错设计）
 * - 支持嵌套 JSON 字符串的二次解析（应对模型输出格式异常）
 * - 空计划返回空 steps 列表（由 Orchestrator 降级为对话）
 */

#ifndef PLANNER_H  // 【条件编译】防止头文件被重复包含
#define PLANNER_H  // 【宏定义】标记该文件已被包含

#include "task.h"   // 【引入】任务数据结构头文件
#include <QString>  // 【引入】Qt字符串类

/**
 * @brief 解析响应的类型
 *
 * LLM 的响应可能是结构化的执行计划（Plan），也可能是纯文本对话（Chat）。
 * 通过 parseResponse() 方法自动识别响应类型。
 */
enum class ParsedResponseType {  // 【枚举类】C++11强类型枚举
    Plan,  // 【枚举值】结构化执行计划（JSON数组）
    Chat   // 【枚举值】纯文本对话（自然语言回答）
};

/**
 * @brief 解析后的响应数据结构
 *
 * 使用 type 字段区分 plan 和 chat，避免为两种响应创建不同的类。
 * 如果 type == Plan，plan 字段有效；
 * 如果 type == Chat，chatResponse 字段有效。
 */
struct ParsedResponse {  // 【结构体】聚合两种响应类型的数据
    ParsedResponseType type;     // 【成员】响应类型：Plan 或 Chat
    TaskPlan plan;               // 【成员】当type为Plan时有效，解析后的执行计划
    QString chatResponse;        // 【成员】当type为Chat时有效，纯文本回复内容
};

/**
 * @brief 计划解析器
 *
 * Planner 是无状态工具类，所有方法都是 const 的，可多线程安全使用。
 * 解析逻辑经过多层容错设计，能应对 7B 模型常见的输出格式偏差。
 */
class Planner  // 【类声明】纯工具类
{
public:
    /**
     * @brief 构造函数
     */
    Planner();  // 【构造函数】创建Planner实例

    /**
     * @brief 解析 JSON 计划字符串
     * @param jsonStr LLM 返回的 JSON 响应文本
     * @return 解析后的 TaskPlan
     *
     * @implementation
     * 1. 尝试将 jsonStr 解析为 QJsonDocument
     * 2. 如果是 JSON 数组 → 遍历每个元素，解析为 TaskStep
     * 3. 如果是 JSON 对象 → 查找 "steps" 或 "plan" 字段，提取步骤数组
     * 4. 如果是嵌套 JSON 字符串（如 {"content": "[...]"}）→ 二次解析
     * 5. 每个步骤提取：step_id、agent、tool、args（对象）、description
     * 6. 字段缺失时使用默认值（0, "", {}）
 */
    TaskPlan parsePlan(const QString& jsonStr) const;  // 【方法】解析JSON计划字符串

    /**
     * @brief 智能解析 LLM 响应
     * @param response LLM 返回的原始响应文本
     * @return ParsedResponse，自动区分 Plan 和 Chat
     *
     * @implementation
     * 1. 先尝试 parsePlan() 解析为计划
     * 2. 如果 steps 非空 → 返回 Plan 类型
     * 3. 如果 steps 为空 → 检查是否包含 "conversation" 关键字
     *    - 包含 → 返回 Chat 类型（提取对话内容）
 *    - 不包含 → 尝试从嵌套 JSON 中提取 content
 *    - 仍失败 → 将整个响应作为 Chat 返回
 */
    ParsedResponse parseResponse(const QString& response) const;  // 【方法】智能识别并解析响应

private:
    /**
     * @brief 解析步骤数组（内部辅助方法）
     * @param stepsArray JSON 数组，每个元素是一个步骤对象
     * @param plan 输出的 TaskPlan
     *
     * @implementation
     * 遍历数组的每个元素，提取 step_id、agent、tool、args、description 字段。
     * 支持字段别名（如 "id" → "step_id"，"parameters" → "args"）。
     * 字段缺失时使用默认值，不会导致解析失败。
     */
    void parseStepsArray(const QJsonArray& stepsArray, TaskPlan& plan) const;  // 【私有方法】解析步骤数组
};

#endif // PLANNER_H  // 【条件编译结束】
