/**
 * @file chatpage.cpp
 * @brief 对话页面实现
 *
 * @details
 * ChatPage提供完整的对话交互界面，包含左侧会话列表侧边栏和右侧聊天区域。
 * 支持消息渲染（用户/AI）、任务计划展示、步骤状态跟踪、主题自适应及会话管理。
 */

#include "chatpage.h"
#include "thememanager.h"
#include "iconhelper.h"
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QFrame>

/**
 * @brief 构造函数
 * @param parent 父QWidget
 *
 * 初始化UI并应用主题，预置若干示例会话数据，
 * 连接ThemeManager主题变化信号以支持动态换肤。
 */
ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent),
      m_currentConversationIndex(0)
{
    setupUI();
    applyTheme();

    // 预置示例会话数据
    m_conversations.append({"Python 代码优化", "今天 14:30", {}, true});
    m_conversations.append({"解释 Transformer 架构", "昨天", {}, false});
    m_conversations.append({"撰写技术文档", "昨天", {}, false});
    m_conversations.append({"数据分析脚本", "3天前", {}, false});
    m_conversations.append({"配置部署方案", "3天前", {}, false});
    updateConversationList();

    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ChatPage::onThemeChanged);
}

/**
 * @brief 析构函数
 */
ChatPage::~ChatPage()
{
}

/**
 * @brief 设置对话页面整体UI布局
 *
 * 采用水平布局，左侧为会话列表面板（setupSidebar），
 * 右侧为聊天与输入区域（setupChatArea），无外边距以贴合父容器。
 */
void ChatPage::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupSidebar();
    setupChatArea();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_chatArea, 1);
}

/**
 * @brief 设置左侧会话列表侧边栏
 *
 * 构建固定宽度的垂直侧边栏，包含：
 * - 顶部标题栏（标题+新建按钮）
 * - 分隔线
 * - 会话列表（QListWidget）
 * - 底部状态栏（在线状态指示）
 */
void ChatPage::setupSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setFixedWidth(260);
    m_sidebar->setObjectName("chatSidebar");

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // 顶部标题栏
    QWidget* headerWidget = new QWidget(m_sidebar);
    headerWidget->setObjectName("sidebarHeader");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 12, 12, 12);
    headerLayout->setSpacing(8);

    m_sidebarTitle = new QLabel("对话记录", headerWidget);
    m_sidebarTitle->setObjectName("sidebarTitle");
    m_sidebarTitle->setStyleSheet("font-size: 12px; font-weight: 600; letter-spacing: -0.02em;");
    headerLayout->addWidget(m_sidebarTitle);

    headerLayout->addStretch();

    m_newChatButton = new QPushButton("+ 新建", headerWidget);
    m_newChatButton->setObjectName("primaryButton");
    m_newChatButton->setFixedHeight(28);
    m_newChatButton->setCursor(Qt::PointingHandCursor);
    connect(m_newChatButton, &QPushButton::clicked, this, &ChatPage::onNewChatClicked);
    headerLayout->addWidget(m_newChatButton);

    sidebarLayout->addWidget(headerWidget);

    // 分隔线
    QFrame* separator = new QFrame(m_sidebar);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setObjectName("sidebarSeparator");
    sidebarLayout->addWidget(separator);

    // 会话列表
    m_conversationList = new QListWidget(m_sidebar);
    m_conversationList->setObjectName("conversationList");
    m_conversationList->setFrameShape(QFrame::NoFrame);
    m_conversationList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_conversationList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_conversationList, &QListWidget::itemClicked, this, &ChatPage::onConversationClicked);
    sidebarLayout->addWidget(m_conversationList, 1);

    // 底部状态栏
    QWidget* statusWidget = new QWidget(m_sidebar);
    statusWidget->setObjectName("statusWidget");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(12, 10, 12, 10);
    statusLayout->setSpacing(8);

    QLabel* statusDot = new QLabel(statusWidget);
    statusDot->setFixedSize(8, 8);
    statusDot->setObjectName("statusDot");
    statusLayout->addWidget(statusDot);

    m_statusLabel = new QLabel("离线模式 · 本地运行", statusWidget);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("font-size: 12px;");
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();

    sidebarLayout->addWidget(statusWidget);
}

