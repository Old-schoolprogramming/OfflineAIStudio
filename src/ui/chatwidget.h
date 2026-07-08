#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScrollArea>

/**
 * @brief 聊天窗口组件 - 用户与AI对话的主要交互界面
 *
 * ChatWidget是一个复合组件，包含：
 * - 上方的聊天消息显示区域（只读的富文本编辑器）
 * - 下方的消息输入框和发送按钮
 *
 * 它负责展示对话历史，接收用户输入，并通过信号通知
 * 上层逻辑有新消息需要发送。
 */
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit ChatWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ChatWidget();

    /**
     * @brief 添加一条消息到聊天显示区域
     * @param sender 发送者名称（如"用户"或"AI"）
     * @param content 消息内容
     * @param isUser 是否为用户消息（决定气泡颜色）
     */
    void addMessage(const QString& sender, const QString& content, bool isUser = false);

    /**
     * @brief 清空所有聊天记录
     */
    void clearChat();

signals:
    /**
     * @brief 发送消息信号
     * @param message 消息内容
     */
    void sendMessage(const QString& message);

private slots:
    /**
     * @brief 发送按钮点击的槽函数
     */
    void onSendClicked();

    /**
     * @brief 输入框按下回车键的槽函数
     */
    void onReturnPressed();

private:
    QTextEdit* m_chatDisplay;       ///< 聊天消息显示区域
    QLineEdit* m_messageInput;      ///< 消息输入框
    QPushButton* m_sendButton;      ///< 发送按钮
    QVBoxLayout* m_mainLayout;      ///< 主布局
};

#endif
