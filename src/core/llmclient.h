#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class LlmClient : public QObject
{
    Q_OBJECT

public:
    explicit LlmClient(QObject *parent = nullptr);
    ~LlmClient();

    void setApiUrl(const QString& url);
    void setModelName(const QString& modelName);
    void setMaxTokens(int maxTokens);
    void setTemperature(double temperature);
    void setApiKey(const QString& apiKey);

    QString apiUrl() const;
    QString modelName() const;
    int maxTokens() const;
    double temperature() const;
    QString apiKey() const;

    void sendPrompt(const QString& prompt, const QString& systemPrompt = "");

signals:
    void responseReceived(const QString& response);
    void responseError(const QString& error);
    void responseCompleted();

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_apiUrl;
    QString m_modelName;
    int m_maxTokens;
    double m_temperature;
    QString m_apiKey;
};

#endif
