#include "conversationmanager.h"
#include <QJsonObject>
#include <QDateTime>
#include <QUuid>

ConversationManager::ConversationManager(QObject *parent)
    : QObject(parent),
      m_currentIndex(-1)
{
    createNewConversation();
}

void ConversationManager::addUserMessage(const QString& content)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_conversations.size()) {
        createNewConversation();
    }

    Message msg;
    msg.role = "user";
    msg.content = content;
    msg.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    m_conversations[m_currentIndex].messages.append(msg);

    while (m_conversations[m_currentIndex].messages.size() > MAX_HISTORY_ROUNDS * 2) {
        m_conversations[m_currentIndex].messages.removeFirst();
    }

    if (m_conversations[m_currentIndex].messages.size() == 1) {
        QString title = generateTitleFromFirstMessage(content);
        m_conversations[m_currentIndex].title = title;
    }
}

void ConversationManager::addAssistantMessage(const QString& content)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_conversations.size()) {
        return;
    }

    Message msg;
    msg.role = "assistant";
    msg.content = content;
    msg.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    m_conversations[m_currentIndex].messages.append(msg);

    while (m_conversations[m_currentIndex].messages.size() > MAX_HISTORY_ROUNDS * 2) {
        m_conversations[m_currentIndex].messages.removeFirst();
    }
}

void ConversationManager::clearCurrentHistory()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_conversations.size()) {
        m_conversations[m_currentIndex].messages.clear();
    }
}

int ConversationManager::historyCount() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_conversations.size()) {
        return 0;
    }
    return m_conversations[m_currentIndex].messages.size();
}

bool ConversationManager::isEmpty() const
{
    return historyCount() == 0;
}

QJsonArray ConversationManager::buildMessageHistory(const QString& systemPrompt) const
{
    QJsonArray messages;

    if (!systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = systemPrompt;
        messages.append(systemMsg);
    }

    if (m_currentIndex >= 0 && m_currentIndex < m_conversations.size()) {
        for (const auto& msg : m_conversations[m_currentIndex].messages) {
            QJsonObject obj;
            obj["role"] = msg.role;
            obj["content"] = msg.content;
            messages.append(obj);
        }
    }

    return messages;
}

int ConversationManager::createNewConversation()
{
    Conversation conv;
    conv.id = generateId();
    conv.title = "新对话";
    conv.createdAt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    conv.isActive = false;

    for (auto& c : m_conversations) {
        c.isActive = false;
    }

    m_conversations.prepend(conv);
    m_conversations[0].isActive = true;
    m_currentIndex = 0;

    return 0;
}

void ConversationManager::switchConversation(int index)
{
    if (index < 0 || index >= m_conversations.size()) {
        return;
    }

    for (auto& c : m_conversations) {
        c.isActive = false;
    }

    m_conversations[index].isActive = true;
    m_currentIndex = index;
}

int ConversationManager::currentConversationIndex() const
{
    return m_currentIndex;
}

const ConversationManager::Conversation& ConversationManager::currentConversation() const
{
    static Conversation empty;
    if (m_currentIndex < 0 || m_currentIndex >= m_conversations.size()) {
        return empty;
    }
    return m_conversations[m_currentIndex];
}

const QList<ConversationManager::Conversation>& ConversationManager::conversations() const
{
    return m_conversations;
}

void ConversationManager::setConversationTitle(int index, const QString& title)
{
    if (index >= 0 && index < m_conversations.size()) {
        m_conversations[index].title = title;
    }
}

QString ConversationManager::generateTitleFromFirstMessage(const QString& message) const
{
    QString trimmed = message.trimmed();
    if (trimmed.length() <= 20) {
        return trimmed;
    }
    return trimmed.left(20) + "...";
}

void ConversationManager::loadFromJson(const QJsonArray& conversations)
{
    m_conversations.clear();

    for (const auto& item : conversations) {
        QJsonObject obj = item.toObject();
        Conversation conv;
        conv.id = obj["id"].toString();
        conv.title = obj["title"].toString();
        conv.createdAt = obj["createdAt"].toString();
        conv.isActive = obj["isActive"].toBool();

        QJsonArray msgs = obj["messages"].toArray();
        for (const auto& msgItem : msgs) {
            QJsonObject msgObj = msgItem.toObject();
            Message msg;
            msg.role = msgObj["role"].toString();
            msg.content = msgObj["content"].toString();
            msg.timestamp = msgObj["timestamp"].toString();
            conv.messages.append(msg);
        }

        m_conversations.append(conv);
    }

    if (m_conversations.isEmpty()) {
        createNewConversation();
    } else {
        bool foundActive = false;
        for (int i = 0; i < m_conversations.size(); ++i) {
            if (m_conversations[i].isActive) {
                m_currentIndex = i;
                foundActive = true;
                break;
            }
        }
        if (!foundActive) {
            m_currentIndex = 0;
            m_conversations[0].isActive = true;
        }
    }
}

QJsonArray ConversationManager::toJson() const
{
    QJsonArray result;

    for (const auto& conv : m_conversations) {
        QJsonObject obj;
        obj["id"] = conv.id;
        obj["title"] = conv.title;
        obj["createdAt"] = conv.createdAt;
        obj["isActive"] = conv.isActive;

        QJsonArray msgs;
        for (const auto& msg : conv.messages) {
            QJsonObject msgObj;
            msgObj["role"] = msg.role;
            msgObj["content"] = msg.content;
            msgObj["timestamp"] = msg.timestamp;
            msgs.append(msgObj);
        }
        obj["messages"] = msgs;

        result.append(obj);
    }

    return result;
}

QString ConversationManager::generateId() const
{
    return QUuid::createUuid().toString().mid(1, 36);
}