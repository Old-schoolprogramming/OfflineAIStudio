#include "llmclient.h"  // 【引入】自己的头文件
#include <QJsonDocument> // 【引入】Qt JSON文档类，用于序列化和解析JSON
#include <QJsonObject>   // 【引入】Qt JSON对象类
#include <QJsonArray>    // 【引入】Qt JSON数组类
#include <QNetworkRequest> // 【引入】Qt网络请求类
#include <QUrl>          // 【引入】Qt URL类
#include <QRegularExpression> // 【引入】Qt正则表达式类，用于解析SSE数据

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * @implementation
 * 初始化网络管理器和默认参数：
 * - m_networkManager: 创建新的网络访问管理器，parent设为本对象
 * - m_maxTokens: 8192，足够生成长文档
 * - m_temperature: 0.7，平衡确定性和创造性
 */
LlmClient::LlmClient(QObject *parent)
    : QObject(parent),              // 【初始化】调用父类构造函数
      m_networkManager(new QNetworkAccessManager(this)),  // 【初始化】创建网络管理器，以this为父对象
      m_maxTokens(8192),           // 【初始化】默认最大token数8192
      m_temperature(0.7)           // 【初始化】默认温度0.7
{
    // 【说明】构造函数体为空，所有初始化在初始化列表中完成
}

/**
 * @brief 析构函数
 *
 * @note m_networkManager是this的子对象，由Qt自动销毁。
 *       m_activeReply是QPointer，会自动置空，不拥有所有权。
 */
LlmClient::~LlmClient()
{
    // 【说明】无需手动释放资源，Qt对象树会自动处理
}

/**
 * @brief 设置API服务地址
 * @param url API的HTTP端点地址
 */
void LlmClient::setApiUrl(const QString& url)
{
    m_apiUrl = url;  // 【保存】记录API地址
}

/**
 * @brief 设置模型名称
 * @param modelName 模型标识字符串
 */
void LlmClient::setModelName(const QString& modelName)
{
    m_modelName = modelName;  // 【保存】记录模型名称
}

/**
 * @brief 设置最大生成token数
 * @param maxTokens 最大token数
 */
void LlmClient::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;  // 【保存】记录最大token限制
}

/**
 * @brief 设置采样温度
 * @param temperature 温度值（0.0-2.0）
 */
void LlmClient::setTemperature(double temperature)
{
    m_temperature = temperature;  // 【保存】记录温度参数
}

/**
 * @brief 设置API密钥
 * @param apiKey 认证密钥
 */
void LlmClient::setApiKey(const QString& apiKey)
{
    m_apiKey = apiKey;  // 【保存】记录API密钥
}

/**
 * @brief 获取当前API地址
 * @return API URL字符串
 */
QString LlmClient::apiUrl() const
{
    return m_apiUrl;  // 【返回】当前配置的API地址
}

/**
 * @brief 获取当前模型名称
 * @return 模型名称字符串
 */
QString LlmClient::modelName() const
{
    return m_modelName;  // 【返回】当前配置的模型名
}

/**
 * @brief 获取当前最大token数
 * @return maxTokens值
 */
int LlmClient::maxTokens() const
{
    return m_maxTokens;  // 【返回】当前最大输出长度限制
}

/**
 * @brief 获取当前温度设置
 * @return temperature值
 */
double LlmClient::temperature() const
{
    return m_temperature;  // 【返回】当前温度参数
}

/**
 * @brief 获取当前API密钥
 * @return API密钥字符串
 */
QString LlmClient::apiKey() const
{
    return m_apiKey;  // 【返回】当前配置的密钥
}

/**
 * @brief 发送简单的Prompt请求
 * @param prompt 用户提示词
 * @param systemPrompt 系统提示词（可选）
 *
 * @implementation
 * 将输入封装为OpenAI格式的messages数组：
 * - 如果systemPrompt非空，添加{"role":"system","content":systemPrompt}
 * - 添加{"role":"user","content":prompt}
 * 然后调用sendMessages()发送，默认启用流式模式。
 */
