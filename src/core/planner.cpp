/**
 * @file planner.cpp
 * @brief 计划解析器实现
 *
 * @details
 * Planner 的核心工作是将大模型返回的非结构化/半结构化文本，
 * 转换为 C++ 程序可以安全操作的数据结构。
 *
 * 解析策略（按优先级排序）：
 * 1. 直接 JSON 数组解析（最标准的情况）
 * 2. JSON 对象中查找 steps/plan 字段（模型包装了一层对象）
 * 3. 嵌套 JSON 字符串二次解析（模型的 JSON 被序列化为字符串）
 * 4. Markdown 代码块提取（模型用 ```json 包装了输出）
 * 5. 失败时返回空计划（由 Orchestrator 降级为对话）
 *
 * 容错设计：
 * - 字段缺失 → 使用默认值（不会崩溃）
 * - 类型不匹配 → 尝试类型转换，失败则用默认值
 * - JSON 解析失败 → 返回空计划
 * - 嵌套格式异常 → 尝试多种提取策略
 */

#include "planner.h"    // 【引入】自己的头文件
#include <QJsonDocument> // 【引入】Qt JSON文档类
#include <QJsonObject>   // 【引入】Qt JSON对象类
#include <QJsonArray>    // 【引入】Qt JSON数组类
#include <QDebug>        // 【引入】Qt调试输出类
#include <QRegularExpression> // 【引入】Qt正则表达式类

/**
 * @brief 构造函数
 */
Planner::Planner()
{
    // 【说明】无状态工具类，无需初始化
}

/**
 * @brief 解析 JSON 计划字符串
 * @param jsonStr LLM 返回的 JSON 响应文本
 * @return 解析后的 TaskPlan（即使失败也返回空计划，不会崩溃）
 *
 * @implementation
 * 多阶段解析策略：
 *
 * 阶段1：直接解析 JSON 数组
 *   如果 jsonStr 本身就是一个 JSON 数组，直接解析每个元素为 TaskStep。
 *
 * 阶段2：解析 JSON 对象，查找 steps/plan 字段
 *   如果 jsonStr 是一个 JSON 对象，查找 "steps" 或 "plan" 字段，
 *   提取其值为步骤数组。
 *
 * 阶段3：嵌套 JSON 字符串二次解析
 *   如果 JSON 对象中包含字符串类型的 "content" 或 "response" 字段，
 *   且该字符串是 JSON 数组，进行二次解析。
 *
 * 阶段4：正则表达式提取 JSON 数组
 *   使用正则匹配文本中的 JSON 数组（[...]），尝试解析。
 *
 * 阶段5：全部失败 → 返回空计划
 */