/**
 * @brief 设置右侧聊天与输入区域
 *
 * 采用垂直布局，上方为只读聊天显示区（QTextBrowser），
 * 下方为输入面板，包含消息输入框及一行功能控制按钮（附件/代码/发送/停止/模型选择）。
 */
void ChatPage::setupChatArea()
{
    m_chatArea = new QWidget(this);
    m_chatArea->setObjectName("chatArea");

    QVBoxLayout* chatLayout = new QVBoxLayout(m_chatArea);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    // 聊天显示区
    m_chatDisplay = new QTextBrowser(m_chatArea);
    m_chatDisplay->setObjectName("chatDisplay");
    m_chatDisplay->setFrameShape(QFrame::NoFrame);
    m_chatDisplay->setOpenExternalLinks(true);
    m_chatDisplay->setOpenLinks(true);
    m_chatDisplay->setStyleSheet(
        "QTextBrowser { border: none; background: transparent; padding: 20px; }"
    );
    chatLayout->addWidget(m_chatDisplay, 1);

    // 输入面板
    QWidget* inputWidget = new QWidget(m_chatArea);
    inputWidget->setObjectName("inputWidget");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputWidget);
    inputLayout->setContentsMargins(20, 12, 20, 12);
    inputLayout->setSpacing(8);

    // 消息输入框
    m_messageInput = new QLineEdit(inputWidget);
    m_messageInput->setObjectName("messageInput");
    m_messageInput->setPlaceholderText("输入消息，按 Enter 发送...");
    m_messageInput->setMinimumHeight(42);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatPage::onSendClicked);
    inputLayout->addWidget(m_messageInput);

    // 功能控制按钮行
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);

    // 左侧：附件与代码片段按钮
    m_attachButton = new QPushButton(inputWidget);
    m_attachButton->setObjectName("iconButton");
    m_attachButton->setFixedSize(32, 32);
    m_attachButton->setIconSize(QSize(16, 16));
    m_attachButton->setCursor(Qt::PointingHandCursor);
    controlsLayout->addWidget(m_attachButton);

    m_codeButton = new QPushButton(inputWidget);
    m_codeButton->setObjectName("iconButton");
    m_codeButton->setFixedSize(32, 32);
    m_codeButton->setIconSize(QSize(16, 16));
    m_codeButton->setCursor(Qt::PointingHandCursor);
    controlsLayout->addWidget(m_codeButton);

    controlsLayout->addStretch();

    // 右侧：发送、停止及模型选择器
    m_sendButton = new QPushButton(inputWidget);
    m_sendButton->setObjectName("primaryButton");
    m_sendButton->setFixedSize(32, 32);
    m_sendButton->setIconSize(QSize(18, 18));
    m_sendButton->setCursor(Qt::PointingHandCursor);
    connect(m_sendButton, &QPushButton::clicked, this, &ChatPage::onSendClicked);
    controlsLayout->addWidget(m_sendButton);

    m_stopButton = new QPushButton("停止", inputWidget);
    m_stopButton->setObjectName("stopButton");
    m_stopButton->setFixedHeight(32);
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setVisible(false);
    connect(m_stopButton, &QPushButton::clicked, this, &ChatPage::onStopClicked);
    controlsLayout->addWidget(m_stopButton);

    m_modelSelector = new QLabel("Qwen-7B", inputWidget);
    m_modelSelector->setObjectName("modelSelector");
    m_modelSelector->setStyleSheet(
        "QLabel { border: 1px solid; border-radius: 8px; padding: 4px 12px; font-size: 12px; }"
    );
    m_modelSelector->setCursor(Qt::PointingHandCursor);
    controlsLayout->addWidget(m_modelSelector);

    inputLayout->addLayout(controlsLayout);
    chatLayout->addWidget(inputWidget);
}

