#include "chatpage.h"
#include "thememanager.h"
#include "iconhelper.h"
#include "outputpanel.h"
#include "tasklistpanel.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QScrollBar>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>

static QString markdownToHtml(const QString& markdown)
{
    QString html = markdown.toHtmlEscaped();

    QRegularExpression codeBlock(R"(```(\w*)\n([\s\S]*?)```)");
    html.replace(codeBlock, "<pre><code class=\"language-\\1\">\\2</code></pre>");

    QRegularExpression inlineCode(R"(``([^`]+)``)");
    html.replace(inlineCode, "<code>\\1</code>");

    QRegularExpression bold(R"(\*\*([^*]+)\*\*)");
    html.replace(bold, "<strong>\\1</strong>");

    QRegularExpression italic(R"(\*([^*]+)\*)");
    html.replace(italic, "<em>\\1</em>");

    QRegularExpression heading(R"(^(#{1,6})\s+(.+)$)", QRegularExpression::MultilineOption);
    {
        QString result;
        int lastEnd = 0;
        QRegularExpressionMatchIterator it = heading.globalMatch(html);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            result += html.mid(lastEnd, match.capturedStart() - lastEnd);
            int level = match.captured(1).length();
            QString text = match.captured(2);
            result += QString("<h%1>%2</h%1>").arg(level).arg(text);
            lastEnd = match.capturedEnd();
        }
        result += html.mid(lastEnd);
        html = result;
    }

    QRegularExpression link(R"(\[([^\]]+)\]\(([^)]+)\))");
    html.replace(link, "<a href=\"\\2\" target=\"_blank\">\\1</a>");

    QRegularExpression list(R"(^\s*[-*+]\s+(.+)$)", QRegularExpression::MultilineOption);
    html.replace(list, "<li>\\1</li>");

    html.replace("\n", "<br>");

    html.replace("<li>", "<ul><li>");
    html.replace("</li><ul>", "</li>");
    html.replace("</li><br>", "</li></ul><br>");
    html.replace("</li></ul></ul>", "</li></ul>");

    return html;
}

ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent),
      m_currentConversationIndex(0),
      m_pendingIndex(0)
{
    m_typingTimer = new QTimer(this);
    m_typingTimer->setInterval(30);
    connect(m_typingTimer, &QTimer::timeout, this, &ChatPage::onTypingTimer);

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(3000);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &ChatPage::onAutoSaveTimer);
    m_autoSaveTimer->start();

    setupUI();
    applyTheme();

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
    setupRightPanel();

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->addWidget(m_chatArea);
    m_mainSplitter->addWidget(m_rightPanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_mainSplitter, 1);
}

void ChatPage::setupSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setFixedWidth(260);
    m_sidebar->setObjectName("chatSidebar");

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

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

    QFrame* separator = new QFrame(m_sidebar);
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    separator->setObjectName("sidebarSeparator");
    sidebarLayout->addWidget(separator);

    m_conversationList = new QListWidget(m_sidebar);
    m_conversationList->setObjectName("conversationList");
    m_conversationList->setFrameShape(QFrame::NoFrame);
    m_conversationList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_conversationList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_conversationList, &QListWidget::itemClicked, this, &ChatPage::onConversationClicked);
    sidebarLayout->addWidget(m_conversationList, 1);

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

    m_chatDisplay = new QTextBrowser(m_chatArea);
    m_chatDisplay->setObjectName("chatDisplay");
    m_chatDisplay->setFrameShape(QFrame::NoFrame);
    m_chatDisplay->setOpenExternalLinks(true);
    m_chatDisplay->setOpenLinks(true);
    m_chatDisplay->setStyleSheet(
        "QTextBrowser { border: none; background: transparent; padding: 20px; }"
    );
    chatLayout->addWidget(m_chatDisplay, 1);

    QWidget* inputWidget = new QWidget(m_chatArea);
    inputWidget->setObjectName("inputWidget");
    QVBoxLayout* inputLayout = new QVBoxLayout(inputWidget);
    inputLayout->setContentsMargins(20, 12, 20, 12);
    inputLayout->setSpacing(8);

    m_messageInput = new QTextEdit(inputWidget);
    m_messageInput->setObjectName("messageInput");
    m_messageInput->setPlaceholderText("输入消息，按 Enter 发送，Shift+Enter 换行...");
    m_messageInput->setMaximumHeight(120);
    m_messageInput->setMinimumHeight(42);
    m_messageInput->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chatLayout->addWidget(m_messageInput);

    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);

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

    m_modelSelector = new QComboBox(inputWidget);
    m_modelSelector->setObjectName("modelSelector");
    m_modelSelector->setPlaceholderText("选择模型");
    // 模型列表在测试连接后由 MainWindow 动态填充
    m_modelSelector->setStyleSheet(
        "QComboBox { border: 1px solid; border-radius: 8px; padding: 4px 12px; font-size: 12px; }"
        "QComboBox::drop-down { border-left: 1px solid; width: 20px; }"
        "QComboBox::down-arrow { image: none; }"
    );
    controlsLayout->addWidget(m_modelSelector);

    inputLayout->addLayout(controlsLayout);
    chatLayout->addWidget(inputWidget);
}