TaskPlan Planner::parsePlan(const QString& jsonStr) const
{
    TaskPlan plan;  // 【创建】空的任务计划对象
    QString str = jsonStr.trimmed();  // 【处理】去除首尾空白字符

    // 如果字符串为空，直接返回空计划
    if (str.isEmpty()) {  // 【判断】空字符串检查
        return plan;  // 【返回】空计划
    }

    // ===== 阶段1：尝试直接解析为 JSON 数组 =====
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8());  // 【解析】尝试解析JSON
    if (doc.isArray()) {  // 【判断】是否为JSON数组
        QJsonArray stepsArray = doc.array();  // 【获取】数组内容
        parseStepsArray(stepsArray, plan);    // 【调用】解析步骤数组
        return plan;  // 【返回】解析后的计划
    }

    // ===== 阶段2：尝试解析为 JSON 对象，查找 steps 或 plan 字段 =====
    if (doc.isObject()) {  // 【判断】是否为JSON对象
        QJsonObject obj = doc.object();  // 【获取】对象内容

        // 尝试从 "steps" 字段提取
        if (obj.contains("steps")) {  // 【判断】是否包含steps字段
            QJsonValue stepsVal = obj["steps"];  // 【获取】steps字段值
            if (stepsVal.isArray()) {  // 【判断】是否为数组
                parseStepsArray(stepsVal.toArray(), plan);  // 【调用】解析步骤数组
                return plan;  // 【返回】解析后的计划
            } else if (stepsVal.isString()) {  // 【判断】是否为字符串（嵌套JSON）
                // 嵌套 JSON 字符串，二次解析
                QString nested = stepsVal.toString();  // 【获取】字符串内容
                QJsonDocument nestedDoc = QJsonDocument::fromJson(nested.toUtf8());  // 【解析】二次解析
                if (nestedDoc.isArray()) {  // 【判断】是否为数组
                    parseStepsArray(nestedDoc.array(), plan);  // 【调用】解析步骤数组
                    return plan;  // 【返回】解析后的计划
                }
            }
        }

        // 尝试从 "plan" 字段提取
        if (obj.contains("plan")) {  // 【判断】是否包含plan字段
            QJsonValue planVal = obj["plan"];  // 【获取】plan字段值
            if (planVal.isArray()) {  // 【判断】是否为数组
                parseStepsArray(planVal.toArray(), plan);  // 【调用】解析步骤数组
                return plan;  // 【返回】解析后的计划
            }
        }

        // ===== 阶段3：处理嵌套 JSON 字符串（如 {"content": "[...]"}）=====
        if (obj.contains("content")) {  // 【判断】是否包含content字段
            QString content = obj["content"].toString();  // 【获取】content字符串
            QJsonDocument contentDoc = QJsonDocument::fromJson(content.toUtf8());  // 【解析】尝试解析content
            if (contentDoc.isArray()) {  // 【判断】是否为数组
                parseStepsArray(contentDoc.array(), plan);  // 【调用】解析步骤数组
                return plan;  // 【返回】解析后的计划
            }
        }
    }

    // ===== 阶段4：正则表达式提取 JSON 数组 =====
    // 尝试匹配 [...] 格式的 JSON 数组（处理模型在非JSON上下文中嵌入数组的情况）
    QRegularExpression arrayRegex(R"(\[.*\])");  // 【正则】匹配方括号包围的内容
    QRegularExpressionMatch match = arrayRegex.match(str);  // 【匹配】在字符串中查找
    if (match.hasMatch()) {  // 【判断】是否匹配到
        QString arrayStr = match.captured(0);  // 【提取】匹配到的数组字符串
        QJsonDocument arrayDoc = QJsonDocument::fromJson(arrayStr.toUtf8());  // 【解析】解析为JSON
        if (arrayDoc.isArray()) {  // 【判断】是否为数组
            parseStepsArray(arrayDoc.array(), plan);  // 【调用】解析步骤数组
            return plan;  // 【返回】解析后的计划
        }
    }

    // ===== 阶段5：全部失败，返回空计划 =====
    return plan;  // 【返回】空计划（steps为空）
}

/**
 * @brief 解析步骤数组
 * @param stepsArray JSON 数组，每个元素是一个步骤对象
 * @param plan 输出的 TaskPlan
 *
 * @implementation
 * 遍历数组的每个元素，提取以下字段：
 * - step_id → TaskStep::stepId（int，默认0）
 * - agent → TaskStep::agent（QString）
 * - tool → TaskStep::tool（QString）
 * - args → TaskStep::args（QVariantMap，从 JSON 对象转换）
 * - description → TaskStep::description（QString）
 *
 * 字段缺失时使用默认值，不会导致解析失败。
 * 参数名称做了别名兼容（如 "parameters" → "args"）。
 */
