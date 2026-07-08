/**
 * @file llmclient.cpp
 * @brief LLM客户端实现
 *
 * @details
 * LlmClient负责与本地或远程大语言模型进行HTTP通信，
 * 实现OpenAI兼容的API协议，支持流式和非流式响应。
 * 该类封装了QNetworkAccessManager，提供同步/异步的请求接口。
 */

#include "llmclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 初始化网络管理器和默认参数。
 * 默认maxTokens为2048，temperature为0.7。
 * 连接QNetworkAccessManager::finished信号到onReplyFinished槽函数。
 */
LlmClient::LlmClient(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_maxTokens(2048),
      m_temperature(0.7)
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &LlmClient::onReplyFinished);
}

/**
 * @brief 析构函数
 */
LlmClient::~LlmClient()
{
}

/**
 * @brief 设置API URL
 * @param url LLM服务端点地址，例如 http://localhost:11434/v1/chat/completions
 *
 * 该URL应为完整的OpenAI兼容API地址。
 */
void LlmClient::setApiUrl(const QString& url)
{
    m_apiUrl = url;
}

/**
 * @brief 设置模型名称
 * @param modelName 模型标识名称，例如 "qwen2.5", "gpt-4o"
 */
void LlmClient::setModelName(const QString& modelName)
{
    m_modelName = modelName;
}

/**
 * @brief 设置最大生成令牌数
 * @param maxTokens 单次响应的最大token数量，默认2048
 *
 * 较大的值允许更长的回复，但会增加响应时间和资源消耗。
 */
void LlmClient::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

/**
 * @brief 设置采样温度
 * @param temperature 温度参数，范围通常为0.0-2.0，默认0.7
 *
 * 较低的值（如0.2）使输出更确定性，较高的值（如0.9）增加创造性。
 */
void LlmClient::setTemperature(double temperature)
{
    m_temperature = temperature;
}

/**
 * @brief 设置API密钥
 * @param apiKey 认证用的Bearer Token
 *
 * 若API端点需要认证，通过此接口设置Authorization头。
 * 空字符串表示不发送认证头（适用于本地无认证服务）。
 */
void LlmClient::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;
}

/**
 * @brief 获取当前API URL
 * @return API端点地址
 */
QString LlmClient::apiUrl() const
{
    return m_apiUrl;
}

/**
 * @brief 获取当前模型名称
 * @return 模型标识字符串
 */
QString LlmClient::modelName() const
{
    return m_modelName;
}

/**
 * @brief 获取最大令牌数设置
 * @return 当前maxTokens值
 */
int LlmClient::maxTokens() const
{
    return m_maxTokens;
}

/**
 * @brief 获取温度参数
 * @return 当前temperature值
 */
double LlmClient::temperature() const
{
    return m_temperature;
}

/**
 * @brief 获取API密钥
 * @return 当前设置的API密钥（可能为空）
 */
QString LlmClient::apiKey() const
{
    return m_apiKey;
}

/**
 * @brief 发送对话提示词
 * @param prompt 用户输入的提示内容
 * @param systemPrompt 系统提示词（可为空）
 *
 * @details
 * 构造OpenAI Chat Completions格式的JSON请求体，包含：
 * - model: 当前设置的模型名称
 * - max_tokens: 最大生成令牌数
 * - temperature: 采样温度
 * - messages: 消息数组，可选包含system角色消息和user角色消息
 *
 * 若设置了API密钥，会在HTTP头中添加 "Authorization: Bearer <apiKey>"。
 * 请求通过POST方法异步发送，响应通过responseReceived信号返回。
 */
void LlmClient::sendPrompt(const QString& prompt, const QString& systemPrompt)
{
    QUrl apiUrl(m_apiUrl);
    QNetworkRequest networkRequest;
    networkRequest.setUrl(apiUrl);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 若API密钥非空，添加Bearer认证头
    if (!m_apiKey.isEmpty()) {
        networkRequest.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    // 构建请求JSON主体
    QJsonObject json;
    json["model"] = m_modelName;
    json["max_tokens"] = m_maxTokens;
    json["temperature"] = m_temperature;

    QJsonArray messages;
    
    // 若提供系统提示词，添加system角色消息
    if (!systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = systemPrompt;
        messages.append(systemMsg);
    }

    // 添加用户消息
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(userMsg);

    json["messages"] = messages;

    // 序列化并发送POST请求
    QJsonDocument doc(json);
    m_networkManager->post(networkRequest, doc.toJson());
}

/**
 * @brief 网络请求完成回调槽函数
 * @param reply QNetworkReply对象，由信号传入
 *
 * @details
 * 处理HTTP响应的完整流程：
 * 1. 检查网络错误：若有错误，发射responseError信号
 * 2. 读取响应字节流并解析为JSON
 * 3. 按OpenAI响应格式提取choices[0].message.content
 * 4. 发射responseReceived信号携带解析出的文本内容
 * 5. 最后发射responseCompleted信号表示本次请求结束
 *
 * @note 无论成功或失败，都会发射responseCompleted信号。
 *       reply对象在使用完毕后通过deleteLater安全释放。
 */
void LlmClient::onReplyFinished(QNetworkReply* reply)
{
    // 检查网络层错误
    if (reply->error() != QNetworkReply::NoError) {
        emit responseError(reply->errorString());
        reply->deleteLater();
        emit responseCompleted();
        return;
    }

    // 读取全部响应数据并解析JSON
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    // 按OpenAI格式解析响应体，提取content字段
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

    // 释放reply并通知请求完成
    reply->deleteLater();
    emit responseCompleted();
}
