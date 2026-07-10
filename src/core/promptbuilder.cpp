/**
 * @file promptbuilder.cpp
 * @brief 系统提示词构建器实现
 *
 * @details
 * PromptBuilder 的核心工作是根据系统当前可用的 Agent/工具，
 * 动态生成高质量的系统提示词，引导大模型输出结构化的执行计划。
 *
 * 提示词工程的关键洞察：
 * 1. 角色设定：明确告诉大模型"你是 Multi-Agent 系统总控"
 * 2. 能力清单：逐一列举每个工具的名称、功能、参数
 * 3. 工作流说明：plan 阶段只决策不执行，tool 阶段只执行不决策
 * 4. 格式约束：强制 JSON 数组格式，包含 step_id、agent、tool、args、description
 * 5. Few-shot 示例：提供 5+ 个真实场景的示例，7B 模型可以直接模仿
 * 6. 简化要求：不要写太长的描述，降低 token 消耗
 *
 * 这些提示词经过大量测试优化，针对 7B 参数级别的本地模型做了专门适配。
 */

#include "promptbuilder.h"  // 【引入】自己的头文件
#include "agent.h"          // 【引入】Agent基类头文件
#include "tool.h"           // 【引入】Tool基类头文件
#include <QJsonObject>      // 【引入】Qt JSON对象类

/**
 * @brief 构造函数
 *
 * PromptBuilder 是无状态工具类，构造函数不做任何初始化工作。
 */
PromptBuilder::PromptBuilder()
{
    // 【说明】无状态工具类，无需初始化
}

/**
 * @brief 构建规划阶段系统提示词
 * @param agents 所有可用的 Agent 列表
 * @return 完整的系统提示词字符串
 *
 * @implementation
 * 系统提示词包含以下部分（按顺序拼接）：
 * 1. 角色设定：告诉 LLM 它是 Multi-Agent 系统的 Orchestrator
 * 2. 可用 Agent 列表：枚举所有 Agent 及其工具
 * 3. 工作流说明：plan 只做决策，tool 只做执行
 * 4. 输出格式要求：严格的 JSON 数组格式
 * 5. 大量示例：8 个真实场景的 few-shot 示例
 * 6. 通用能力说明：直接回答不需要工具的场景
 *
 * 每个工具的 Prompt 描述通过 tool->promptDescription() 动态生成，
 * 确保提示词始终与系统中实际注册的工具保持一致。
 */
