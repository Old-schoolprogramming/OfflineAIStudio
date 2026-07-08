#include "chatwidget.h"
#include <QDateTime>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setObjectName("chatDisplay");
    m_mainLayout->addWidget(m_chatDisplay);

    QWidget* inputWidget = new QWidget(this);
    QHBoxLayout* inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(8, 8, 8, 8);
    inputLayout->setSpacing(8);

    m_messageInput = new QLineEdit(inputWidget);
    m_messageInput->setObjectName("messageInput");
    m_messageInput->setPlaceholderText("输入消息...");
    inputLayout->addWidget(m_messageInput);

    m_sendButton = new QPushButton("发送", inputWidget);
    m_sendButton->setObjectName("sendButton");
    inputLayout->addWidget(m_sendButton);

    m_mainLayout->addWidget(inputWidget);

    connect(m_sendButton, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatWidget::onReturnPressed);
}

ChatWidget::~ChatWidget()
{
}

void ChatWidget::addMessage(const QString& sender, const QString& content, bool isUser)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    
    QString style = isUser ? "color: #6366f1; font-weight: bold;" : "color: #ef4444; font-weight: bold;";
    QString bgStyle = isUser ? "background-color: #e0e7ff; border-radius: 8px; padding: 8px; margin: 4px;" 
                             : "background-color: #fef2f2; border-radius: 8px; padding: 8px; margin: 4px;";
    
    m_chatDisplay->append(QString("<div style='%1'><span style='%2'>%3</span> <span style='color: #9ca3af; font-size: 12px;'>%4</span></div>")
                          .arg(bgStyle, style, sender, time));
    m_chatDisplay->append(QString("<div style='padding-left: 8px; padding-right: 8px;'>%1</div><br>").arg(content.toHtmlEscaped()));
    
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_chatDisplay->setTextCursor(cursor);
}

void ChatWidget::clearChat()
{
    m_chatDisplay->clear();
}

void ChatWidget::onSendClicked()
{
    QString message = m_messageInput->text().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);
        m_messageInput->clear();
    }
}

void ChatWidget::onReturnPressed()
{
    onSendClicked();
}