void ChatPage::setupRightPanel()
{
    m_rightPanel = new QWidget(this);
    m_rightPanel->setObjectName("rightPanel");
    m_rightPanel->setMinimumWidth(280);

    QVBoxLayout* rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_rightStack = new QStackedWidget(m_rightPanel);
    rightLayout->addWidget(m_rightStack);

    QWidget* emptyPanel = new QWidget(m_rightPanel);
    QVBoxLayout* emptyLayout = new QVBoxLayout(emptyPanel);
    emptyLayout->setContentsMargins(20, 20, 20, 20);
    QLabel* emptyLabel = new QLabel("执行日志、步骤进度和文件结果将显示在这里", emptyPanel);
    emptyLabel->setStyleSheet("color: #94a3b8; font-size: 13px; text-align: center;");
    emptyLabel->setWordWrap(true);
    emptyLayout->addWidget(emptyLabel, 0, Qt::AlignCenter);
    m_rightStack->addWidget(emptyPanel);

    m_outputPanel = new OutputPanel(m_rightPanel);
    m_rightStack->addWidget(m_outputPanel);

    m_taskListPanel = new TaskListPanel(m_rightPanel);
    m_rightStack->addWidget(m_taskListPanel);
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
        "QTextEdit#messageInput { background-color: %2; color: %6; border: 1px solid %4; border-radius: 8px; padding: 8px 14px; font-size: 14px; }"
        "QTextEdit#messageInput:focus { border-color: %5; }"
        "QPushButton#primaryButton { background-color: %5; color: white; border-radius: 8px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton#primaryButton:hover { background-color: %7; }"
        "QPushButton#stopButton { background-color: #EF4444; color: white; border-radius: 8px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton#stopButton:hover { background-color: #DC2626; }"
        "QPushButton#iconButton { background-color: transparent; color: %8; border-radius: 8px; font-size: 13px; }"
        "QPushButton#iconButton:hover { background-color: %3; }"
        "QComboBox#modelSelector { color: %7; border-color: %4; background-color: %2; }"
    ).arg(bg, bg2, bg3, bdr, pri, txtPri, pri, txtSec, txtTer));

    m_rightPanel->setStyleSheet(QString(
        "QWidget#rightPanel { background-color: %1; border-left: 1px solid %2; }"
    ).arg(bg, bdr));

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
        item->setSizeHint(QSize(m_conversationList->width(), 80));

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

        QString preview = "";
        if (!conv.messages.isEmpty()) {
            preview = conv.messages.last().left(30);
            if (conv.messages.last().length() > 30) {
                preview += "...";
            }
        }

        if (!preview.isEmpty()) {
            QLabel* previewLabel = new QLabel(preview, btn);
            previewLabel->setStyleSheet(QString("font-size: 11px; color: %1;").arg(txtTer));
            previewLabel->setAlignment(Qt::AlignLeft);
            previewLabel->setWordWrap(true);
            layout->addWidget(previewLabel);
        }

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