/**
 * @brief 应用当前主题样式
 *
 * 从ThemeManager获取颜色令牌，为侧边栏、聊天区、输入控件及按钮构建QSS，
 * 同时更新图标以匹配当前主题。
 */
void ChatPage::applyTheme()
{
    ThemeManager* tm = ThemeManager::instance();
    QString bg = tm->background().name();
    QString bg2 = tm->bgSecondary().name();
    QString bg3 = tm->bgTertiary().name();
    QString bdr = tm->border().name();
    QString pri = tm->primary().name();
    QString txtPri = tm->textPrimary().name();
    QString txtSec = tm->textSecondary().name();
    QString txtTer = tm->textTertiary().name();
    QString priSub = tm->primarySubtle().name();

    m_sidebar->setStyleSheet(QString(
        "QWidget#chatSidebar { background-color: %1; border-right: 1px solid %4; }"
        "QWidget#sidebarHeader { background-color: %1; }"
        "QWidget#statusWidget { background-color: %1; border-top: 1px solid %4; }"
        "QLabel#sidebarTitle { color: %6; }"
        "QLabel#statusLabel { color: %8; }"
        "QLabel#statusDot { background-color: #22C55E; border-radius: 4px; }"
        "QListWidget#conversationList { background-color: %1; border: none; outline: none; }"
        "QListWidget#conversationList::item { border-radius: 8px; padding: 10px 12px; margin: 2px 8px; color: %6; }"
        "QListWidget#conversationList::item:hover { background-color: %3; }"
        "QListWidget#conversationList::item:selected { background-color: %9; color: %6; border-left: 2px solid %5; }"
        "QFrame { border-color: %4; }"
    ).arg(bg2, bg, bg3, bdr, pri, txtPri, txtSec, txtTer, priSub));

    m_chatArea->setStyleSheet(QString(
        "QWidget#chatArea { background-color: %1; }"
        "QWidget#inputWidget { background-color: %1; border-top: 1px solid %4; }"
        "QLineEdit#messageInput { background-color: %2; color: %6; border: 1px solid %4; border-radius: 8px; padding: 8px 14px; font-size: 14px; }"
        "QLineEdit#messageInput:focus { border-color: %5; }"
        "QPushButton#primaryButton { background-color: %5; color: white; border-radius: 8px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton#primaryButton:hover { background-color: %7; }"
        "QPushButton#stopButton { background-color: #EF4444; color: white; border-radius: 8px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton#stopButton:hover { background-color: #DC2626; }"
        "QPushButton#iconButton { background-color: transparent; color: %8; border-radius: 8px; font-size: 13px; }"
        "QPushButton#iconButton:hover { background-color: %3; }"
        "QLabel#modelSelector { color: %7; border-color: %4; background-color: %2; }"
    ).arg(bg, bg2, bg3, bdr, pri, txtPri, pri, txtSec, txtTer));

    // 更新工具栏图标
    QColor iconColor = ThemeManager::instance()->textSecondary();
    m_attachButton->setIcon(QIcon(IconHelper::attach(16, iconColor)));
    m_codeButton->setIcon(QIcon(IconHelper::code(16, iconColor)));
    m_sendButton->setIcon(QIcon(IconHelper::send(18, QColor("#FFFFFF"))));
}

/**
 * @brief 刷新会话列表UI
 *
 * 清空列表后根据m_conversations重新构建每一项。
 * 每个会话项使用自定义QPushButton作为item widget，展示标题、时间戳及激活状态高亮。
 */
