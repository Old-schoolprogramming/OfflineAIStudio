/**
 * @file promptbuilder.cpp
 * @brief Prompt 构建器实现
 *
 * @details
 * PromptBuilder 的核心工作是字符串拼接。
 * 规划模式下的系统提示词是本文件最重要的输出，它直接决定了：
 * - LLM 是否能正确理解工具描述
 * - LLM 是否能输出可被 Planner 解析的 JSON
 * - 系统整体的稳定性和可靠性
 *
 * 提示词设计原则：
 * 1. 清晰性：每个工具的描述要准确、无歧义
 * 2. 完整性：JSON 格式的每个字段都要有示例和说明
 * 3. 约束性：明确告诉 LLM "Return ONLY the JSON object, no extra explanation"
 * 4. 容错性：为可选字段提供默认值说明
 */

#include "promptbuilder.h"
#include "agent.h"
#include "tool.h"

/**
 * @brief 构造函数
 *
 * @implementation
 * 初始化两个格式模板：
 * - m_toolCallFormat: v1.0 的文本工具调用格式（保留兼容）
 * - m_jsonPlanFormat: v2.0 的 JSON 计划格式模板，使用 C++11 原始字符串字面量
 *   确保 JSON 模板中的特殊字符（如引号、反斜杠）不需要转义
 */
PromptBuilder::PromptBuilder()
    : m_toolCallFormat("<function_name>(param1=value1, param2=value2, ...)")
{
    // C++11 原始字符串字面量（R"(...)"），避免大量转义
    m_jsonPlanFormat = R"({
  "goal": "任务目标描述",
  "steps": [
    {
      "step_id": 1,
      "agent": "Agent名称",
      "tool": "工具名称",
      "args": {
        "param1": "value1",
        "param2": "value2"
      },
      "description": "步骤描述"
    }
  ]
})";
}

/**
 * @brief 析构函数
 */
PromptBuilder::~PromptBuilder()
{
}

/**
 * @brief 构建普通对话的系统提示词
 * @param role AI 角色
 * @param agents 可用 Agent 列表
 * @return 系统提示词
 *
 * @implementation
 * 1. 声明 AI 角色
 * 2. 遍历所有 Agent，输出名称、描述和工具列表
 * 3. 说明工具调用格式
 * 4. 要求中文回复
 */
QString PromptBuilder::buildSystemPrompt(const QString& role, const QList<Agent*>& agents)
{
    QString prompt = "You are an AI assistant with the role: " + role + "\n";
    prompt += "You have access to the following agents and their tools:\n\n";

    for (Agent* agent : agents) {
        prompt += "=== " + agent->name() + " ===\n";
        prompt += "Description: " + agent->description() + "\n";
        prompt += agent->formatToolsForPrompt() + "\n";
    }

    prompt += "When you need to use a tool, format your response as: " + m_toolCallFormat + "\n";
    prompt += "If no tool is needed, provide a direct answer to the user.\n";
    prompt += "Always respond in Chinese.\n";

    return prompt;
}

/**
 * @brief 构建普通对话的用户提示词
 * @param userQuery 用户输入
 * @return 用户提示词
 */
QString PromptBuilder::buildUserPrompt(const QString& userQuery)
{
    return "User query: " + userQuery + "\n";
}

/**
 * @brief 构建工具执行结果提示词
 * @param toolName 工具名称
 * @param result 执行结果
 * @return 结果提示词
 */
QString PromptBuilder::buildToolResultPrompt(const QString& toolName, const QString& result)
{
    return "Tool " + toolName + " executed successfully. Result:\n" + result + "\n";
}

/**
 * @brief 构建完整的普通对话 Prompt
 * @param userQuery 用户问题
 * @param role AI 角色
 * @param agents 可用 Agent 列表
 * @return 完整 Prompt
 */
QString PromptBuilder::buildFullPrompt(const QString& userQuery, const QString& role, const QList<Agent*>& agents)
{
    QString prompt = buildSystemPrompt(role, agents);
    prompt += "\n" + buildUserPrompt(userQuery);
    return prompt;
}

/**
 * @brief 构建规划模式的系统提示词
 * @param agents 可用 Agent 列表
 * @return 规划专用系统提示词
 *
 * @implementation
 * 系统提示词的结构：
 * 1. 角色声明："You are an expert task planner and multi-agent orchestrator"
 * 2. Agent/工具清单：遍历所有 Agent，输出 name、description、tools
 * 3. JSON 格式示例：展示精确的输出格式（m_jsonPlanFormat）
 * 4. 生成规则：7 条明确的规则约束 LLM 的输出行为
 * 5. 降级说明：纯文本回答场景的处理方式
 *
 * @note 这个提示词是系统 Prompt 工程的核心产物。
 *       任何格式变更都需要同步更新 m_jsonPlanFormat 和 Planner::parseStep() 的字段映射。
 */
QString PromptBuilder::buildPlanningSystemPrompt(const QList<Agent*>& agents)
{
    QString prompt = R"(You are an expert task planner and multi-agent orchestrator.
Your job is to analyze the user's request and create a detailed execution plan in JSON format.

You have access to the following agents and their tools:

)";

    // 输出所有 Agent 和工具的详细描述
    for (Agent* agent : agents) {
        prompt += "=== " + agent->name() + " ===\n";
        prompt += "Description: " + agent->description() + "\n";
        prompt += agent->formatToolsForPrompt() + "\n\n";
    }

    // 输出 JSON 格式要求
    prompt += R"(
Your response must be a single JSON object following this exact format:

)";
    prompt += m_jsonPlanFormat + "\n\n";

    // 输出生成规则
    prompt += R"(
Rules for creating the plan:
1. Break down the user's request into clear, sequential steps
2. Each step must specify which agent and tool to use
3. Provide appropriate arguments for each tool call
4. Steps should be ordered logically, with later steps potentially depending on earlier ones
5. Include a brief description for each step
6. Use Chinese for descriptions
7. Return ONLY the JSON object, no extra explanation text

If the user's request is a simple question that doesn't require any tools, respond with a normal answer instead of a plan.
)";

    return prompt;
}

/**
 * @brief 构建规划模式的用户提示词
 * @param userQuery 用户输入
 * @return 用户提示词
 *
 * @implementation
 * 简单的格式化，明确告知 LLM "请为此任务创建执行计划"。
 */
QString PromptBuilder::buildPlanningUserPrompt(const QString& userQuery)
{
    return "User request: " + userQuery + "\n\nPlease create an execution plan for this task.";
}
