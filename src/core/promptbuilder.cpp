#include "promptbuilder.h"
#include "agent.h"

PromptBuilder::PromptBuilder()
    : m_toolCallFormat("<function_name>(param1=value1, param2=value2, ...)")
{
}

PromptBuilder::~PromptBuilder()
{
}

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

QString PromptBuilder::buildUserPrompt(const QString& userQuery)
{
    return "User query: " + userQuery + "\n";
}

QString PromptBuilder::buildToolResultPrompt(const QString& toolName, const QString& result)
{
    return "Tool " + toolName + " executed successfully. Result:\n" + result + "\n";
}

QString PromptBuilder::buildFullPrompt(const QString& userQuery, const QString& role, const QList<Agent*>& agents)
{
    QString prompt = buildSystemPrompt(role, agents);
    prompt += "\n" + buildUserPrompt(userQuery);
    return prompt;
}
