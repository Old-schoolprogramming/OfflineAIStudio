/**
 * @file conversationmanager.cpp
 * @brief 对话管理器实现
 *
 * @details
 * ConversationManager 管理多个对话会话，支持多轮上下文。
 * 每个会话独立维护消息历史，切换会话时上下文完全隔离。
 */

#include "conversationmanager.h"  // 【引入】自己的头文件

/**
 * @brief 构造函数
 * @param parent 父 QObject
 *
 * @implementation
 * 1. 初始化会话列表为空
 * 2. 设置当前会话索引为 -1（表示无激活会话）
 * 3. 调用 createNewConversation() 创建默认会话
 */
ConversationManager::ConversationManager(QObject *parent)
    : QObject(parent),                    // 【初始化】调用父类构造函数
      m_currentConversationIndex(-1)      // 【初始化】初始无激活会话
{
    createNewConversation();              // 【调用】创建默认会话
}

/**
 * @brief 析构函数
 *
 * @note QVector 会自动管理其元素的内存，无需手动释放。
 */
ConversationManager::~ConversationManager()
{
    // 【说明】Qt容器自动管理内存，无需手动释放
}

/**
 * @brief 添加系统消息到当前会话
 * @param content 系统消息内容
 */
void ConversationManager::addSystemMessage(const QString& content)
{
    // 【保护】如果没有激活的会话，直接返回
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return;
    }
    // 【创建】构造Message对象
    Message msg;
    msg.role = "system";      // 【设置】角色为系统
    msg.content = content;    // 【设置】内容
    // 【追加】添加到当前会话的消息列表
    m_conversations[m_currentConversationIndex].messages.append(msg);
}

/**
 * @brief 添加用户消息到当前会话
 * @param content 用户输入内容
 */
void ConversationManager::addUserMessage(const QString& content)
{
    // 【保护】检查当前会话索引是否有效
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return;
    }
    Message msg;
    msg.role = "user";        // 【设置】角色为用户
    msg.content = content;    // 【设置】内容
    m_conversations[m_currentConversationIndex].messages.append(msg);  // 【追加】添加消息
}

/**
 * @brief 添加助手消息到当前会话
 * @param content AI 回复内容
 */
void ConversationManager::addAssistantMessage(const QString& content)
{
    // 【保护】检查当前会话索引是否有效
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return;
    }
    Message msg;
    msg.role = "assistant";   // 【设置】角色为助手
    msg.content = content;    // 【设置】内容
    m_conversations[m_currentConversationIndex].messages.append(msg);  // 【追加】添加消息
}

/**
 * @brief 获取当前会话的所有消息
 * @return 当前会话的消息列表
 */
QVector<Message> ConversationManager::messages() const
{
    // 【保护】检查当前会话索引是否有效
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return QVector<Message>();  // 【返回】空列表
    }
    return m_conversations[m_currentConversationIndex].messages;  // 【返回】当前会话的消息列表
}

/**
 * @brief 获取当前激活的会话索引
 * @return 当前会话索引
 */
int ConversationManager::currentConversationIndex() const
{
    return m_currentConversationIndex;  // 【返回】当前会话索引
}

/**
 * @brief 切换到指定会话
 * @param index 会话索引
 */
void ConversationManager::switchConversation(int index)
{
    // 【保护】检查索引是否有效
    if (index < 0 || index >= m_conversations.size()) {
        return;  // 【返回】索引无效，不做切换
    }
    m_currentConversationIndex = index;       // 【设置】更新当前会话索引
    emit currentConversationChanged();        // 【信号】通知UI当前会话已变化
}

/**
 * @brief 创建新会话
 *
 * @implementation
 * 1. 创建新的 Conversation 对象
 * 2. 标题设为 "新对话 N"（N 为会话数量+1）
 * 3. 添加到列表末尾
 * 4. 切换到新创建的会话
 */
void ConversationManager::createNewConversation()
{
    Conversation conv;  // 【创建】新的会话对象
    conv.title = QString("新对话 %1").arg(m_conversations.size() + 1);  // 【设置】自动生成标题
    m_conversations.append(conv);              // 【追加】添加到会话列表
    m_currentConversationIndex = m_conversations.size() - 1;  // 【设置】切换到新会话
    emit conversationsChanged();               // 【信号】通知UI会话列表变化
    emit currentConversationChanged();         // 【信号】通知UI当前会话变化
}

/**
 * @brief 删除指定会话
 * @param index 要删除的会话索引
 *
 * @implementation
 * 1. 检查索引有效性
 * 2. 从列表中移除指定会话
 * 3. 调整当前会话索引：
 *    - 如果删除的是当前会话且后面还有会话，保持当前索引（指向原来的下一个）
 *    - 如果删除的是最后一个，索引减1
 * 4. 如果列表为空，创建新的默认会话
 */