void LlmClient::sendPrompt(const QString& prompt, const QString& systemPrompt)
{
    QJsonArray messages;  // 【创建】消息数组

    // 【条件添加】如果提供了系统提示词，添加到数组开头
    if (!systemPrompt.isEmpty()) {
        QJsonObject systemMsg;                    // 【创建】系统消息对象
        systemMsg["role"] = "system";             // 【设置】角色为system
        systemMsg["content"] = systemPrompt;      // 【设置】内容
        messages.append(systemMsg);               // 【追加】添加到数组
    }

    QJsonObject userMsg;                          // 【创建】用户消息对象
    userMsg["role"] = "user";                     // 【设置】角色为user
    userMsg["content"] = prompt;                  // 【设置】用户输入内容
    messages.append(userMsg);                     // 【追加】添加到数组

    sendMessages(messages, true);                 // 【调用】发送消息，启用流式模式
}

/**
 * @brief 发送消息数组请求
 * @param messages OpenAI格式的消息数组
 * @param stream 是否启用流式响应
 *
 * @implementation
 * 1. 构造请求URL和HTTP请求对象
 * 2. 设置Content-Type为application/json
 * 3. 如果apiKey非空，添加Authorization头（Bearer token）
 * 4. 构造JSON请求体：model, max_tokens, temperature, stream, messages
 * 5. 发送POST请求
 * 6. 连接reply的信号到处理槽
 */
void LlmClient::sendMessages(const QJsonArray& messages, bool stream)
{
    QUrl apiUrl(m_apiUrl);                        // 【创建】URL对象
    QNetworkRequest networkRequest;               // 【创建】网络请求对象
    networkRequest.setUrl(apiUrl);                // 【设置】请求地址
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");  // 【设置】内容类型为JSON

    // 【条件添加】如果设置了API密钥，添加认证头
    if (!m_apiKey.isEmpty()) {
        networkRequest.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());  // 【设置】Bearer token认证
    }

    QJsonObject json;                             // 【创建】请求体JSON对象
    json["model"] = m_modelName;                  // 【设置】模型名称
    json["max_tokens"] = m_maxTokens;             // 【设置】最大token数
    json["temperature"] = m_temperature;          // 【设置】温度参数
    json["stream"] = stream;                      // 【设置】是否流式
    json["messages"] = messages;                  // 【设置】消息数组

    QJsonDocument doc(json);                      // 【创建】JSON文档
    // 【发送】POST请求，请求体为JSON字符串
    QNetworkReply* reply = m_networkManager->post(networkRequest, doc.toJson());

    m_buffer.clear();                             // 【重置】清空数据缓冲区
    m_streamMode = stream;                        // 【记录】当前模式
    m_activeReply = reply;                        // 【记录】当前活动的回复对象

    // 【信号连接】有数据可读时触发onReadyRead
    connect(reply, &QNetworkReply::readyRead, this, &LlmClient::onReadyRead);
    // 【信号连接】请求完成时触发onReplyFinished，使用lambda传递reply指针
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

/**
 * @brief 有数据可读时的处理槽
 *
 * @implementation
 * 1. 获取发送信号的reply对象
 * 2. 读取所有可用数据并追加到buffer
 * 3. 如果是流式模式，调用processSseStream实时解析
 * 4. 非流式模式下，数据累积到buffer等onReplyFinished统一处理
 */
void LlmClient::onReadyRead()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());  // 【获取】触发信号的reply对象
    if (!reply) return;  // 【保护】如果转换失败则直接返回

    m_buffer += QString::fromUtf8(reply->readAll());  // 【读取】将所有可用数据追加到buffer

    if (m_streamMode) {  // 【分支】如果是流式模式
        processSseStream();  // 【调用】实时解析SSE数据
    } else {
        // 【非流式】buffer累加直到onReplyFinished统一处理
    }
}

/**
 * @brief 解析SSE流数据
 *
 * @implementation
 * SSE格式示例：
 *   data: {"choices":[{"delta":{"content":"Hello"}}]}\n\n
 *   data: [DONE]\n\n
 *
 * 解析步骤：
 * 1. 使用正则表达式匹配所有 data: {...} 行
 * 2. 跳过[DONE]标记
 * 3. 解析JSON，提取choices[0].delta.content或choices[0].message.content
 * 4. 发射responseChunk信号
 * 5. 从buffer中移除已处理的数据
 */
