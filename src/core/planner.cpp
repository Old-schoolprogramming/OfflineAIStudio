/**
 * @file planner.cpp
 * @brief 计划解析器实现
 *
 * @details
 * Planner 的实现基于 Qt 的 JSON 解析能力（QJsonDocument / QJsonObject / QJsonArray）
 * 和正则表达式提取。
 *
 * 核心设计考量：
 * 1. 容错性：大模型的输出格式可能不严格，需要支持多种 JSON 包裹形式
 * 2. 最小化依赖：仅依赖 Qt 核心库，无需第三方 JSON 库
 * 3. 性能：解析过程在内存中完成，时间复杂度 O(n)，n 为 LLM 返回文本长度
 */

#include "planner.h"
#include <QRegularExpression>
#include <QUuid>

/**
 * @brief 构造函数
 *
 * Planner 是无状态类，构造函数不初始化任何成员变量。
 */
Planner::Planner()
{
}

/**
 * @brief 析构函数
 */
Planner::~Planner()
{
}

/**
 * @brief 从文本中提取 JSON 字符串块
 * @param text LLM 返回的原始文本
 * @return 提取到的 JSON 字符串
 *
 * @implementation
 * 使用正则表达式 `\{[\s\S]*\}` 匹配第一个以 `{` 开头、以 `}` 结尾的块。
 * `[\s\S]` 匹配任意字符（包括换行符），确保能匹配多行 JSON。
 *
 * 如果匹配成功，返回匹配的完整字符串（包含外层大括号）。
 * 如果匹配失败（LLM 没有返回 JSON），返回去除首尾空白后的原文本，
 * 以便上层尝试作为纯文本处理。
 */
QString Planner::extractJsonBlock(const QString& text)
{
    // 正则：匹配最外层的一对花括号及其内部所有内容（包括换行）
    QRegularExpression jsonRegex(R"(\{[\s\S]*\})");
    QRegularExpressionMatch match = jsonRegex.match(text);
    if (match.hasMatch()) {
        return match.captured(0);
    }
    return text.trimmed();
}

/**
 * @brief 解析 LLM 响应，生成 TaskPlan
 * @param llmResponse 大模型返回的原始文本
 * @return 解析后的 TaskPlan
 *
 * @implementation
 * 1. 生成唯一 planId（QUuid，去除花括号格式）
 * 2. 提取 JSON 块
 * 3. 解析为 QJsonDocument，验证是否为对象
 * 4. 提取 goal 字段（兼容 "goal" 和 "task" 两个键名）
 * 5. 遍历 steps 数组，调用 parseStep() 逐个解析
 * 6. 将解析后的 TaskStep 添加到 plan.steps
 *
 * 如果任何步骤解析失败（如 steps 不是数组），该步骤会被跳过，
 * 其他步骤仍然会被添加到计划中。
 */
TaskPlan Planner::parsePlan(const QString& llmResponse)
{
    TaskPlan plan;

    // 使用 QUuid 生成唯一标识符，去除花括号使其更紧凑
    plan.planId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 第一步：从文本中提取 JSON 块
    QString jsonStr = extractJsonBlock(llmResponse);

    // 第二步：解析 JSON 字符串
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());

    // 如果解析失败或不是 JSON 对象，返回空计划（后续降级为普通对话）
    if (!doc.isObject()) {
        plan.goal = "解析失败";
        return plan;
    }

    QJsonObject obj = doc.object();

    // 第三步：提取目标描述，兼容 "goal" 和 "task" 两个字段名
    if (obj.contains("goal")) {
        plan.goal = obj["goal"].toString();
    } else if (obj.contains("task")) {
        plan.goal = obj["task"].toString();
    }

    // 第四步：解析步骤数组
    if (obj.contains("steps") && obj["steps"].isArray()) {
        QJsonArray stepsArray = obj["steps"].toArray();
        for (const auto& stepValue : stepsArray) {
            if (stepValue.isObject()) {
                TaskStep step = parseStep(stepValue.toObject());
                plan.steps.append(step);
            }
        }
    }

    return plan;
}

/**
 * @brief 将单个 JSON 对象解析为 TaskStep
 * @param stepObj steps 数组中的一个元素
 * @return 填充后的 TaskStep
 *
 * @implementation
 * 字段映射逻辑：
 * - step_id（优先）或 id → stepId，默认为 0
 * - agent → agent 名称
 * - tool → 工具名称
 * - args → 遍历 QJsonObject 的所有键值对，转为 QVariantMap
 * - description（优先）或 name → 步骤描述
 *
 * 所有字段都是可选的，缺失的字段保持为空或默认值。
 * 这种设计允许大模型输出部分字段，Planner 仍然能正确解析已知字段。
 */
TaskStep Planner::parseStep(const QJsonObject& stepObj)
{
    TaskStep step;

    // 解析步骤编号：优先使用 "step_id"，其次 "id"
    if (stepObj.contains("step_id")) {
        step.stepId = stepObj["step_id"].toInt();
    } else if (stepObj.contains("id")) {
        step.stepId = stepObj["id"].toInt();
    }

    // 解析建议的 Agent 名称
    if (stepObj.contains("agent")) {
        step.agent = stepObj["agent"].toString();
    }

    // 解析工具名称（路由到 Agent 的关键字段）
    if (stepObj.contains("tool")) {
        step.tool = stepObj["tool"].toString();
    }

    // 解析参数：将 QJsonObject 转为 QVariantMap
    if (stepObj.contains("args") && stepObj["args"].isObject()) {
        QJsonObject argsObj = stepObj["args"].toObject();
        for (auto it = argsObj.begin(); it != argsObj.end(); ++it) {
            step.args[it.key()] = it.value().toVariant();
        }
    }

    // 解析描述：优先使用 "description"，其次 "name"
    if (stepObj.contains("description")) {
        step.description = stepObj["description"].toString();
    } else if (stepObj.contains("name")) {
        step.description = stepObj["name"].toString();
    }

    // 初始状态为 Pending，等待调度器执行
    step.status = StepStatus::Pending;

    return step;
}

/**
 * @brief 校验执行计划的有效性
 * @param plan 待校验的 TaskPlan
 * @return true 当计划有效
 *
 * @implementation
 * 校验规则：
 * 1. steps 列表不能为空 → 空计划无法执行
 * 2. 每个步骤必须有非空的 tool 字段 → tool 是路由到 Agent 的必要条件
 *
 * 不校验 Agent 名称是否存在，因为：
 * - 大模型可能拼错 Agent 名称
 * - TaskScheduler 在执行时会通过 findAgentForTool() 动态查找
 * - 找不到 Agent 时会通过 stepFailed 信号报告错误
 */
bool Planner::validatePlan(const TaskPlan& plan)
{
    // 空计划无法执行
    if (plan.steps.isEmpty()) {
        return false;
    }

    // 每个步骤必须有工具名称
    for (const auto& step : plan.steps) {
        if (step.tool.isEmpty()) {
            return false;
        }
    }

    return true;
}