void ChatPage::updateConversationList()
{
    m_conversationList->clear();
    ThemeManager* tm = ThemeManager::instance();
    QString txtPri = tm->textPrimary().name();
    QString txtTer = tm->textTertiary().name();
    QString pri = tm->primary().name();
    QString priSub = tm->primarySubtle().name();

    for (int i = 0; i < m_conversations.size(); ++i) {
        const auto& conv = m_conversations[i];
        QListWidgetItem* item = new QListWidgetItem(m_conversationList);
        item->setData(Qt::UserRole, i);
        item->setSizeHint(QSize(m_conversationList->width(), 56));

        QPushButton* btn = new QPushButton(m_conversationList);
        btn->setObjectName(conv.isActive ? "convActive" : "convItem");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(false);

        QVBoxLayout* layout = new QVBoxLayout(btn);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(2);

        // 会话标题
        QLabel* titleLabel = new QLabel(conv.title, btn);
        titleLabel->setStyleSheet(QString("font-size: 13px; font-weight: %1; color: %2;")
            .arg(conv.isActive ? "600" : "500", txtPri));
        titleLabel->setAlignment(Qt::AlignLeft);
        layout->addWidget(titleLabel);

        // 时间戳
        QLabel* timeLabel = new QLabel(conv.timestamp, btn);
        timeLabel->setStyleSheet(QString("font-size: 11px; color: %1;").arg(txtTer));
        timeLabel->setAlignment(Qt::AlignLeft);
        layout->addWidget(timeLabel);

        // 根据激活状态设置背景与左边框
        QString btnStyle = conv.isActive ?
            QString("background-color: %1; border-left: 3px solid %2; border-radius: 8px;")
                .arg(priSub, pri) :
            QString("background-color: transparent; border-radius: 8px;");
        btn->setStyleSheet(btnStyle);

        // 点击后切换当前会话并刷新列表
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            onConversationClicked(nullptr);
            m_currentConversationIndex = i;
            for (int j = 0; j < m_conversations.size(); ++j) {
                m_conversations[j].isActive = (j == i);
            }
            updateConversationList();
        });

        m_conversationList->addItem(item);
        m_conversationList->setItemWidget(item, btn);
    }
}

/**
 * @brief 向聊天区域追加一条消息
 * @param sender 发送者名称（如"用户"或"AI"）
 * @param content 消息原始文本内容
 * @param isUser true表示用户消息，false表示AI消息
 *
 * 使用HTML构建气泡式消息卡片，包含头像、发送者、时间戳及内容。
 * 用户消息右对齐（蓝色气泡），AI消息左对齐（带边框浅色气泡）。
 * 追加后自动滚动到底部。
 */
void ChatPage::addMessage(const QString& sender, const QString& content, bool isUser)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    ThemeManager* tm = ThemeManager::instance();

    // 根据消息来源选择颜色与头像
    QString avatarBg = isUser ? tm->primary().name() : "#2563EB";
    QString avatarText = isUser ? "U" : "AI";
    QString msgBg = isUser ? tm->primary().name() : tm->bgSecondary().name();
    QString msgBorder = isUser ? "transparent" : tm->border().name();
    QString msgColor = isUser ? "#FFFFFF" : tm->textPrimary().name();
    QString nameColor = isUser ? "#FFFFFF" : "#3B82F6";
    QString timeColor = isUser ? "rgba(255,255,255,0.6)" : tm->textTertiary().name();

    QString avatarHtml = QString(
        "<div style='background: %1; color: white; width: 32px; height: 32px; border-radius: 16px; display: flex; align-items: center; justify-content: center; font-size: 12px; font-weight: 700; flex-shrink: 0;'>%2</div>"
    ).arg(avatarBg, avatarText);

    QString messageHtml;
    if (isUser) {
        // 用户消息：右对齐，尖角在右下
        messageHtml = QString(
            "<div style='display: flex; flex-direction: row-reverse; align-items: flex-start; gap: 10px; margin: 12px 0;'>"
            "%1"
            "<div style='background: %2; border-radius: 16px 16px 0 16px; padding: 12px 16px; max-width: 70%%;'>"
            "<div style='font-size: 12px; font-weight: 600; color: %3; margin-bottom: 4px;'>%4 <span style='color: %5; font-weight: 400;'>%6</span></div>"
            "<div style='font-size: 13px; color: %7; line-height: 1.6;'>%8</div>"
            "</div>"
            "</div>"
        ).arg(avatarHtml, msgBg, nameColor, sender, timeColor, time, msgColor, content.toHtmlEscaped());
    } else {
        // AI消息：左对齐，尖角在左下，带边框
        messageHtml = QString(
            "<div style='display: flex; align-items: flex-start; gap: 10px; margin: 12px 0;'>"
            "%1"
            "<div style='background: %2; border: 1px solid %3; border-radius: 16px 16px 16px 0; padding: 12px 16px; max-width: 70%%;'>"
            "<div style='font-size: 12px; font-weight: 600; color: %4; margin-bottom: 4px;'>%5 <span style='color: %6; font-weight: 400;'>%7</span></div>"
            "<div style='font-size: 13px; color: %8; line-height: 1.6;'>%9</div>"
            "</div>"
            "</div>"
        ).arg(avatarHtml, msgBg, msgBorder, nameColor, sender, timeColor, time, msgColor, content.toHtmlEscaped());
    }

    m_chatDisplay->append(messageHtml);

    // 自动滚动到底部
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @brief 追加原始HTML输出到聊天显示区
 * @param text 要追加的HTML字符串
 *
 * 追加后自动滚动到底部。
 */
