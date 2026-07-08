#ifndef LLMCLIENT_H
#define LLMCLIENT_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief LLM客户端类 - 负责与本地大语言模型API通信
 *
 * 这个类封装了与本地大模型服务的HTTP通信。
 * 支持标准的OpenAI格式API接口，可以对接任何兼容该格式的模型服务，
 * 如 LM Studio、Ollama（需开启OpenAI兼容模式）、vLLM 等。
 */
class LlmClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit LlmClient(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~LlmClient();

    /**
     * @brief 设置API地址
     * @param url API的完整URL地址
     */
    void setApiUrl(const QString& url);

    /**
     * @brief 设置模型名称
     * @param modelName 模型标识名称
     */
    void setModelName(const QString& modelName);

    /**
     * @brief 设置最大生成Token数
     * @param maxTokens 最大Token数量
     */
    void setMaxTokens(int maxTokens);

    /**
     * @brief 设置生成温度
     * @param temperature 温度值（0.0-1.0），值越高创造性越强
     */
    void setTemperature(double temperature);

    /**
     * @brief 获取API地址
     * @return API URL
     */
    QString apiUrl() const;

    /**
     * @brief 获取模型名称
     * @return 模型名称
     */
    QString modelName() const;

    /**
     * @brief 获取最大Token数
     * @return 最大Token数
     */
    int maxTokens() const;

    /**
     * @brief 获取温度值
     * @return 温度值
     */
    double temperature() const;

    /**
     * @brief 发送Prompt给大模型
     * @param prompt 用户输入的提示词
     * @param systemPrompt 系统提示词（可选），用于设定AI的角色和行为
     */
    void sendPrompt(const QString& prompt, const QString& systemPrompt = "");

signals:
    /**
     * @brief 收到模型回复信号
     * @param response 模型返回的文本内容
     */
    void responseReceived(const QString& response);

    /**
     * @brief 响应出错信号
     * @param error 错误信息
     */
    void responseError(const QString& error);

    /**
     * @brief 响应完成信号（无论成功失败都会触发）
     */
    void responseCompleted();

private slots:
    /**
     * @brief 网络请求完成的回调槽函数
     * @param reply 网络回复对象
     */
    void onReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;   ///< 网络访问管理器
    QString m_apiUrl;                          ///< API地址
    QString m_modelName;                       ///< 模型名称
    int m_maxTokens;                           ///< 最大生成Token数
    double m_temperature;                      ///< 生成温度
};

#endif
