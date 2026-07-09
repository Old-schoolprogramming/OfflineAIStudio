#include "llmclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QRegularExpression>

LlmClient::LlmClient(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_maxTokens(2048),
      m_temperature(0.7)
{
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

    sendMessages(messages, true);
}

void LlmClient::sendMessages(const QJsonArray& messages, bool stream)
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
    json["stream"] = stream;
    json["messages"] = messages;

    QJsonDocument doc(json);
    QNetworkReply* reply = m_networkManager->post(networkRequest, doc.toJson());

    m_buffer.clear();
    m_streamMode = stream;
    m_activeReply = reply;

    connect(reply, &QNetworkReply::readyRead, this, &LlmClient::onReadyRead);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void LlmClient::onReadyRead()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    m_buffer += QString::fromUtf8(reply->readAll());

    if (m_streamMode) {
        processSseStream();
    } else {
        // 非流式：buffer 累加直到 onReplyFinished 统一处理
    }
}

void LlmClient::processSseStream()
{
    QRegularExpression dataRegex(R"(data:\s*(\{[^\n]*\})\s*\n)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = dataRegex.globalMatch(m_buffer);

    int lastProcessedEnd = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString jsonStr = match.captured(1);
        lastProcessedEnd = match.capturedEnd();

        if (jsonStr.contains("[DONE]") || jsonStr.trimmed() == "[DONE]") {
            continue;
        }

        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        if (!obj.contains("choices")) continue;

        QJsonArray choices = obj["choices"].toArray();
        if (choices.isEmpty()) continue;

        QJsonObject choice = choices[0].toObject();

        if (choice.contains("delta")) {
            QJsonObject delta = choice["delta"].toObject();
            QString content = delta["content"].toString();
            if (!content.isEmpty()) {
                emit responseChunk(content);
            }
        } else if (choice.contains("message")) {
            QJsonObject message = choice["message"].toObject();
            QString content = message["content"].toString();
            if (!content.isEmpty()) {
                emit responseReceived(content);
            }
        }
    }

    if (lastProcessedEnd > 0) {
        m_buffer = m_buffer.mid(lastProcessedEnd);
    }
}

void LlmClient::onReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit responseError(reply->errorString());
        reply->deleteLater();
        emit responseCompleted();
        return;
    }

    QString remainingData = QString::fromUtf8(reply->readAll());
    if (!remainingData.isEmpty()) {
        m_buffer += remainingData;
    }

    if (m_streamMode) {
        processSseStream();
    } else {
        // 非流式：直接解析 buffer 中的 JSON
        QJsonDocument doc = QJsonDocument::fromJson(m_buffer.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("choices")) {
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject choice = choices[0].toObject();
                    QJsonObject message = choice["message"].toObject();
                    QString content = message["content"].toString();
                    if (!content.isEmpty()) {
                        emit responseReceived(content);
                    }
                }
            }
        }
    }

    m_buffer.clear();
    reply->deleteLater();
    emit responseCompleted();
}