void ChatPage::addMessage(const QString& sender, const QString& content, bool isUser, bool saveToConversation)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    ThemeManager* tm = ThemeManager::instance();

    QString formattedContent = isUser ? content.toHtmlEscaped() : markdownToHtml(content);

    QString messageHtml;
    if (isUser) {
        QString userBg = "#4F46E5";
        QString userText = "#FFFFFF";
        messageHtml = QString(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin:8px 0;'><tr>"
            "<td align='right'>"
            "<div style='display:inline-block;background:%1;border-radius:16px 16px 2px 16px;padding:10px 14px;max-width:75%%;text-align:left;'>"
            "<div style='font-size:11px;color:rgba(255,255,255,0.7);margin-bottom:3px;'>%2 %3</div>"
            "<div style='font-size:13px;color:%4;line-height:1.5;'>%5</div>"
            "</div>"
            "</td></tr></table>"
        ).arg(userBg, sender, time, userText, formattedContent);
    } else {
        QString aiBg = tm->bgSecondary().name();
        QString aiText = tm->textPrimary().name();
        QString aiNameColor = "#6366f1";
        messageHtml = QString(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin:8px 0;'><tr>"
            "<td align='left'>"
            "<div style='display:inline-block;background:%1;border-radius:16px 16px 16px 2px;padding:10px 14px;max-width:75%%;text-align:left;'>"
            "<div style='font-size:11px;color:%2;margin-bottom:3px;'>%3 %4</div>"
            "<div style='font-size:13px;color:%5;line-height:1.5;'>%6</div>"
            "</div>"
            "</td></tr></table>"
        ).arg(aiBg, aiNameColor, sender, time, aiText, formattedContent);
    }

    m_chatDisplay->append(messageHtml);

    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());

    // 同步消息到当前对话
    if (saveToConversation && m_currentConversationIndex >= 0 && m_currentConversationIndex < m_conversations.size()) {
        if (isUser) {
            m_conversations[m_currentConversationIndex].messages.append("用户: " + content);
        } else {
            m_conversations[m_currentConversationIndex].messages.append("AI: " + content);
        }
    }
}

void ChatPage::appendMessageChunk(const QString& chunk)
{
    m_currentAiMessage += chunk;

    m_pendingText = m_currentAiMessage;
    m_pendingIndex = 0;

    if (!m_typingTimer->isActive()) {
        m_typingTimer->start();
    }
}

