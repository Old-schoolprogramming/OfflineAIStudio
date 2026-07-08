#include "chatpage.h"
#include "thememanager.h"
#include "iconhelper.h"
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QFrame>

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent),
      m_currentConversationIndex(0)
{
    setupUI();
    applyTheme();

    // Add sample conversations
    m_conversations.append({"Python 代码优化", "今天 14:30", {}, true});
    m_conversations.append({"解释 Transformer 架构", "昨天", {}, false});
    m_conversations.append({"撰写技术文档", "昨天", {}, false});
    m_conversations.append({"数据分析脚本", "3天前", {}, false});
    m_conversations.append({"配置部署方案", "3天前", {}, false});
    updateConversationList();

    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ChatPage::onThemeChanged);
}

ChatPage::~ChatPage()
{
}

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

void ChatPage::setupSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setFixedWidth(260);
    m_sidebar->setObjectName("chatSidebar");

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    // Header
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

    // Separator
    QFrame* separator = new QFrame(m_sidebar);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setObjectName("sidebarSeparator");
    sidebarLayout->addWidget(separator);

    // Conversation list
    m_conversationList = new QListWidget(m_sidebar);
    m_conversationList->setObjectName("conversationList");
    m_conversationList->setFrameShape(QFrame::NoFrame);
    m_conversationList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_conversationList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_conversationList, &QListWidget::itemClicked, this, &ChatPage::onConversationClicked);
    sidebarLayout->addWidget(m_conversationList, 1);

    // Bottom status
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

void ChatPage::setupChatArea()
{
    m_chatArea = new QWidget(this);
    m_chatArea->setObjectName("chatArea");

    QVBoxLayout* chatLayout = new QVBoxLayout(m_chatArea);
    chatLayout->setContentsMargins(0, 0, 0, 0);
    chatLayout->setSpacing(0);

    // Chat display
    m_chatDisplay = new QTextBrowser(m_chatArea);
    m_chatDisplay->setObjectName("chatDisplay");
    m_chatDisplay->setFrameShape(QFrame::NoFrame);
    m_chatDisplay->setOpenExternalLinks(true);
    m_chatDisplay->setOpenLinks(true);
    m_chatDisplay->setStyleSheet(
        "QTextBrowser { border: none; background: transparent; padding: 20px; }"
    );
    chatLayout->addWidget(m_chatDisplay, 1);

    // Input area
    QWidget* inputWidget = new QWidget(m_chatArea);
    inputWidget->setObjectName("inputWidget");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputWidget);
    inputLayout->setContentsMargins(20, 12, 20, 12);
    inputLayout->setSpacing(8);

    m_messageInput = new QLineEdit(inputWidget);
    m_messageInput->setObjectName("messageInput");
    m_messageInput->setPlaceholderText("输入消息，按 Enter 发送...");
    m_messageInput->setMinimumHeight(42);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatPage::onSendClicked);
    inputLayout->addWidget(m_messageInput);

    // Controls row
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);

    // Left controls
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

    // Right controls
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

    QColor iconColor = ThemeManager::instance()->textSecondary();
    m_attachButton->setIcon(QIcon(IconHelper::attach(16, iconColor)));
    m_codeButton->setIcon(QIcon(IconHelper::code(16, iconColor)));
    m_sendButton->setIcon(QIcon(IconHelper::send(18, QColor("#FFFFFF"))));
}

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

        QLabel* titleLabel = new QLabel(conv.title, btn);
        titleLabel->setStyleSheet(QString("font-size: 13px; font-weight: %1; color: %2;")
            .arg(conv.isActive ? "600" : "500", txtPri));
        titleLabel->setAlignment(Qt::AlignLeft);
        layout->addWidget(titleLabel);

        QLabel* timeLabel = new QLabel(conv.timestamp, btn);
        timeLabel->setStyleSheet(QString("font-size: 11px; color: %1;").arg(txtTer));
        timeLabel->setAlignment(Qt::AlignLeft);
        layout->addWidget(timeLabel);

        QString btnStyle = conv.isActive ?
            QString("background-color: %1; border-left: 3px solid %2; border-radius: 8px;")
                .arg(priSub, pri) :
            QString("background-color: transparent; border-radius: 8px;");
        btn->setStyleSheet(btnStyle);

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

void ChatPage::addMessage(const QString& sender, const QString& content, bool isUser)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    ThemeManager* tm = ThemeManager::instance();

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

    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ChatPage::appendOutput(const QString& text)
{
    m_chatDisplay->append(text);
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

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

void ChatPage::clearChat()
{
    m_chatDisplay->clear();
}

void ChatPage::clearOutput()
{
    clearChat();
}

void ChatPage::setPlan(const TaskPlan& plan)
{
    m_currentPlan = plan;
}

void ChatPage::updateStepStatus(int stepId, StepStatus status)
{
    for (auto& step : m_currentPlan.steps) {
        if (step.stepId == stepId) {
            step.status = status;
            break;
        }
    }
}

void ChatPage::clearPlan()
{
    m_currentPlan = TaskPlan();
}

void ChatPage::setInputEnabled(bool enabled)
{
    m_messageInput->setEnabled(enabled);
    m_sendButton->setEnabled(enabled);
}

void ChatPage::showStopButton(bool show)
{
    m_stopButton->setVisible(show);
    m_sendButton->setVisible(!show);
}

void ChatPage::onSendClicked()
{
    QString message = m_messageInput->text().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);
        m_messageInput->clear();
    }
}

void ChatPage::onStopClicked()
{
    emit stopClicked();
}

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

void ChatPage::onThemeChanged()
{
    applyTheme();
    updateConversationList();
}
