#include "llmclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>

LlmClient::LlmClient(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_maxTokens(2048),
      m_temperature(0.7)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &LlmClient::onReplyFinished);
}

LlmClient::~LlmClient()
{
}

void LlmClient::setApiUrl(const QString& url)
{
    m_apiUrl = url;
}

void LlmClient::setModelName(const QString& modelName)
{
    m_modelName = modelName;
}

void LlmClient::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

void LlmClient::setTemperature(double temperature)
{
    m_temperature = temperature;
}

void LlmClient::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
}

QString LlmClient::apiUrl() const
{
    return m_apiUrl;
}

QString LlmClient::modelName() const
{
    return m_modelName;
}

int LlmClient::maxTokens() const
{
    return m_maxTokens;
}

double LlmClient::temperature() const
{
    return m_temperature;
}

QString LlmClient::apiKey() const
{
    return m_apiKey;
}

void LlmClient::sendPrompt(const QString& prompt, const QString& systemPrompt)
{
    QUrl apiUrl(m_apiUrl);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(apiUrl);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_apiKey.isEmpty()) {
        networkRequest.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    QJsonObject json;
    json["model"] = m_modelName;
    json["max_tokens"] = m_maxTokens;
    json["temperature"] = m_temperature;

    QJsonArray messages;
    
    if (!systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = systemPrompt;
        messages.append(systemMsg);
    }

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(userMsg);

    json["messages"] = messages;

    QJsonDocument doc(json);
    m_networkManager->post(networkRequest, doc.toJson());
}

void LlmClient::onReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit responseError(reply->errorString());
        reply->deleteLater();
        emit responseCompleted();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("choices")) {
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                if (choice.contains("message")) {
                    QJsonObject message = choice["message"].toObject();
                    QString content = message["content"].toString();
                    emit responseReceived(content);
                }
            }
        }
    }

    reply->deleteLater();
    emit responseCompleted();
}
