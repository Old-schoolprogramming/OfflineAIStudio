#ifndef LLMCLIENT_H  // 【条件编译】防止头文件被重复包含
#define LLMCLIENT_H  // 【宏定义】标记该文件已被包含

#include <QObject>    // 【引入】Qt对象基类
#include <QString>    // 【引入】Qt字符串类
#include <QVariantMap> // 【引入】Qt变体映射类，用于键值对数据
#include <QNetworkAccessManager> // 【引入】Qt网络访问管理器，用于发送HTTP请求
#include <QNetworkReply>         // 【引入】Qt网络回复类，用于接收HTTP响应
#include <QJsonArray>            // 【引入】Qt JSON数组类
#include <QPointer>              // 【引入】Qt智能指针，会自动置空当被指向对象销毁

/**
 * @brief LLM客户端类 —— 负责与大模型API进行网络通信
 *
 * @details
 * LlmClient封装了所有与LLM（大语言模型）API交互的网络操作，包括：
 * - 设置API连接参数（URL、模型名、密钥等）
 * - 发送Prompt请求（支持普通对话和消息数组两种格式）
 * - 处理流式（SSE）和非流式响应
 * - 解析JSON格式的API返回数据
 *
 * 网络通信基于Qt的QNetworkAccessManager，采用异步信号槽机制，
 * 不会阻塞UI线程。
 */
class LlmClient : public QObject  // 【类声明】继承QObject以使用信号槽机制
{
    Q_OBJECT  // 【Qt宏】启用元对象系统，支持信号槽和反射

public:
    /**
     * @brief 构造函数
     * @param parent 父QObject对象，用于Qt内存管理
     *
     * 初始化网络管理器、默认参数（maxTokens=8192, temperature=0.7）
     */
    explicit LlmClient(QObject *parent = nullptr);  // 【构造函数】explicit防止隐式转换

    /**
     * @brief 析构函数
     */
    ~LlmClient();  // 【析构函数】清理资源

    /**
     * @brief 设置API服务地址
     * @param url API的HTTP端点地址，如 "http://localhost:1234/v1/chat/completions"
     */
    void setApiUrl(const QString& url);  // 【方法】配置LLM服务的URL

    /**
     * @brief 设置模型名称
     * @param modelName 要调用的模型标识，如 "qwen2.5-7b-instruct"
     */
    void setModelName(const QString& modelName);  // 【方法】配置使用的AI模型

    /**
     * @brief 设置最大生成token数
     * @param maxTokens 单次响应最多生成的token数量
     *
     * @note token是模型的基本处理单元，一个汉字约1-2个token。
     *       默认值8192可以生成长篇文档。
     */
    void setMaxTokens(int maxTokens);  // 【方法】限制模型输出长度

    /**
     * @brief 设置采样温度
     * @param temperature 0.0-2.0之间的值，控制输出随机性
     *
     * @details
     * - 温度越低（如0.2），输出越确定、保守
     * - 温度越高（如0.8），输出越多样、有创意
     * - 默认0.7，平衡确定性和多样性
     */
    void setTemperature(double temperature);  // 【方法】控制模型输出的随机程度

    /**
     * @brief 设置API密钥
     * @param apiKey 访问LLM服务的认证密钥
     *
     * @note 如果LLM服务在本地运行且无需认证，可以留空。
     */
    void setApiKey(const QString& apiKey);  // 【方法】配置访问密钥

    /**
     * @brief 获取当前API地址
     * @return API URL字符串
     */
    QString apiUrl() const;  // 【方法】获取当前配置的API地址

    /**
     * @brief 获取当前模型名称
     * @return 模型名称字符串
     */
    QString modelName() const;  // 【方法】获取当前配置的模型名

    /**
     * @brief 获取当前最大token数
     * @return maxTokens值
     */
    int maxTokens() const;  // 【方法】获取当前最大输出长度限制

    /**
     * @brief 获取当前温度设置
     * @return temperature值
     */
    double temperature() const;  // 【方法】获取当前温度参数

    /**
     * @brief 获取当前API密钥
     * @return API密钥字符串
     */
    QString apiKey() const;  // 【方法】获取当前配置的密钥