void ChatPage::appendOutput(const QString& text)
{
    m_chatDisplay->append(text);
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @brief 追加步骤标题卡片
 * @param stepId 步骤ID
 * @param description 步骤描述
 *
 * 以主题色边框卡片形式渲染步骤开始信息，包含步骤ID、描述及当前时间。
 */
void ChatPage::appendStepHeader(int stepId, const QString& description)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ThemeManager* tm = ThemeManager::instance();
    QString pri = tm->primary().name();
    QString priSub = tm->primarySubtle().name();
    QString txtSec = tm->textSecondary().name();

    QString html = QString(
        "<div style='margin: 12px 0; padding: 10px 14px; background: %1; border: 1px solid %2; border-radius: 10px;'>"
        "<div style='color: %2; font-weight: 600; font-size: 13px;'>▶ 步骤 %3 — %4</div>"
        "<div style='color: %5; font-size: 11px; margin-top: 4px;'>%6</div>"
        "</div>"
    ).arg(priSub, pri, QString::number(stepId), description.toHtmlEscaped(), txtSec, time);

    appendOutput(html);
}

/**
 * @brief 追加步骤日志输出
 * @param stepId 步骤ID（当前仅保留接口一致性，未在UI中单独按stepId分组）
 * @param output 步骤输出的原始文本
 *
 * 将原始文本转义为HTML并替换换行为<br>，以等宽字体样式追加到聊天区。
 */
void ChatPage::appendStepOutput(int stepId, const QString& output)
{
    Q_UNUSED(stepId)
    ThemeManager* tm = ThemeManager::instance();
    QString txtSec = tm->textSecondary().name();

    QString formatted = output.toHtmlEscaped();
    formatted.replace("\n", "<br>");

    QString html = QString(
        "<div style='padding: 4px 12px; color: %1; font-family: Consolas, monospace; font-size: 12px;'>%2</div>"
    ).arg(txtSec, formatted);

    appendOutput(html);
}

/**
 * @brief 追加步骤执行结果
 * @param stepId 步骤ID（当前仅保留接口一致性）
 * @param success true表示成功，false表示失败
 * @param message 结果摘要或错误信息
 *
 * 根据成功或失败状态渲染绿色/红色边框的结果徽章卡片。
 */
void ChatPage::appendStepResult(int stepId, bool success, const QString& message)
{
    Q_UNUSED(stepId)
    ThemeManager* tm = ThemeManager::instance();
    QString suc = success ? tm->stateSuccess().name() : tm->stateError().name();
    QString bg = success ? "rgba(34, 197, 94, 0.1)" : "rgba(239, 68, 68, 0.1)";
    QString bdr = success ? "#22C55E" : "#EF4444";
    QString statusText = success ? "成功" : "失败";
    QString icon = success ? "✓" : "✗";

    QString html = QString(
        "<div style='margin: 8px 0; padding: 8px 12px; background: %1; border: 1px solid %2; border-radius: 8px;'>"
        "<span style='color: %3; font-weight: 600;'>%4 %5</span>"
        "<span style='color: %6; margin-left: 8px; font-size: 12px;'>%7</span>"
        "</div>"
    ).arg(bg, bdr, suc, icon, statusText, tm->textSecondary().name(), message.toHtmlEscaped());

    appendOutput(html);
}

