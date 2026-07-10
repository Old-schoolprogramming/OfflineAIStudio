/**
 * @file conversationmanager.h
 * @brief 对话管理器 —— 多轮对话历史的存储与检索
 *
 * @details
 * ConversationManager 负责管理用户与 AI 的多轮对话历史。
 * 它维护一个会话（Conversation）列表，每个会话包含一系列消息（Message）。
 *
 * 核心功能：
 * - 存储消息历史（system、user、assistant 三种角色）
 * - 支持多会话管理（创建、切换、删除会话）
 * - 构建符合 OpenAI 格式的消息数组（buildMessageHistory）
 * - 序列化/反序列化（toJson / loadFromJson）
 *
 * 设计说明：
 * - Message 使用 struct 定义，包含 role 和 content
 * - Conversation 使用 struct 定义，包含消息列表和标题
 * - 当前激活的会话由 m_currentConversationIndex 标识
 * - 支持多会话切换，方便用户同时处理多个话题
 */

#ifndef CONVERSATIONMANAGER_H  // 【条件编译】防止头文件被重复包含
#define CONVERSATIONMANAGER_H  // 【宏定义】标记该文件已被包含

#include <QObject>    // 【引入】Qt对象基类
#include <QVector>    // 【引入】Qt动态数组容器
#include <QString>    // 【引入】Qt字符串类
#include <QJsonArray> // 【引入】Qt JSON数组类
#include <QJsonObject>// 【引入】Qt JSON对象类

/**
 * @brief 单条消息结构
 *
 * 对应 OpenAI Chat API 的消息格式：
 * { "role": "user", "content": "你好" }
 */
struct Message {  // 【结构体】单条对话消息
    QString role;     // 【成员】消息角色："system" | "user" | "assistant"
    QString content;  // 【成员】消息内容文本
};

/**
 * @brief 单个会话结构
 *
 * 一个会话是一组相关的连续对话，有独立的上下文。
 * 例如："项目A开发"会话和"项目B文档"会话相互独立。
 */
struct Conversation {  // 【结构体】一个完整的对话会话
    QVector<Message> messages;  // 【成员】该会话的所有消息列表
    QString title;              // 【成员】会话标题，用于UI展示
};

/**
 * @brief 对话管理器
 *
 * ConversationManager 继承自 QObject，支持信号槽机制。
 * 它不直接与 LLM 通信，只负责管理对话数据，供 Orchestrator 构建请求时使用。
 */
class ConversationManager : public QObject  // 【类声明】继承QObject以支持信号槽
{
    Q_OBJECT  // 【Qt宏】启用元对象系统

public:
    /**
     * @brief 构造函数
     * @param parent 父 QObject
     *
     * 初始化时自动创建一个默认会话。
     */
    explicit ConversationManager(QObject *parent = nullptr);  // 【构造函数】explicit防止隐式转换

    /**
     * @brief 析构函数
     */
    ~ConversationManager();  // 【析构函数】释放资源

    /**
     * @brief 添加系统消息到当前会话
     * @param content 系统消息内容
     *
     * @note 系统消息通常作为对话的第一条消息，设定AI的行为规范。
     */
    void addSystemMessage(const QString& content);  // 【方法】添加系统角色消息

    /**
     * @brief 添加用户消息到当前会话
     * @param content 用户输入内容
     */
    void addUserMessage(const QString& content);  // 【方法】添加用户角色消息

    /**
     * @brief 添加助手消息到当前会话
     * @param content AI 回复内容
     */
    void addAssistantMessage(const QString& content);  // 【方法】添加助手角色消息

    /**
     * @brief 获取当前会话的所有消息
     * @return 当前会话的消息列表引用（可直接修改）
     */
    QVector<Message> messages() const;  // 【方法】获取当前会话的所有消息

    /**
     * @brief 获取当前激活的会话索引
     * @return 当前会话索引（从0开始）
     */
    int currentConversationIndex() const;  // 【方法】获取当前会话索引

    /**
     * @brief 切换到指定会话
     * @param index 会话索引
     *
     * @note 如果 index 超出范围，不做任何操作。
     */
    void switchConversation(int index);  // 【方法】切换到指定会话

    /**
     * @brief 创建新会话
     *
     * @implementation
     * 1. 创建一个新的 Conversation 对象
     * 2. 设置标题为 "新对话 N"
     * 3. 添加到会话列表
     * 4. 切换到新会话
     */
    void createNewConversation();  // 【方法】创建新对话会话

    /**
     * @brief 删除指定会话
     * @param index 要删除的会话索引
     *
     * @implementation
     * 1. 从列表中移除指定会话
     * 2. 如果删除的是当前会话，切换到前一个或第一个会话
     * 3. 如果删除后列表为空，创建一个新的默认会话
     */
    void deleteConversation(int index);  // 【方法】删除指定会话

    /**
     * @brief 构建 OpenAI 格式的消息历史数组
     * @param systemPrompt 系统提示词（作为第一条 system 消息）
     * @return 符合 OpenAI API 格式的消息数组
     *
     * @implementation
     * 1. 添加 systemPrompt 作为第一条 system 消息
     * 2. 遍历当前会话的所有消息，转换为 QJsonObject
     * 3. 返回 QJsonArray
     *
     * @note 如果当前会话已有 system 消息，systemPrompt 会作为新的第一条 system 消息插入。
     */
    QJsonArray buildMessageHistory(const QString& systemPrompt) const;  // 【方法】构建API请求用的消息数组

    /**
     * @brief 将会话数据序列化为 JSON 数组
     * @return JSON 数组，每个元素是一个会话对象
     *
     * @note 用于保存到配置文件或传输到其他地方。
     */
    QJsonArray toJson() const;  // 【方法】序列化为JSON

    /**
     * @brief 从 JSON 数组加载会话数据
     * @param data 序列化后的 JSON 数组
     *
     * @note 会清空当前所有会话，然后用 JSON 数据重建。
     */
    void loadFromJson(const QJsonArray& data);  // 【方法】从JSON加载会话数据

signals:  // 【Qt关键字】信号声明
    /**
     * @brief 会话列表发生变化
     *
     * 当创建、删除会话时发射，UI 层可据此更新会话列表视图。
     */
    void conversationsChanged();  // 【信号】通知UI会话列表已变化

    /**
     * @brief 当前会话发生变化
     *
     * 当切换会话时发射，UI 层可据此更新当前显示的对话内容。
     */
    void currentConversationChanged();  // 【信号】通知UI当前会话已变化

private:  // 【访问修饰符】私有成员
    QVector<Conversation> m_conversations;  // 【成员变量】所有会话的列表
    int m_currentConversationIndex;         // 【成员变量】当前激活的会话索引
};

#endif // CONVERSATIONMANAGER_H  // 【条件编译结束】
