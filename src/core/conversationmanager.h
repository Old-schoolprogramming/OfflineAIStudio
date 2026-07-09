#ifndef CONVERSATIONMANAGER_H
#define CONVERSATIONMANAGER_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QJsonArray>

class ConversationManager : public QObject
{
    Q_OBJECT

public:
    explicit ConversationManager(QObject *parent = nullptr);

    struct Message {
        QString role;
        QString content;
        QString timestamp;
    };

    struct Conversation {
        QString id;
        QString title;
        QString createdAt;
        QList<Message> messages;
        bool isActive;
    };

    void addUserMessage(const QString& content);
    void addAssistantMessage(const QString& content);
    void clearCurrentHistory();
    int historyCount() const;
    bool isEmpty() const;

    QJsonArray buildMessageHistory(const QString& systemPrompt = "") const;

    int createNewConversation();
    void switchConversation(int index);
    int currentConversationIndex() const;
    const Conversation& currentConversation() const;
    const QList<Conversation>& conversations() const;

    void setConversationTitle(int index, const QString& title);
    QString generateTitleFromFirstMessage(const QString& message) const;

    void loadFromJson(const QJsonArray& conversations);
    QJsonArray toJson() const;

    static const int MAX_HISTORY_ROUNDS = 20;

private:
    QList<Conversation> m_conversations;
    int m_currentIndex;

    QString generateId() const;
};

#endif