void ConversationManager::deleteConversation(int index)
{
    // 【保护】检查索引是否有效
    if (index < 0 || index >= m_conversations.size()) {
        return;  // 【返回】索引无效
    }

    m_conversations.remove(index);  // 【移除】从列表中删除指定会话

    // 【调整】更新当前会话索引
    if (m_currentConversationIndex >= m_conversations.size()) {
        m_currentConversationIndex = m_conversations.size() - 1;  // 【设置】指向最后一个
    }
    if (m_currentConversationIndex < 0 && !m_conversations.isEmpty()) {
        m_currentConversationIndex = 0;  // 【设置】指向第一个
    }

    // 【判断】如果列表为空，创建默认会话
    if (m_conversations.isEmpty()) {
        createNewConversation();  // 【调用】创建新会话
    } else {
        emit conversationsChanged();       // 【信号】通知UI会话列表变化
        emit currentConversationChanged(); // 【信号】通知UI当前会话变化
    }
}

/**
 * @brief 构建 OpenAI 格式的消息历史数组
 * @param systemPrompt 系统提示词
 * @return 符合 OpenAI API 格式的消息数组
 *
 * @implementation
 * 1. 创建 QJsonArray
 * 2. 添加 systemPrompt 作为第一条 system 消息
 * 3. 遍历当前会话的所有消息，每条转换为 {"role": "...", "content": "..."}
 * 4. 返回数组
 *
 * @note 系统提示词总是作为第一条消息，确保AI能正确理解上下文。
 */
QJsonArray ConversationManager::buildMessageHistory(const QString& systemPrompt) const
{
    QJsonArray messages;  // 【创建】消息数组

    // 【添加】系统提示词作为第一条消息
    QJsonObject systemMsg;
    systemMsg["role"] = "system";           // 【设置】角色为system
    systemMsg["content"] = systemPrompt;    // 【设置】系统提示词内容
    messages.append(systemMsg);             // 【追加】添加到数组

    // 【保护】检查当前会话是否有效
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return messages;  // 【返回】只有系统消息的数组
    }

    // 【遍历】将当前会话的所有消息转换为JSON格式
    for (const Message& msg : m_conversations[m_currentConversationIndex].messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;          // 【设置】消息角色
        msgObj["content"] = msg.content;    // 【设置】消息内容
        messages.append(msgObj);            // 【追加】添加到数组
    }

    return messages;  // 【返回】完整的消息数组
}

/**
 * @brief 将会话数据序列化为 JSON 数组
 * @return JSON 数组
 *
 * @implementation
 * 每个会话被序列化为：
 * {
 *   "title": "会话标题",
 *   "messages": [
 *     {"role": "user", "content": "..."},
 *     ...
 *   ]
 * }
 */
QJsonArray ConversationManager::toJson() const
{
    QJsonArray result;  // 【创建】结果数组

    for (const Conversation& conv : m_conversations) {  // 【遍历】所有会话
        QJsonObject convObj;
        convObj["title"] = conv.title;  // 【设置】会话标题

        QJsonArray messagesArray;       // 【创建】消息数组
        for (const Message& msg : conv.messages) {  // 【遍历】会话中的所有消息
            QJsonObject msgObj;
            msgObj["role"] = msg.role;      // 【设置】角色
            msgObj["content"] = msg.content; // 【设置】内容
            messagesArray.append(msgObj);   // 【追加】添加消息
        }
        convObj["messages"] = messagesArray;  // 【设置】消息数组
        result.append(convObj);             // 【追加】添加会话
    }

    return result;  // 【返回】序列化后的JSON数组
}

/**
 * @brief 从 JSON 数组加载会话数据
 * @param data 序列化后的 JSON 数组
 *
 * @implementation
 * 1. 清空当前所有会话
 * 2. 遍历 JSON 数组，每个元素解析为一个 Conversation
 * 3. 恢复会话标题和消息列表
 * 4. 设置当前会话为第一个
 */
void ConversationManager::loadFromJson(const QJsonArray& data)
{
    m_conversations.clear();  // 【清空】清除所有现有会话

    for (const QJsonValue& val : data) {  // 【遍历】JSON数组中的每个会话
        if (!val.isObject()) continue;    // 【跳过】非对象元素

        QJsonObject convObj = val.toObject();  // 【获取】会话对象
        Conversation conv;
        conv.title = convObj["title"].toString();  // 【提取】会话标题

        QJsonArray messagesArray = convObj["messages"].toArray();  // 【获取】消息数组
        for (const QJsonValue& msgVal : messagesArray) {  // 【遍历】每条消息
            if (!msgVal.isObject()) continue;  // 【跳过】非对象元素

            QJsonObject msgObj = msgVal.toObject();  // 【获取】消息对象
            Message msg;
            msg.role = msgObj["role"].toString();       // 【提取】角色
            msg.content = msgObj["content"].toString(); // 【提取】内容
            conv.messages.append(msg);  // 【追加】添加消息
        }

        m_conversations.append(conv);  // 【追加】添加会话
    }

    // 【设置】当前会话为第一个，如果没有则创建默认会话
    if (!m_conversations.isEmpty()) {
        m_currentConversationIndex = 0;  // 【设置】指向第一个会话
    } else {
        createNewConversation();  // 【调用】创建默认会话
    }

    emit conversationsChanged();       // 【信号】通知UI会话列表变化
    emit currentConversationChanged(); // 【信号】通知UI当前会话变化
}