    /**
     * @brief 发送简单的Prompt请求（兼容模式）
     * @param prompt 用户输入的提示词
     * @param systemPrompt 系统提示词（可选），用于设定AI角色和行为规范
     *
     * @implementation
     * 将prompt和systemPrompt封装为messages数组格式，
     * 然后调用sendMessages()发送，默认启用流式响应。
     */
    void sendPrompt(const QString& prompt, const QString& systemPrompt = "");  // 【方法】简单Prompt发送接口

    /**
     * @brief 发送消息数组请求（标准模式）
     * @param messages 符合OpenAI格式的消息数组，每个元素含role和content字段
     * @param stream 是否启用流式响应（SSE）
     *
     * @implementation
     * 1. 构造JSON请求体（model, messages, max_tokens, temperature, stream）
     * 2. 设置HTTP头部（Content-Type, Authorization）
     * 3. 发送POST请求
     * 4. 连接 QNetworkReply 的信号到处理槽
     */
    void sendMessages(const QJsonArray& messages, bool stream = true);  // 【方法】标准消息发送接口

signals:  // 【Qt关键字】信号声明区域，用于通知外部组件事件
    /**
     * @brief 收到完整响应
     * @param response LLM返回的完整文本内容
     *
     * @note 非流式模式下，在请求完成后一次性发射；
     *       流式模式下，在所有chunk接收完毕后也会发射一次完整文本。
     */
    void responseReceived(const QString& response);  // 【信号】通知上层收到完整AI回复

    /**
     * @brief 收到流式响应片段
     * @param chunk LLM返回的文本片段（通常是一个句子或几个字）
     *
     * @note 仅在流式模式下发射，可用于实时显示打字机效果。
     */
    void responseChunk(const QString& chunk);  // 【信号】流式模式下逐段输出AI回复

    /**
     * @brief 请求发生错误
     * @param error 错误描述字符串
     *
     * 可能的错误：网络连接失败、API返回错误、超时等。
     */
    void responseError(const QString& error);  // 【信号】通知上层请求出错

    /**
     * @brief 响应接收完成
     *
     * 无论成功或失败，在请求处理完毕后都会发射此信号，
     * 可用于清理资源或重置UI状态。
     */
    void responseCompleted();  // 【信号】通知上层本次请求已结束

private slots:  // 【Qt关键字】私有槽函数，处理网络事件
    /**
     * @brief HTTP请求完成的处理槽
     * @param reply 网络回复对象
     *
     * @implementation
     * 1. 检查reply是否有网络错误
     * 2. 读取剩余数据到buffer
     * 3. 如果是流式模式，最后一次解析SSE数据
     * 4. 如果是非流式模式，解析完整JSON响应
     * 5. 发射responseCompleted信号
     * 6. 释放reply对象
     */
    void onReplyFinished(QNetworkReply* reply);  // 【槽】HTTP请求完成后的处理

    /**
     * @brief 有数据可读的处理槽
     *
     * @implementation
     * 当 QNetworkReply 接收到新数据时自动触发。
     * 将数据追加到buffer，如果是流式模式则实时解析SSE数据并发射chunk信号。
     */
    void onReadyRead();  // 【槽】收到网络数据时的实时处理

private:  // 【访问修饰符】私有成员
    /**
     * @brief 解析SSE（Server-Sent Events）流数据
     *
     * @implementation
     * SSE格式：每行以 "data: " 开头，后跟JSON对象。
     * 使用正则表达式从buffer中提取所有data行，
     * 解析JSON中的delta.content字段，发射responseChunk信号。
     * 已处理的数据从buffer中移除，保留未完整接收的数据。
     */
    void processSseStream();  // 【私有方法】解析流式响应数据

    QNetworkAccessManager* m_networkManager;  // 【成员变量】网络访问管理器，负责发送所有HTTP请求
    QString m_apiUrl;        // 【成员变量】API服务地址
    QString m_modelName;     // 【成员变量】模型名称
    int m_maxTokens;         // 【成员变量】最大输出token数
    double m_temperature;    // 【成员变量】采样温度
    QString m_apiKey;        // 【成员变量】API认证密钥
    QString m_buffer;        // 【成员变量】网络数据缓冲区，累积接收到的原始数据
    bool m_streamMode;       // 【成员变量】当前是否处于流式模式
    QPointer<QNetworkReply> m_activeReply;  // 【成员变量】当前活动的网络回复对象，QPointer会自动管理生命周期
};

#endif  // 【条件编译结束】LLMCLIENT_H
