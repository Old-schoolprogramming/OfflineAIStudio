/**
 * @file chatwidget.cpp
 * @brief 聊天界面控件实现
 *
 * @details
 * ChatWidget 提供基础的聊天 UI，包含：
 * - 只读的聊天消息显示区（QTextEdit，HTML 富文本渲染）
 * - 消息输入框（QLineEdit）和发送按钮（QPushButton）
 *
 * 核心设计要点：
 * 1. 用户消息和 AI 消息使用不同颜色区分（用户：靛蓝 #6366f1，AI：红 #ef4444）
 * 2. 消息背景使用浅色圆角卡片，提升可读性
 * 3. 支持点击按钮和按回车键两种发送方式
 * 4. 每次追加消息后自动滚动到底部
 * 5. 所有消息内容经过 toHtmlEscaped() 转义，防止 HTML 注入
 */

#include "chatwidget.h"
#include <QDateTime>

/**
 * @brief 构造函数
 * @param parent 父 QWidget
 *
 * @implementation
 * 布局结构（垂直方向）：
 * m_mainLayout (QVBoxLayout)
 *   ├── m_chatDisplay  — 只读 QTextEdit（占据主要空间）
 *   └── inputWidget    — 输入区域（QHBoxLayout）
 *         ├── m_messageInput — QLineEdit（输入框，占位文本"输入消息..."）
 *         └── m_sendButton   — QPushButton（"发送"按钮）
 *
 * 信号连接：
 * - 发送按钮点击 → onSendClicked()
 * - 输入框回车键 → onReturnPressed()
 */
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

/**
 * @brief 析构函数
 */
ChatWidget::~ChatWidget()
{
}

/**
 * @brief 向聊天窗口追加一条消息
 * @param sender 发送者名称（如 "用户" 或 "AI"）
 * @param content 消息正文（纯文本，会自动进行 HTML 转义）
 * @param isUser true=用户消息, false=AI 消息
 *
 * @implementation
 * 1. 获取当前时间戳（HH:mm:ss 格式）
 * 2. 根据 isUser 选择不同样式：
 *    - 用户：靛蓝色 (#6366f1) 加粗标题 + 浅蓝背景 (#e0e7ff)
 *    - AI：红色 (#ef4444) 加粗标题 + 浅红背景 (#fef2f2)
 * 3. 生成 HTML：包含发送者、时间戳和消息内容（经过 toHtmlEscaped 转义）
 * 4. 使用 QTextEdit::append() 追加到显示区
 * 5. 移动 QTextCursor 到文档末尾，实现自动滚动
 */
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

/**
 * @brief 清空所有聊天内容
 */
void ChatWidget::clearChat()
{
    m_chatDisplay->clear();
}

/**
 * @brief 发送按钮点击事件处理
 *
 * @implementation
 * 1. 获取输入框文本并去除首尾空白（trimmed）
 * 2. 若文本非空：
 *    - 发射 sendMessage(message) 信号，通知外部逻辑层发送消息
 *    - 清空输入框，准备下一次输入
 */
void ChatWidget::onSendClicked()
{
    QString message = m_messageInput->text().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);
        m_messageInput->clear();
    }
}

/**
 * @brief 输入框回车键按下事件处理
 *
 * @implementation
 * 直接委托给 onSendClicked() 处理，统一发送逻辑
 */
void ChatWidget::onReturnPressed()
{
    onSendClicked();
}