QString PromptBuilder::buildPlanningSystemPrompt(const QList<Agent*>& agents) const
{
    // 【创建】系统提示词的基础部分：角色设定和工作流说明
    QString systemPrompt =
        "## Role\n"
        "你是 Multi-Agent System Orchestrator。你是中央协调器，拥有多个专门的Agent团队。\n\n"
        "## Workflow\n"
        "1. plan 阶段：只做决策，不执行工具，不输出思考过程，只输出严格的 JSON Plan\n"
        "2. tool 阶段：只做执行，不推理，按 plan 调用工具\n\n"
        "## Agents\n";

    // 【遍历】为每个Agent及其工具生成描述
    for (Agent* agent : agents) {
        systemPrompt += QString("### %1\n%2\n").arg(agent->name(), agent->description());  // 【添加】Agent名称和描述
        for (Tool* tool : agent->tools()) {
            systemPrompt += tool->promptDescription() + "\n";  // 【添加】每个工具的Prompt描述
        }
    }

    // 【添加】输出格式要求部分
    systemPrompt +=
        "\n## Output Format\n"
        "必须用严格的 JSON 数组格式：\n"
        "[{\"step_id\": 1, \"agent\": \"ComputerAgent\", \"tool\": \"runCommand\", \"args\": {\"command\": \"...\"}, \"description\": \"...\"}, ...]\n\n";

    // 【添加】提示词要求和能力说明
    systemPrompt +=
        "## Requirements\n"
        "- step_id 必须从1开始连续递增\n"
        "- agent 必须是上面列出的 Agent 名称\n"
        "- tool 必须是该 Agent 的工具名称\n"
        "- args 必须符合工具的参数要求\n"
        "- 只要任务涉及文件操作，必须按 writeFile 三步执行\n"
        "- 不要写太长的 description，简短即可\n\n"
        "## 示例：\n"
        "用户请求：\"创建一个文件夹D:\\test，在里面新建一个hello.py，写入打印Hello World的代码并运行\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"ComputerAgent\",\"tool\":\"runCommand\",\"args\":{\"command\":\"mkdir D:\\\\test\"},\"description\":\"创建项目目录\"},{\"step_id\":2,\"agent\":\"ComputerAgent\",\"tool\":\"writeFile\",\"args\":{\"path\":\"D:\\\\test\\\\hello.py\",\"content\":\"print('Hello World')\"},\"description\":\"写入Python代码\"},{\"step_id\":3,\"agent\":\"ComputerAgent\",\"tool\":\"runCommand\",\"args\":{\"command\":\"cd D:\\\\test && python hello.py\"},\"description\":\"运行程序\"}]\n\n"
        "用户请求：\"在D:\\docs下写一个README.md，介绍这个Qt C++ Multi-Agent项目\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"ComputerAgent\",\"tool\":\"writeFile\",\"args\":{\"path\":\"D:\\\\docs\\\\README.md\",\"content\":\"# Qt C++ Multi-Agent项目\\n\\n...（文件内容）\"},\"description\":\"写入README文件\"}]\n\n"
        "用户请求：\"查看D:\\test\\hello.py的内容\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"ComputerAgent\",\"tool\":\"readFile\",\"args\":{\"path\":\"D:\\\\test\\\\hello.py\"},\"description\":\"读取文件内容\"}]\n\n"
        "用户请求：\"创建一个Qt QWidget项目，放在D:\\MyQtApp，包含main.cpp和CMakeLists.txt\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"ComputerAgent\",\"tool\":\"runCommand\",\"args\":{\"command\":\"mkdir D:\\\\MyQtApp\"},\"description\":\"创建项目目录\"},{\"step_id\":2,\"agent\":\"ComputerAgent\",\"tool\":\"writeFile\",\"args\":{\"path\":\"D:\\\\MyQtApp\\\\main.cpp\",\"content\":\"#include <QApplication>\\n#include <QWidget>\\n\\nint main(int argc, char *argv[]) {\\n    QApplication a(argc, argv);\\n    QWidget w;\\n    w.show();\\n    return a.exec();\\n}\"},\"description\":\"写入main.cpp\"},{\"step_id\":3,\"agent\":\"ComputerAgent\",\"tool\":\"writeFile\",\"args\":{\"path\":\"D:\\\\MyQtApp\\\\CMakeLists.txt\",\"content\":\"cmake_minimum_required(VERSION 3.16)\\nproject(MyQtApp)\\nset(CMAKE_CXX_STANDARD 17)\\nfind_package(Qt6 REQUIRED COMPONENTS Widgets)\\nadd_executable(MyQtApp main.cpp)\\ntarget_link_libraries(MyQtApp Qt6::Widgets)\"},\"description\":\"写入CMakeLists.txt\"}]\n\n"
        "用户请求：\"分析D:\\test下所有文件的代码质量和bug\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"ComputerAgent\",\"tool\":\"readFile\",\"args\":{\"path\":\"D:\\\\test\"},\"description\":\"读取文件内容\"},{\"step_id\":2,\"agent\":\"CodeAgent\",\"tool\":\"analyzeCode\",\"args\":{\"code\":\"{{file_content}}\",\"analysis_type\":\"comprehensive\"},\"description\":\"分析代码\"}]\n\n"
        "用户请求：\"编译D:\\MyQtApp这个项目\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"ComputerAgent\",\"tool\":\"runCommand\",\"args\":{\"command\":\"cd D:\\\\MyQtApp && cmake -B build .\"},\"description\":\"配置CMake\"},{\"step_id\":2,\"agent\":\"ComputerAgent\",\"tool\":\"runCommand\",\"args\":{\"command\":\"cd D:\\\\MyQtApp && cmake --build build\"},\"description\":\"编译项目\"}]\n\n"
        "用户请求：\"解释一下多线程编程中的死锁问题\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"CodeAgent\",\"tool\":\"askQuestion\",\"args\":{\"question\":\"解释多线程编程中的死锁问题\"},\"description\":\"回答技术问题\"}]\n\n"
        "用户请求：\"帮我写一个Python函数，计算斐波那契数列\"\n"
        "JSON计划：\n"
        "[{\"step_id\":1,\"agent\":\"CodeAgent\",\"tool\":\"generateCode\",\"args\":{\"language\":\"python\",\"description\":\"计算斐波那契数列的函数\"},\"description\":\"生成Python代码\"}]\n\n"
        "## 通用能力\n"
        "- 你也可以不返回 JSON，直接给出自然语言回答，用于简单问答、代码解释、技术讨论等不需要使用工具的场景。\n"
        "- 对于简单的文本处理、翻译、润色等任务，直接返回结果即可，不需要生成计划。\n"
        "- 只有涉及文件操作、代码生成、系统命令等需要工具执行的任务，才需要生成 JSON 计划。\n";

    return systemPrompt;  // 【返回】完整的系统提示词
}

/**
 * @brief 构建规划阶段用户提示词
 * @param userInput 用户的自然语言输入
 * @return 包装后的用户提示词
 *
 * @implementation
 * 当前实现只是简单包装，添加说明要求返回 JSON 计划。
 * 未来可以在此添加：
 * - 对话历史上下文
 * - 用户偏好设置
 * - 环境信息（当前目录、可用磁盘空间等）
 */
QString PromptBuilder::buildPlanningUserPrompt(const QString& userInput) const
{
    // 【构造】将用户输入包装成Prompt格式
    return QString("根据以下用户请求，生成详细的执行计划：\n\n用户请求：%1\n\n请用JSON格式返回计划。").arg(userInput);
}

/**
 * @brief 构建工具定义 Prompt
 * @param agents 所有可用的 Agent 列表
 * @return 所有工具的定义文本
 *
 * @implementation
 * 遍历所有 Agent 的所有工具，调用 tool->promptDescription() 获取每个工具的文本描述，
 * 用换行符连接。
 *
 * @note 当前该方法未被 buildPlanningSystemPrompt() 直接使用，
 *       后者内联了类似的工具描述生成逻辑。保留此方法为未来重构预留接口。
 */
QString PromptBuilder::buildToolDefinitionPrompt(const QList<Agent*>& agents) const
{
    QString toolsPrompt;  // 【创建】工具描述字符串
    for (Agent* agent : agents) {  // 【遍历】所有Agent
        for (Tool* tool : agent->tools()) {  // 【遍历】每个Agent的所有工具
            toolsPrompt += tool->promptDescription() + "\n";  // 【追加】工具描述+换行
        }
    }
    return toolsPrompt;  // 【返回】所有工具的文本描述
}
