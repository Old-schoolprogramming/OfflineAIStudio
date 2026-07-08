#include "agent.h"
#include "tool.h"

Agent::Agent(QObject *parent)
    : QObject(parent)
{
}

Agent::~Agent()
{
}

QString Agent::formatToolsForPrompt() const
{
    QString result = "Available tools:\n";
    QList<Tool*> agentTools = tools();
    
    for (Tool* tool : agentTools) {
        result += tool->formatForPrompt() + "\n";
    }
    
    return result;
}