void Planner::parseStepsArray(const QJsonArray& stepsArray, TaskPlan& plan) const
{
    for (const QJsonValue& stepVal : stepsArray) {  // 【遍历】数组中的每个步骤
        if (!stepVal.isObject()) continue;  // 【跳过】非对象元素

        QJsonObject stepObj = stepVal.toObject();  // 【获取】步骤对象
        TaskStep step;  // 【创建】空的步骤对象

        // 提取 step_id（支持 "step_id" 和 "id" 两种键名）
        if (stepObj.contains("step_id")) {  // 【判断】是否有step_id
            step.stepId = stepObj["step_id"].toInt(0);  // 【提取】转换为整数，默认0
        } else if (stepObj.contains("id")) {  // 【判断】是否有id
            step.stepId = stepObj["id"].toInt(0);  // 【提取】转换为整数，默认0
        }

        // 提取 agent（支持 "agent" 和 "agent_name" 两种键名）
        if (stepObj.contains("agent")) {  // 【判断】是否有agent
            step.agent = stepObj["agent"].toString();  // 【提取】转换为字符串
        } else if (stepObj.contains("agent_name")) {  // 【判断】是否有agent_name
            step.agent = stepObj["agent_name"].toString();  // 【提取】转换为字符串
        }

        // 提取 tool（支持 "tool" 和 "tool_name" 两种键名）
        if (stepObj.contains("tool")) {  // 【判断】是否有tool
            step.tool = stepObj["tool"].toString();  // 【提取】转换为字符串
        } else if (stepObj.contains("tool_name")) {  // 【判断】是否有tool_name
            step.tool = stepObj["tool_name"].toString();  // 【提取】转换为字符串
        }

        // 提取 args（支持 "args"、"arguments" 和 "parameters" 三种键名）
        if (stepObj.contains("args")) {  // 【判断】是否有args
            QJsonObject argsObj = stepObj["args"].toObject();  // 【获取】参数对象
            for (const QString& key : argsObj.keys()) {  // 【遍历】每个参数
                step.args[key] = argsObj[key].toVariant();  // 【转换】JSON值转为QVariant
            }
        } else if (stepObj.contains("arguments")) {  // 【判断】是否有arguments
            QJsonObject argsObj = stepObj["arguments"].toObject();
            for (const QString& key : argsObj.keys()) {
                step.args[key] = argsObj[key].toVariant();
            }
        } else if (stepObj.contains("parameters")) {  // 【判断】是否有parameters
            QJsonObject argsObj = stepObj["parameters"].toObject();
            for (const QString& key : argsObj.keys()) {
                step.args[key] = argsObj[key].toVariant();
            }
        }

        // 提取 description
        if (stepObj.contains("description")) {  // 【判断】是否有description
            step.description = stepObj["description"].toString();  // 【提取】转换为字符串
        }

        plan.steps.append(step);  // 【追加】将步骤添加到计划中
    }
}

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
 *
 * 这种设计使得 Orchestrator 无需关心 LLM 返回的具体格式，
 * 只需根据 ParsedResponse::type 决定后续处理逻辑。
 */
ParsedResponse Planner::parseResponse(const QString& response) const
{
    ParsedResponse result;  // 【创建】解析结果对象
    result.type = ParsedResponseType::Chat;  // 【默认】先假设是Chat类型

    // 尝试解析为计划
    TaskPlan plan = parsePlan(response);  // 【调用】解析JSON计划

    // 如果成功解析到步骤，返回 Plan 类型
    if (!plan.steps.isEmpty()) {  // 【判断】是否解析到步骤
        result.type = ParsedResponseType::Plan;  // 【设置】类型为Plan
        result.plan = plan;  // 【保存】解析到的计划
        return result;  // 【返回】Plan类型结果
    }

    // 如果没有解析到步骤，检查是否是特定的对话格式
    if (response.contains("conversation", Qt::CaseInsensitive)) {  // 【判断】是否包含conversation关键字
        // 尝试提取对话内容
        QRegularExpression contentRegex(R"regex("content"\s*:\s*"([^"]*)"\s*})regex");  // 【正则】匹配content字段
        QRegularExpressionMatch match = contentRegex.match(response);  // 【匹配】在响应中查找
        if (match.hasMatch()) {  // 【判断】是否匹配到
            result.chatResponse = match.captured(1);  // 【提取】捕获组1为对话内容
        } else {
            result.chatResponse = response;  // 【兜底】使用完整响应作为对话内容
        }
    } else {
        // 尝试从嵌套 JSON 中提取 content
        QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());  // 【解析】尝试解析JSON
        if (doc.isObject()) {  // 【判断】是否为对象
            QJsonObject obj = doc.object();  // 【获取】对象内容
            if (obj.contains("content")) {  // 【判断】是否有content字段
                result.chatResponse = obj["content"].toString();  // 【提取】content作为对话内容
            } else {
                result.chatResponse = response;  // 【兜底】使用完整响应
            }
        } else {
            result.chatResponse = response;  // 【兜底】使用完整响应
        }
    }

    return result;  // 【返回】Chat类型结果
}