void ChatPage::onTypingTimer()
{
    if (m_pendingIndex >= m_pendingText.length()) {
        m_typingTimer->stop();
        return;
    }

    int chunkSize = qMin(5, m_pendingText.length() - m_pendingIndex);
    QString chunk = m_pendingText.mid(m_pendingIndex, chunkSize);
    m_pendingIndex += chunkSize;

    QString time = QDateTime::currentDateTime().toString("HH:mm");
    ThemeManager* tm = ThemeManager::instance();

    QString aiBg = tm->bgSecondary().name();
    QString aiText = tm->textPrimary().name();
    QString aiNameColor = "#6366f1";

    QString currentContent = m_chatDisplay->toHtml();
    QString searchMarker = "id='currentAiContent'";
    int idIndex = currentContent.lastIndexOf(searchMarker);

    if (idIndex == -1) {
        QString messageHtml = QString(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin:8px 0;'><tr>"
            "<td align='left'>"
            "<div style='display:inline-block;background:%1;border-radius:16px 16px 16px 2px;padding:10px 14px;max-width:75%%;text-align:left;'>"
            "<div style='font-size:11px;color:%2;margin-bottom:3px;'>AI %3</div>"
            "<div style='font-size:13px;color:%4;line-height:1.5;' id='currentAiContent'>%5</div>"
            "</div>"
            "</td></tr></table>"
        ).arg(aiBg, aiNameColor, time, aiText, markdownToHtml(m_currentAiMessage));

        m_chatDisplay->append(messageHtml);
    } else {
        int divStart = currentContent.lastIndexOf(">", idIndex);
        if (divStart != -1) {
            int divEnd = currentContent.indexOf("</div>", divStart + 1);
            if (divEnd != -1) {
                QString before = currentContent.left(divStart + 1);
                QString after = currentContent.mid(divEnd);
                QString newContent = before + markdownToHtml(m_currentAiMessage) + after;
                m_chatDisplay->setHtml(newContent);
            }
        }
    }

    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ChatPage::finishCurrentMessage()
{
    m_typingTimer->stop();

    // 同步流式AI消息到当前对话
    if (!m_currentAiMessage.isEmpty() && m_currentConversationIndex >= 0 && m_currentConversationIndex < m_conversations.size()) {
        m_conversations[m_currentConversationIndex].messages.append("AI: " + m_currentAiMessage);
    }

    m_currentAiMessage.clear();
    m_pendingText.clear();
    m_pendingIndex = 0;
}

void ChatPage::addFileCard(const QString& name, const QString& path, bool isDir, qint64 size, const QString& modified)
{
    ThemeManager* tm = ThemeManager::instance();
    QString bg = tm->bgSecondary().name();
    QString bdr = tm->border().name();
    QString pri = tm->primary().name();
    QString txtPri = tm->textPrimary().name();
    QString txtTer = tm->textTertiary().name();

    QString icon = isDir ? "📁" : "📄";
    QString sizeStr = isDir ? "文件夹" : QString("%1 KB").arg(size / 1024);

    QString html = QString(
        "<div style='display: flex; align-items: center; gap: 12px; margin: 8px 0; padding: 10px 14px; background: %1; border: 1px solid %2; border-radius: 10px;'>"
        "<span style='font-size: 18px;'>%3</span>"
        "<div style='flex: 1; min-width: 0;'>"
        "<div style='font-size: 13px; color: %4; font-weight: 500;'>%5</div>"
        "<div style='font-size: 11px; color: %6; margin-top: 2px;'>%7 · %8</div>"
        "</div>"
        "<a href='file:///%9' style='color: %10; font-size: 12px; padding: 4px 12px; border: 1px solid %10; border-radius: 6px; text-decoration: none;' onclick=\"QDesktopServices::openUrl(QUrl('file:///%9'))\">打开</a>"
        "</div>"
    ).arg(bg, bdr, icon, txtPri, name.toHtmlEscaped(), txtTer, path.toHtmlEscaped(), sizeStr, path, pri);

    m_chatDisplay->append(html);

    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ChatPage::appendOutput(const QString& text)
{
    m_chatDisplay->append(text);
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void ChatPage::appendLog(const QString& html)
{
    m_chatDisplay->append(html);
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
    QString message = m_messageInput->toPlainText().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);
        m_messageInput->clear();
    }
}

void ChatPage::onStopClicked()
{
    emit stopClicked();
}

void ChatPage::syncCurrentConversation()
{
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return;
    }

    QStringList messages;
    QString html = m_chatDisplay->toHtml();
    // 简单同步：从对话列表中提取已保存的消息
    // 消息已经在 addMessage 时同步到 m_conversations 了
    // 这里只需要更新标题和时间戳

    if (!m_conversations[m_currentConversationIndex].messages.isEmpty()) {
        QString firstMsg = m_conversations[m_currentConversationIndex].messages.first();
        if (firstMsg.startsWith("用户: ")) {
            QString title = firstMsg.mid(4).left(20);
            if (firstMsg.mid(4).length() > 20) title += "...";
            m_conversations[m_currentConversationIndex].title = title;
        }
    }
    m_conversations[m_currentConversationIndex].timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

void ChatPage::onNewChatClicked()
{
    // 先保存当前对话
    syncCurrentConversation();

    // 不要自己创建新对话，由 MainWindow/Orchestrator 统一管理
    clearChat();
    emit newChatClicked();
}

void ChatPage::onConversationClicked(QListWidgetItem* item)
{
    if (!item) return;
    int index = item->data(Qt::UserRole).toInt();

    // 保存当前对话再切换
    syncCurrentConversation();

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

void ChatPage::onAutoSaveTimer()
{
    syncCurrentConversation();
    updateConversationList();
}

void ChatPage::loadConversations(const QJsonArray& conversations)
{
    m_conversations.clear();

    for (const auto& item : conversations) {
        QJsonObject obj = item.toObject();
        Conversation conv;
        conv.title = obj["title"].toString("新对话");
        conv.timestamp = obj["createdAt"].toString();
        conv.isActive = obj["isActive"].toBool();

        QJsonArray msgs = obj["messages"].toArray();
        for (const auto& msgItem : msgs) {
            QJsonObject msgObj = msgItem.toObject();
            QString role = msgObj["role"].toString();
            QString content = msgObj["content"].toString();
            if (role == "user") {
                conv.messages.append("用户: " + content);
            } else if (role == "assistant") {
                conv.messages.append("AI: " + content);
            }
        }

        m_conversations.append(conv);
    }

    updateConversationList();
}

void ChatPage::loadConversationMessages(int index)
{
    if (index < 0 || index >= m_conversations.size()) {
        return;
    }

    clearChat();
    m_currentConversationIndex = index;

    const auto& conv = m_conversations[index];
    for (const QString& msg : conv.messages) {
        if (msg.startsWith("用户: ")) {
            addMessage("用户", msg.mid(4), true, false);
        } else if (msg.startsWith("AI: ")) {
            addMessage("AI", msg.mid(3), false, false);
        }
    }
}