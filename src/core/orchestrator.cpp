#include "orchestrator.h"
#include "agent.h"
#include "tool.h"
#include <QDebug>
#include <QRegularExpression>

Orchestrator::Orchestrator(QObject *parent)
    : Agent(parent),
      m_llmClient(nullptr),
      m_isProcessing(false)
{
}

Orchestrator::~Orchestrator()
{
}

QString Orchestrator::name() const
{
    return "Orchestrator";
}

QString Orchestrator::description() const
{
    return "总控Agent，负责协调调用其他Agent和工具";
}

Agent::AgentType Orchestrator::type() const
{
    return OrchestratorType;
}

QList<Tool*> Orchestrator::tools() const
{
    QList<Tool*> allTools;
    for (Agent* agent : m_agents) {
        allTools.append(agent->tools());
    }
    return allTools;
}

QVariantMap Orchestrator::executeTool(const QString& toolName, const QVariantMap& args)
{
    Agent* agent = findAgentForTool(toolName);
    if (agent) {
        return agent->executeTool(toolName, args);
    }
    
    QVariantMap result;
    result["success"] = false;
    result["error"] = "Tool not found: " + toolName;
    return result;
}

void Orchestrator::setLlmClient(LlmClient* client)
{
    m_llmClient = client;
    connect(m_llmClient, &LlmClient::responseReceived, this, &Orchestrator::onLlmResponse);
    connect(m_llmClient, &LlmClient::responseError, this, &Orchestrator::onLlmError);
}

void Orchestrator::addAgent(Agent* agent)
{
    m_agents.append(agent);
}

void Orchestrator::processQuery(const QString& query)
{
    if (!m_llmClient || m_isProcessing) {
        return;
    }

    m_isProcessing = true;
    m_currentQuery = query;

    QString systemPrompt = m_promptBuilder.buildSystemPrompt("AI Assistant", m_agents);
    QString userPrompt = m_promptBuilder.buildUserPrompt(query);

    m_llmClient->sendPrompt(userPrompt, systemPrompt);
}

void Orchestrator::onLlmResponse(const QString& response)
{
    emit messageReceived(response);

    QString toolCall = parseToolCall(response);
    if (!toolCall.isEmpty()) {
        QString toolName = extractToolName(toolCall);
        QVariantMap args = extractToolArgs(toolCall);

        emit agentSelected(toolName);

        QVariantMap result = executeTool(toolName, args);
        
        if (result.value("success", false).toBool()) {
            QString resultPrompt = m_promptBuilder.buildToolResultPrompt(toolName, result.value("result").toString());
            m_llmClient->sendPrompt(resultPrompt, m_promptBuilder.buildSystemPrompt("AI Assistant", m_agents));
        } else {
            emit messageReceived("工具执行失败: " + result.value("error").toString());
            m_isProcessing = false;
        }
    } else {
        m_isProcessing = false;
    }
}

void Orchestrator::onLlmError(const QString& error)
{
    emit messageReceived("LLM错误: " + error);
    m_isProcessing = false;
}

QString Orchestrator::parseToolCall(const QString& response)
{
    QRegularExpression regex(R"(<(\w+)\(([^)]+)\)>)");
    QRegularExpressionMatch match = regex.match(response);
    
    if (match.hasMatch()) {
        return match.captured(0);
    }
    
    return "";
}

QString Orchestrator::extractToolName(const QString& toolCall)
{
    QRegularExpression regex(R"(<(\w+)\()");
    QRegularExpressionMatch match = regex.match(toolCall);
    
    if (match.hasMatch()) {
        return match.captured(1);
    }
    
    return "";
}

QVariantMap Orchestrator::extractToolArgs(const QString& toolCall)
{
    QVariantMap args;
    QRegularExpression regex(R"(\(([^)]+)\))");
    QRegularExpressionMatch match = regex.match(toolCall);
    
    if (match.hasMatch()) {
        QString paramsStr = match.captured(1);
        QStringList params = paramsStr.split(",");
        
        for (const QString& param : params) {
            QStringList parts = param.split("=");
            if (parts.size() == 2) {
                QString key = parts[0].trimmed();
                QString value = parts[1].trimmed();
                value = value.remove("\"");
                args[key] = value;
            }
        }
    }
    
    return args;
}

Agent* Orchestrator::findAgentForTool(const QString& toolName)
{
    for (Agent* agent : m_agents) {
        QList<Tool*> tools = agent->tools();
        for (Tool* tool : tools) {
            if (tool->name() == toolName) {
                return agent;
            }
        }
    }
    return nullptr;
}
