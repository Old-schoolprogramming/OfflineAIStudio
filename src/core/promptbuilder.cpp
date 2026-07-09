/**
 * @file promptbuilder.cpp
 * @brief Prompt构建器实现
 *
 * @details
 * PromptBuilder的核心工作是字符串拼接。
 * 规划模式下的系统提示词是本文件最重要的输出，它直接决定了：
 * - LLM是否能正确理解工具描述
 * - LLM是否能输出可被Planner解析的JSON
 * - 系统整体的稳定性和可靠性
 *
 * 提示词设计原则：
 * 1. 清晰性：每个工具的描述要准确、无歧义
 * 2. 完整性：JSON格式的每个字段都要有示例和说明
 * 3. 约束性：明确告诉LLM "Return ONLY the JSON object, no extra explanation"
 * 4. 容错性：为可选字段提供默认值说明
 */

#include "promptbuilder.h"
#include "agent.h"
#include "tool.h"

/**
 * @brief 构造函数
 *
 * 初始化两个格式模板：
 * - m_toolCallFormat: v1.0的文本工具调用格式（保留兼容）
 * - m_jsonPlanFormat: v2.0的JSON计划格式模板，使用C++11原始字符串字面量
 *   确保JSON模板中的特殊字符（如引号、反斜杠）不需要转义
 */
PromptBuilder::PromptBuilder()
    : m_toolCallFormat("<function_name>(param1=value1, param2=value2, ...)")
{
    // C++11原始字符串字面量（R"(...)"），避免大量转义
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
      "description": "步骤描述",
      "use_context": true,
      "max_retries": 2
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
 * @param role AI角色描述
 * @param agents 可用Agent列表
 * @return 系统提示词字符串
 *
 * @details
 * 构建流程：
 * 1. 声明AI角色
 * 2. 遍历所有Agent，输出名称、描述和工具列表（通过Agent::formatToolsForPrompt）
 * 3. 说明工具调用格式（m_toolCallFormat）
 * 4. 要求中文回复
 */
QString PromptBuilder::buildSystemPrompt(const QString& role, const QList<Agent*>& agents)
{
    QString prompt = "You are an AI assistant with the role: " + role + "\n";
    prompt += "You have access to the following agents and their tools:\n\n";

    // 遍历所有Agent，拼接其名称、描述和工具信息
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
 * @param userQuery 用户原始输入
 * @return 格式化后的用户提示词
 *
 * 简单的字符串拼接，为LLM明确标注用户输入部分。
 */
QString PromptBuilder::buildUserPrompt(const QString& userQuery)
{
    return "User query: " + userQuery + "\n";
}

/**
 * @brief 构建工具执行结果提示词
 * @param toolName 执行的工具名称
 * @param result 工具执行返回的原始结果字符串
 * @return 格式化后的结果提示词
 *
 * 将工具执行结果包装为LLM可理解的上下文信息，
 * 通常作为新一轮对话的用户消息或系统消息插入。
 */
QString PromptBuilder::buildToolResultPrompt(const QString& toolName, const QString& result)
{
    return "Tool " + toolName + " executed successfully. Result:\n" + result + "\n";
}

/**
 * @brief 构建完整的普通对话Prompt
 * @param userQuery 用户问题
 * @param role AI角色
 * @param agents 可用Agent列表
 * @return 包含系统提示词和用户提示词的完整Prompt
 *
 * 组合buildSystemPrompt和buildUserPrompt的输出，
 * 用于一次性发送给LLM的非规划模式请求。
 */
QString PromptBuilder::buildFullPrompt(const QString& userQuery, const QString& role, const QList<Agent*>& agents)
{
    QString prompt = buildSystemPrompt(role, agents);
    prompt += "\n" + buildUserPrompt(userQuery);
    return prompt;
}

/**
 * @brief 构建规划模式的系统提示词
 * @param agents 可用Agent列表
 * @return 规划专用系统提示词
 *
 * @details
 * 系统提示词的结构：
 * 1. 角色声明："You are an expert task planner and multi-agent orchestrator"
 * 2. Agent/工具清单：遍历所有Agent，输出name、description、tools
 * 3. JSON格式示例：展示精确的输出格式（m_jsonPlanFormat）
 * 4. 生成规则：7条明确的规则约束LLM的输出行为
 * 5. 降级说明：纯文本回答场景的处理方式
 *
 * @note 这个提示词是系统Prompt工程的核心产物。
 *       任何格式变更都需要同步更新m_jsonPlanFormat和Planner::parseStep()的字段映射。
 */
QString PromptBuilder::buildPlanningSystemPrompt(const QList<Agent*>& agents)
{
    QString prompt = R"(You are an expert task planner and multi-agent orchestrator.
Your job is to analyze the user's request and create a detailed execution plan in JSON format.

You have access to the following agents and their tools:

)";

    // 输出所有Agent和工具的详细描述
    for (Agent* agent : agents) {
        prompt += "=== " + agent->name() + " ===\n";
        prompt += "Description: " + agent->description() + "\n";
        prompt += agent->formatToolsForPrompt() + "\n\n";
    }

    // 输出JSON格式要求
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
8. For steps that need the result of the previous step, set "use_context": true. The previous step's output will be passed as the "context" argument
9. For critical steps that may fail, set "max_retries": 2 or higher to enable automatic retries
10. If a task can be solved in multiple ways, prefer the simplest and most reliable method first
11. Consider using different agents for backup - if one agent fails, another can try the same task
12. For complex tasks, consider intermediate validation steps to catch errors early

Agent Collaboration Strategy:
- FileAgent: For all file system operations (reading, writing, searching, copying, etc.)
- ComputerAgent: For system operations, network, and environment information
- CodeAgent: For code analysis, statistics, and code-related operations
- SearchAgent: For content search and replacement in files
- TextAgent: For text processing, encoding/decoding, and hash calculations
- If one agent's tool fails, the system will automatically try another agent that has a similar tool
- Steps are executed sequentially, and each step's output can be passed to the next step via context

Additional step fields (optional):
- use_context (boolean): If true, the previous step's output will be injected as the "context" argument
- max_retries (integer): Maximum number of retry attempts if the step fails (default: 2)

If the user's request is a simple question that doesn't require any tools, respond with a normal answer instead of a plan.
)";

    return prompt;
}

/**
 * @brief 构建规划模式的用户提示词
 * @param userQuery 用户输入
 * @return 用户提示词
 *
 * @details
 * 简单的格式化，明确告知LLM "请为此任务创建执行计划"。
 */
QString PromptBuilder::buildPlanningUserPrompt(const QString& userQuery)
{
    return "User request: " + userQuery + "\n\nPlease create an execution plan for this task.";
}
