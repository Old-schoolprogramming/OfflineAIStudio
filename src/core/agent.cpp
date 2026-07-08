/**
 * @file agent.cpp
 * @brief Agent基类实现
 *
 * @details
 * Agent是智能代理系统的抽象基类，所有具体代理（如FileAgent、ComputerAgent）
 * 均继承自此类。该类定义了代理的基本接口：名称、描述、类型、工具列表，
 * 并提供了将工具格式化为提示词文本的通用实现。
 */

#include "agent.h"
#include "tool.h"

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 初始化Agent基类，设置对象树关系。
 */
Agent::Agent(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief 析构函数
 */
Agent::~Agent()
{
}

/**
 * @brief 将当前代理的所有工具格式化为提示词文本
 * @return 格式化后的工具描述字符串
 *
 * @details
 * 生成可供LLM理解的工具说明文本，格式如下：
 * @code
 * Available tools:
 * - toolName: toolDescription
 *   Parameters:
 *     - paramName (type) [required]: paramDescription
 * @endcode
 *
 * 通过调用tools()获取当前代理的工具列表，
 * 然后遍历每个Tool并调用其formatForPrompt()方法拼接结果。
 */
QString Agent::formatToolsForPrompt() const
{
    QString result = "Available tools:\n";
    QList<Tool*> agentTools = tools();
    
    // 遍历所有工具，拼接格式化的工具描述
    for (Tool* tool : agentTools) {
        result += tool->formatForPrompt() + "\n";
    }
    
    return result;
}