void LlmClient::processSseStream()
{
    // 【正则】匹配 "data: " 后跟JSON对象，直到换行
    QRegularExpression dataRegex(R"(data:\s*(\{[^\n]*\})\s*\n)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator it = dataRegex.globalMatch(m_buffer);  // 【全局匹配】查找buffer中所有匹配

    int lastProcessedEnd = 0;  // 【记录】最后处理到的位置
    while (it.hasNext()) {     // 【循环】处理每个匹配
        QRegularExpressionMatch match = it.next();  // 【获取】下一个匹配
        QString jsonStr = match.captured(1);        // 【提取】捕获组1：JSON字符串
        lastProcessedEnd = match.capturedEnd();     // 【记录】匹配结束位置

        // 【跳过】结束标记[DONE]
        if (jsonStr.contains("[DONE]") || jsonStr.trimmed() == "[DONE]") {
            continue;
        }

        // 【解析】将JSON字符串解析为QJsonDocument
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isObject()) continue;  // 【跳过】解析失败或不是对象

        QJsonObject obj = doc.object();  // 【获取】顶层JSON对象
        if (!obj.contains("choices")) continue;  // 【跳过】没有choices字段

        QJsonArray choices = obj["choices"].toArray();  // 【获取】choices数组
        if (choices.isEmpty()) continue;  // 【跳过】choices为空

        QJsonObject choice = choices[0].toObject();  // 【获取】第一个choice

        // 【分支A】流式响应：提取delta.content
        if (choice.contains("delta")) {
            QJsonObject delta = choice["delta"].toObject();  // 【获取】delta对象
            QString content = delta["content"].toString();   // 【获取】内容文本
            if (!content.isEmpty()) {
                emit responseChunk(content);  // 【信号】发射文本片段
            }
        }
        // 【分支B】非流式响应：提取message.content
        else if (choice.contains("message")) {
            QJsonObject message = choice["message"].toObject();  // 【获取】message对象
            QString content = message["content"].toString();     // 【获取】内容文本
            if (!content.isEmpty()) {
                emit responseReceived(content);  // 【信号】发射完整响应
            }
        }
    }

    // 【清理】如果处理了数据，从buffer中移除已处理部分
    if (lastProcessedEnd > 0) {
        m_buffer = m_buffer.mid(lastProcessedEnd);
    }
}

/**
 * @brief HTTP请求完成的处理槽
 * @param reply 网络回复对象
 *
 * @implementation
 * 1. 检查reply是否有网络错误
 * 2. 读取剩余数据
 * 3. 流式模式：最后一次解析SSE
 * 4. 非流式模式：解析完整JSON，提取content
 * 5. 清空buffer，释放reply，发射responseCompleted
 */
void LlmClient::onReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {  // 【检查】是否有网络错误
        emit responseError(reply->errorString());    // 【信号】通知上层错误信息
        reply->deleteLater();                        // 【延迟释放】安全释放reply
        emit responseCompleted();                    // 【信号】通知请求结束
        return;
    }

    // 【读取】获取reply中剩余的所有数据
    QString remainingData = QString::fromUtf8(reply->readAll());
    if (!remainingData.isEmpty()) {  // 【判断】如果有剩余数据
        m_buffer += remainingData;   // 【追加】添加到buffer
    }

    if (m_streamMode) {  // 【分支】流式模式
        processSseStream();  // 【调用】最后一次解析SSE数据
    } else {
        // 【非流式】直接解析buffer中的完整JSON
        QJsonDocument doc = QJsonDocument::fromJson(m_buffer.toUtf8());  // 【解析】JSON文档
        if (doc.isObject()) {  // 【判断】是否为JSON对象
            QJsonObject obj = doc.object();  // 【获取】顶层对象
            if (obj.contains("choices")) {   // 【判断】是否包含choices
                QJsonArray choices = obj["choices"].toArray();  // 【获取】choices数组
                if (!choices.isEmpty()) {    // 【判断】choices非空
                    QJsonObject choice = choices[0].toObject();  // 【获取】第一个choice
                    QJsonObject message = choice["message"].toObject();  // 【获取】message对象
                    QString content = message["content"].toString();     // 【获取】内容
                    if (!content.isEmpty()) {
                        emit responseReceived(content);  // 【信号】发射完整响应
                    }
                }
            }
        }
    }

    m_buffer.clear();         // 【重置】清空缓冲区
    reply->deleteLater();     // 【延迟释放】安全释放reply对象
    emit responseCompleted(); // 【信号】通知上层本次请求已完全结束
}
