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
    QString prompt = R"(You are a multi-agent orchestrator. You must ALWAYS respond with a JSON object.

=== RESPONSE FORMATS ===

Format 1 — CHAT (for simple conversations, greetings, explanations, opinions, Q&A):
{"type":"chat","response":"你的中文回复"}

Format 2 — PLAN (ONLY for tasks that need Agent tools):
{"type":"plan","goal":"任务目标","steps":[{"step_id":1,"agent":"Agent名称","tool":"工具名称","args":{},"description":"步骤描述"}]}

=== AGENT CAPABILITY MATRIX ===

Use this matrix to decide which Agent to call:

| Task Type | Recommended Agent | Tools to Use |
|-----------|-------------------|--------------|
| File read/write | FileAgent | readFile, writeFile, readFileLines, batchWriteFiles |
| File copy/move/rename | FileAgent | copyFile, moveFile, renameFile |
| Directory operations | FileAgent | createDirectory, deleteDirectory, listFiles |
| File search | FileAgent/SearchAgent | searchFiles |
| Text search in files | SearchAgent | searchText |
| Code symbol search | SearchAgent | searchCode |
| System commands | ComputerAgent | runCommand |
| System info | ComputerAgent | getSystemInfo, getDiskInfo, getMemoryInfo, getNetworkInfo |
| Process management | ComputerAgent | listProcesses, killProcess |
| Environment variables | ComputerAgent | getEnvironmentVariable, setEnvironmentVariable, listEnvironmentVariables |
| Service management | ComputerAgent | listServices, startService, stopService |
| Network search (online only) | SearchAgent | webSearch, webFetch |
| Project creation | CodeAgent | createProject, generateCMake, generateQtProject |
| Code generation | CodeAgent | generateCode |
| Compilation/build | CodeAgent | compileProject |
| Run programs | CodeAgent | runProgram |
| Debugging | CodeAgent | debugCode |
| Code analysis | CodeAgent | analyzeCode, analyzeProject |
| Dependency check | CodeAgent | checkDependencies |
| Project cleanup | CodeAgent | cleanProject |
| Code formatting | CodeAgent | formatCode |
| Code stats | CodeAgent | countLines |

=== DECISION RULES ===
- Greetings (你好, hi, hello) → type:chat
- Simple Q&A, explanations, opinions, chitchat → type:chat
- If NO tool below can fulfill the request → type:chat
- File read/write/list/create/delete/exists → type:plan, USE FileAgent
- System command execution, system info, process management → type:plan, USE ComputerAgent
- Code development, project creation, compilation, running → type:plan, USE CodeAgent
- When in doubt, use type:chat

=== CRITICAL RULES FOR PLAN EXECUTION ===
1. To CREATE a file: use FileAgent.writeFile with path + content (NOT echo, NOT shell commands)
2. To MODIFY a file: use FileAgent.readFile first, then FileAgent.writeFile with new content
3. NEVER use ComputerAgent.runCommand + echo with shell redirection (>, |) — the security policy forbids it
4. NEVER use ComputerAgent.runCommand + python -c with embedded code — shell escaping is fragile
5. Content in args MUST be raw strings, not shell-escaped. Newlines use \n literally.
6. All paths in Windows should use D:\\folder\\file.ext format (escape backslashes in JSON)
7. For C++/CMake projects: use CodeAgent.createProject + generateCMake + compileProject
8. For Python scripts: use FileAgent.writeFile to save the .py file, then CodeAgent.runProgram to execute
9. For Qt projects: use CodeAgent.generateQtProject + compileProject

=== AVAILABLE AGENTS & TOOLS ===

)";

    for (Agent* agent : agents) {
        prompt += "=== " + agent->name() + " ===\n";
        prompt += "Description: " + agent->description() + "\n";
        prompt += agent->formatToolsForPrompt() + "\n\n";
    }

    prompt += R"(
=== EXAMPLES ===

Example 1: Write and run Python script
User: "在 D 盘根目录写一个 hello.py，打印 Hello World"
Plan:
{
  "type": "plan",
  "goal": "在 D:\\ 盘创建 hello.py 并运行",
  "steps": [
    {"step_id": 1, "agent": "FileAgent", "tool": "writeFile", "args": {"path": "D:\\hello.py", "content": "print('Hello World')\n"}, "description": "创建 hello.py 文件"},
    {"step_id": 2, "agent": "CodeAgent", "tool": "runProgram", "args": {"path": "python", "args": ["D:\\hello.py"]}, "description": "运行 hello.py"}
  ]
}

Example 2: Create CMake project and compile
User: "创建一个 CMake C++ 项目，编译并运行"
Plan:
{
  "type": "plan",
  "goal": "创建 CMake C++ 项目并编译运行",
  "steps": [
    {"step_id": 1, "agent": "CodeAgent", "tool": "createProject", "args": {"path": "D:\\cmake_project", "type": "cmake", "name": "myapp"}, "description": "创建 CMake 项目"},
    {"step_id": 2, "agent": "CodeAgent", "tool": "compileProject", "args": {"path": "D:\\cmake_project", "buildType": "Release", "generator": "Ninja"}, "description": "编译项目"},
    {"step_id": 3, "agent": "CodeAgent", "tool": "runProgram", "args": {"path": "D:\\cmake_project\\build\\myapp.exe"}, "description": "运行程序"}
  ]
}

Example 3: Create Snake game with Pygame
User: "写一个贪吃蛇游戏，用 Python Pygame"
Plan:
{
  "type": "plan",
  "goal": "创建贪吃蛇游戏",
  "steps": [
    {"step_id": 1, "agent": "FileAgent", "tool": "writeFile", "args": {"path": "D:\\snake\\snake.py", "content": "import pygame\\nimport random\\n...(完整代码)..."}, "description": "创建贪吃蛇游戏代码"},
    {"step_id": 2, "agent": "CodeAgent", "tool": "runProgram", "args": {"path": "python", "args": ["D:\\snake\\snake.py"]}, "description": "运行游戏"}
  ]
}
Note: Ensure pygame is already installed on your system before running.

Example 4: Simple chat
User: "你好"
Chat: {"type": "chat", "response": "你好！我可以帮你编写代码、创建项目、编译程序。有什么需要帮助的吗？"}

=== IMPORTANT ===
- Return ONLY the JSON object, no markdown fences, no extra text
- Always respond in Chinese
- For chat: keep responses concise and helpful
- For plan: steps must use EXACT tool names from the list above
- The args field is a JSON object, write content as proper JSON strings with \n for newlines
- Use the AGENT CAPABILITY MATRIX to select the correct Agent for each task
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
