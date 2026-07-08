/**
 * @file chatpage.h
 * @brief 对话页面 —— 主界面核心聊天组件
 *
 * 提供完整的对话交互界面，包括：
 * - 左侧对话历史侧边栏（新建对话、切换历史记录）
 * - 右侧聊天区域（消息气泡、任务计划展示、执行日志输出）
 * - 底部输入控制栏（消息输入、发送、停止、附件、代码模式）
 *
 * 消息布局采用左右分栏设计：用户消息居右（蓝色气泡），AI消息居左（灰色气泡）。
 * 支持多步骤任务计划的实时状态追踪与可视化展示。
 */
#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>
#include <QScrollArea>
#include <QTextBrowser>

#include "core/task.h"

class ChatPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父Widget
     *
     * 构建完整的对话页面UI，包括侧边栏、聊天区域和输入控制栏。
     * 初始化预设对话历史数据，应用当前主题样式。
     */
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();

    /**
     * @brief 添加一条聊天消息到显示区域
     * @param sender 发送者名称（"用户"或"AI"）
     * @param content 消息内容（纯文本，内部自动做HTML转义）
     * @param isUser true表示用户消息（居右蓝色气泡），false表示AI消息（居左灰色气泡）
     */
    void addMessage(const QString& sender, const QString& content, bool isUser = false);

    /** @brief 追加HTML富文本到输出面板（用于系统日志、计划摘要等） */
    void appendOutput(const QString& text);

    /** @brief 追加步骤头部信息（步骤编号+描述） */
    void appendStepHeader(int stepId, const QString& description);

    /** @brief 追加步骤执行输出日志 */
    void appendStepOutput(int stepId, const QString& output);

    /** @brief 追加步骤执行结果（成功/失败） */
    void appendStepResult(int stepId, bool success, const QString& message);

    /** @brief 清空聊天显示区域 */
    void clearChat();

    /** @brief 清空输出面板 */
    void clearOutput();

    /**
     * @brief 设置当前执行的计划
     * @param plan 从Orchestrator接收到的TaskPlan
     */
    void setPlan(const TaskPlan& plan);

    /**
     * @brief 更新指定步骤的执行状态
     * @param stepId 步骤编号
     * @param status 新状态（Pending/Running/Completed/Failed）
     */
    void updateStepStatus(int stepId, StepStatus status);

    /** @brief 清除当前计划显示 */
    void clearPlan();

    /**
     * @brief 设置输入框可用状态
     * @param enabled true表示可用，false表示禁用
     */
    void setInputEnabled(bool enabled);

    /**
     * @brief 显示/隐藏停止按钮
     * @param show true显示停止按钮并隐藏发送按钮，false反之
     */
    void showStopButton(bool show);

signals:
    /** @brief 用户点击发送按钮时发射，携带输入框文本 */
    void sendMessage(const QString& message);

    /** @brief 用户点击停止按钮时发射 */
    void stopClicked();

    /** @brief 用户点击新建对话按钮时发射 */
    void newChatClicked();

    /** @brief 用户选中某条对话历史时发射 */
    void conversationSelected(int index);

private slots:
    void onSendClicked();           ///< 发送按钮点击处理
    void onStopClicked();           ///< 停止按钮点击处理
    void onNewChatClicked();        ///< 新建对话按钮点击处理
    void onConversationClicked(QListWidgetItem* item); ///< 对话历史点击处理
    void onThemeChanged();          ///< 主题切换时重新应用样式

private:
    void setupUI();                 ///< 构建整体布局
    void setupSidebar();            ///< 构建左侧对话历史侧边栏
    void setupChatArea();           ///< 构建右侧聊天区域
    void applyTheme();              ///< 应用当前主题样式表
    void updateConversationList();  ///< 刷新对话历史列表显示

    // 侧边栏组件
    QWidget* m_sidebar;             ///< 侧边栏容器
    QListWidget* m_conversationList; ///< 对话历史列表
    QLabel* m_sidebarTitle;         ///< "对话记录"标题
    QPushButton* m_newChatButton;   ///< "+ 新建"按钮
    QLabel* m_statusLabel;          ///< 底部状态标签（离线模式）

    // 聊天区域组件
    QWidget* m_chatArea;            ///< 聊天区域容器
    QTextBrowser* m_chatDisplay;    ///< 消息显示区（支持HTML渲染）
    QLineEdit* m_messageInput;      ///< 消息输入框
    QPushButton* m_sendButton;      ///< 发送按钮（图标按钮）
    QPushButton* m_stopButton;      ///< 停止按钮（执行中显示）
    QPushButton* m_attachButton;    ///< 附件按钮
    QPushButton* m_codeButton;      ///< 代码模式按钮
    QLabel* m_modelSelector;        ///< 模型选择器标签

    TaskPlan m_currentPlan;         ///< 当前执行的计划
    int m_currentConversationIndex; ///< 当前选中的对话索引

    /** @brief 对话数据结构 */
    struct Conversation {
        QString title;              ///< 对话标题
        QString timestamp;          ///< 时间戳
        QStringList messages;       ///< 消息列表
        bool isActive;              ///< 是否为当前活跃对话
    };
    QList<Conversation> m_conversations; ///< 所有对话历史
};

#endif