/**
 * @brief 清空聊天显示区
 */
void ChatPage::clearChat()
{
    m_chatDisplay->clear();
}

/**
 * @brief 清空输出（与clearChat同义）
 */
void ChatPage::clearOutput()
{
    clearChat();
}

/**
 * @brief 设置当前任务计划
 * @param plan 任务计划对象
 */
void ChatPage::setPlan(const TaskPlan& plan)
{
    m_currentPlan = plan;
}

/**
 * @brief 更新指定步骤的状态
 * @param stepId 目标步骤ID
 * @param status 新的步骤状态
 *
 * 在m_currentPlan.steps中查找匹配stepId的条目并更新其状态。
 */
void ChatPage::updateStepStatus(int stepId, StepStatus status)
{
    for (auto& step : m_currentPlan.steps) {
        if (step.stepId == stepId) {
            step.status = status;
            break;
        }
    }
}

/**
 * @brief 清空当前任务计划
 */
void ChatPage::clearPlan()
{
    m_currentPlan = TaskPlan();
}

/**
 * @brief 设置输入区域启用状态
 * @param enabled true为启用，false为禁用
 *
 * 同步控制消息输入框和发送按钮的可用性。
 */
void ChatPage::setInputEnabled(bool enabled)
{
    m_messageInput->setEnabled(enabled);
    m_sendButton->setEnabled(enabled);
}

/**
 * @brief 控制停止按钮的显隐
 * @param show true显示停止按钮并隐藏发送按钮，false则相反
 */
void ChatPage::showStopButton(bool show)
{
    m_stopButton->setVisible(show);
    m_sendButton->setVisible(!show);
}

/**
 * @brief 发送按钮点击槽函数
 *
 * 获取输入框文本并去除首尾空白，若非空则发射sendMessage信号并清空输入框。
 */
void ChatPage::onSendClicked()
{
    QString message = m_messageInput->text().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);
        m_messageInput->clear();
    }
}

/**
 * @brief 停止按钮点击槽函数
 *
 * 发射stopClicked信号，由上层主窗口接收并中断Orchestrator执行。
 */
void ChatPage::onStopClicked()
{
    emit stopClicked();
}

/**
 * @brief 新建会话按钮点击槽函数
 *
 * 将所有现有会话设为非激活，在列表头部插入"新对话"并设为激活，
 * 限制最大会话数为20，随后刷新列表、清空聊天区并发射newChatClicked信号。
 */
void ChatPage::onNewChatClicked()
{
    for (auto& conv : m_conversations) {
        conv.isActive = false;
    }
    m_conversations.prepend({"新对话", "刚刚", {}, true});
    if (m_conversations.size() > 20) {
        m_conversations.removeLast();
    }
    updateConversationList();
    clearChat();
    emit newChatClicked();
}

/**
 * @brief 会话列表项点击槽函数
 * @param item 被点击的QListWidgetItem（可能为nullptr）
 *
 * 根据item携带的UserRole索引激活对应会话，刷新列表并发射conversationSelected信号。
 */
void ChatPage::onConversationClicked(QListWidgetItem* item)
{
    if (!item) return;
    int index = item->data(Qt::UserRole).toInt();
    for (int i = 0; i < m_conversations.size(); ++i) {
        m_conversations[i].isActive = (i == index);
    }
    m_currentConversationIndex = index;
    updateConversationList();
    emit conversationSelected(index);
}

/**
 * @brief 主题变化响应槽函数
 *
 * 重新应用主题样式并刷新会话列表以更新各会话项的颜色。
 */
void ChatPage::onThemeChanged()
{
    applyTheme();
    updateConversationList();
}
