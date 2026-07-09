/**
 * @file mainwindow.cpp
 * @brief 主窗口实现
 *
 * @details
 * MainWindow是应用程序的主窗口，包含左侧导航栏、顶部标题栏和
 * QStackedWidget管理的三个页面（对话、模型配置、技能导入）。
 * 负责核心组件（LLM客户端、Orchestrator、Agent）的初始化与生命周期管理，
 * 以及各页面间的导航协调和主题切换。
 */

#include "mainwindow.h"
#include "ui/thememanager.h"
#include "ui/iconhelper.h"
#include "core/environmentdetector.h"
#include <QMessageBox>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

static QString configFilePath()
{
    return QCoreApplication::applicationDirPath() + "/config.json";
}

static QString conversationsFilePath()
{
    return QCoreApplication::applicationDirPath() + "/conversations.json";
}

/**
 * @brief 构造函数
 * @param parent 父QWidget
 *
 * 初始化核心组件（LLM客户端、Orchestrator、Agent），
 * 设置UI布局和信号连接，并应用默认窗口尺寸与主题。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_currentPage(0),
      m_testManager(nullptr)
{
    setupCoreComponents();
    setupUI();
    applyStyles();

    setWindowTitle("Offline AI Studio");
    setMinimumSize(900, 600);
    resize(1400, 900);

    loadConfig();

    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
}

/**
 * @brief 析构函数
 *
 * 手动释放核心组件资源（LLM客户端、编排器、各个Agent），
 * 确保Qt对象树之外的裸指针得到正确清理。
 */
MainWindow::~MainWindow()
{
    delete m_llmClient;
    delete m_orchestrator;
    delete m_fileAgent;
    delete m_computerAgent;
    delete m_searchAgent;
}

/**
 * @brief 初始化核心逻辑组件
 *
 * 创建并配置LlmClient、Orchestrator及各类Agent实例，
 * 建立Orchestrator与主窗口之间的信号/槽连接，
 * 以便将任务计划、步骤执行状态及错误信息转发到UI层。
 */
void MainWindow::setupCoreComponents()
{
    m_llmClient = new LlmClient(this);

    m_orchestrator = new Orchestrator(this);
    m_orchestrator->setLlmClient(m_llmClient);

    m_fileAgent = new FileAgent(this);
    m_computerAgent = new ComputerAgent(this);
    m_searchAgent = new SearchAgent(this);
    m_codeAgent = new CodeAgent(this);

    m_envDetector = new EnvironmentDetector(this);
    m_codeAgent->setEnvironmentDetector(m_envDetector);
    m_envDetector->detect();

    m_orchestrator->addAgent(m_fileAgent);
    m_orchestrator->addAgent(m_computerAgent);
    m_orchestrator->addAgent(m_searchAgent);
    m_orchestrator->addAgent(m_codeAgent);

    // 连接Orchestrator各阶段信号到主窗口槽函数
    connect(m_orchestrator, &Orchestrator::planGenerated,
            this, &MainWindow::onPlanGenerated);
    connect(m_orchestrator, &Orchestrator::stepStarted,
            this, &MainWindow::onStepStarted);
    connect(m_orchestrator, &Orchestrator::stepOutput,
            this, &MainWindow::onStepOutput);
    connect(m_orchestrator, &Orchestrator::stepCompleted,
            this, &MainWindow::onStepCompleted);
    connect(m_orchestrator, &Orchestrator::stepFailed,
            this, &MainWindow::onStepFailed);
    connect(m_orchestrator, &Orchestrator::planCompleted,
            this, &MainWindow::onPlanCompleted);
    connect(m_orchestrator, &Orchestrator::messageReceived,
            this, &MainWindow::onOrchestratorMessage);
    connect(m_orchestrator, &Orchestrator::errorOccurred,
            this, &MainWindow::onOrchestratorError);
    connect(m_orchestrator, &Orchestrator::responseChunkReceived,
            this, &MainWindow::onResponseChunk);
    connect(m_orchestrator, &Orchestrator::logMessage,
            this, &MainWindow::onLogMessage);
    connect(m_llmClient, &LlmClient::responseCompleted,
            this, &MainWindow::onResponseCompleted);
}

/**
 * @brief 设置主窗口整体UI布局
 *
 * 创建中央QWidget并采用水平布局，左侧为导航边栏，右侧为主内容区。
 * 主内容区由setupMainArea()进一步划分为标题栏和页面栈。
 */
void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    setupSidebar();
    setupMainArea();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_mainArea, 1);
}

/**
 * @brief 设置左侧导航边栏
 *
 * 构建固定宽度的垂直边栏，包含Logo、三个导航按钮（对话/模型配置/技能导入）
 * 及底部的主题切换按钮。导航按钮通过onNavButtonClicked切换页面。
 */
void MainWindow::setupSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setFixedWidth(56);
    m_sidebar->setObjectName("appSidebar");

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(8, 12, 8, 12);
    sidebarLayout->setSpacing(4);
    sidebarLayout->setAlignment(Qt::AlignHCenter);

    // Logo区域
    QLabel* logoLabel = new QLabel(m_sidebar);
    logoLabel->setFixedSize(32, 32);
    logoLabel->setPixmap(IconHelper::logo(32, QColor("#FFFFFF")));
    logoLabel->setToolTip("AI Assistant");
    sidebarLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    sidebarLayout->addSpacing(8);

    // 主界面导航按钮
    m_navMain = new QPushButton(m_sidebar);
    m_navMain->setFixedSize(40, 40);
    m_navMain->setObjectName("navButton");
    m_navMain->setProperty("navKey", "main");
    m_navMain->setToolTip("主界面");
    m_navMain->setIconSize(QSize(20, 20));
    m_navMain->setCursor(Qt::PointingHandCursor);
    connect(m_navMain, &QPushButton::clicked, this, [this]() { onNavButtonClicked(0); });
    sidebarLayout->addWidget(m_navMain, 0, Qt::AlignHCenter);

    // 模型配置导航按钮
    m_navModel = new QPushButton(m_sidebar);
    m_navModel->setFixedSize(40, 40);
    m_navModel->setObjectName("navButton");
    m_navModel->setProperty("navKey", "model");
    m_navModel->setToolTip("模型配置");
    m_navModel->setIconSize(QSize(20, 20));
    m_navModel->setCursor(Qt::PointingHandCursor);
    connect(m_navModel, &QPushButton::clicked, this, [this]() { onNavButtonClicked(1); });
    sidebarLayout->addWidget(m_navModel, 0, Qt::AlignHCenter);

    // 技能导入导航按钮
    m_navSkill = new QPushButton(m_sidebar);
    m_navSkill->setFixedSize(40, 40);
    m_navSkill->setObjectName("navButton");
    m_navSkill->setProperty("navKey", "skill");
    m_navSkill->setToolTip("技能导入");
    m_navSkill->setIconSize(QSize(20, 20));
    m_navSkill->setCursor(Qt::PointingHandCursor);
    connect(m_navSkill, &QPushButton::clicked, this, [this]() { onNavButtonClicked(2); });
    sidebarLayout->addWidget(m_navSkill, 0, Qt::AlignHCenter);

    sidebarLayout->addStretch();

    // 主题切换按钮
    m_themeToggle = new QPushButton(m_sidebar);
    m_themeToggle->setFixedSize(40, 40);
    m_themeToggle->setObjectName("navButton");
    m_themeToggle->setToolTip("切换主题");
    m_themeToggle->setIconSize(QSize(20, 20));
    m_themeToggle->setCursor(Qt::PointingHandCursor);
    connect(m_themeToggle, &QPushButton::clicked, this, &MainWindow::onThemeToggleClicked);
    sidebarLayout->addWidget(m_themeToggle, 0, Qt::AlignHCenter);

    updateNavButtons();
}

/**
 * @brief 设置右侧主内容区
 *
 * 主内容区采用垂直布局，顶部为标题栏（setupHeader），
 * 下方为QStackedWidget页面栈（setupPages），用于承载对话、模型配置、技能导入三个页面。
 */
void MainWindow::setupMainArea()
{
    m_mainArea = new QWidget(this);
    m_mainArea->setObjectName("mainArea");

    QVBoxLayout* mainLayout = new QVBoxLayout(m_mainArea);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupHeader();
    setupPages();

    mainLayout->addWidget(m_header);
    mainLayout->addWidget(m_stack, 1);
}

/**
 * @brief 设置顶部标题栏
 *
 * 构建固定高度的水平标题栏，左侧显示当前页面标题，
 * 右侧包含搜索输入框和当前主题模式标签。
 */
void MainWindow::setupHeader()
{
    m_header = new QWidget(m_mainArea);
    m_header->setFixedHeight(48);
    m_header->setObjectName("appHeader");

    QHBoxLayout* headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(20, 0, 20, 0);
    headerLayout->setSpacing(16);

    m_headerTitle = new QLabel("对话", m_header);
    m_headerTitle->setObjectName("headerTitle");
    m_headerTitle->setStyleSheet("font-size: 14px; font-weight: 600;");
    headerLayout->addWidget(m_headerTitle);

    headerLayout->addStretch();

    // 搜索输入框
    m_searchInput = new QLineEdit(m_header);
    m_searchInput->setObjectName("headerSearch");
    m_searchInput->setPlaceholderText("搜索...");
    m_searchInput->setFixedSize(200, 32);
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    headerLayout->addWidget(m_searchInput);

    // 主题模式标签
    m_themeLabel = new QLabel(
        ThemeManager::instance()->currentTheme() == ThemeManager::Dark ? "深色模式" : "浅色模式",
        m_header
    );
    m_themeLabel->setObjectName("themeLabel");
    m_themeLabel->setStyleSheet("font-size: 11px;");
    headerLayout->addWidget(m_themeLabel);
}

/**
 * @brief 设置QStackedWidget页面栈
 *
 * 创建ChatPage、ModelConfigPage、SkillImportPage三个页面实例，
 * 按顺序添加到QStackedWidget中，并连接各页面发出的信号到主窗口对应槽函数。
 */
void MainWindow::setupPages()
{
    m_stack = new QStackedWidget(m_mainArea);
    m_stack->setObjectName("pageStack");

    m_chatPage = new ChatPage(m_stack);
    m_modelPage = new ModelConfigPage(m_stack);
    m_skillPage = new SkillImportPage(m_stack);

    m_stack->addWidget(m_chatPage);
    m_stack->addWidget(m_modelPage);
    m_stack->addWidget(m_skillPage);

    // 连接对话页面的用户操作信号
    connect(m_chatPage, &ChatPage::sendMessage, this, &MainWindow::onMessageSent);
    connect(m_chatPage, &ChatPage::stopClicked, this, &MainWindow::onStopClicked);
    connect(m_chatPage, &ChatPage::newChatClicked, this, &MainWindow::onNewChatClicked);
    connect(m_chatPage, &ChatPage::conversationSelected, this, &MainWindow::onConversationSelected);

    // 连接模型配置页面的设置变更信号
    connect(m_modelPage, &ModelConfigPage::settingsChanged, this, &MainWindow::onSettingsChanged);
    connect(m_modelPage, &ModelConfigPage::testConnectionClicked, this, &MainWindow::onTestConnection);
    connect(m_modelPage, &ModelConfigPage::restoreDefaultsClicked, this, &MainWindow::onRestoreDefaults);
}

/**
 * @brief 应用动态样式表
 *
 * 根据ThemeManager当前主题获取颜色令牌，
 * 为侧边栏和主内容区（含标题栏、搜索框、页面栈）构建并设置QSS样式。
 */
void MainWindow::applyStyles()
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

    m_sidebar->setStyleSheet(QString(
        "QWidget#appSidebar { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        "QPushButton#navButton { background-color: transparent; border: none; border-radius: 10px; }"
        "QPushButton#navButton:hover { background-color: %3; }"
        "QPushButton#navButton[active=true] { background-color: %5; }"
    ).arg(bg, bg2, bg3, bdr, pri, txtPri, txtSec, txtTer, txtTer));

    m_mainArea->setStyleSheet(QString(
        "QWidget#mainArea { background-color: %1; border: 1px solid %4; border-radius: 12px; }"
        "QWidget#appHeader { background-color: %2; border-bottom: 1px solid %4; border-top-left-radius: 12px; border-top-right-radius: 12px; }"
        "QLabel#headerTitle { color: %7; }"
        "QLineEdit#headerSearch { background-color: %3; color: %7; border: 1px solid transparent; border-radius: 999px; padding: 0 14px; font-size: 13px; }"
        "QLineEdit#headerSearch:focus { border-color: %6; background-color: %1; }"
        "QLabel#themeLabel { color: %9; }"
        "QStackedWidget#pageStack { background-color: %1; border-bottom-left-radius: 12px; border-bottom-right-radius: 12px; }"
    ).arg(bg, bg2, bg3, bdr, pri, pri, txtPri, txtSec, txtTer));
}

/**
 * @brief 更新导航按钮的图标与激活状态样式
 *
 * 根据m_currentPage判断当前激活的页面索引，
 * 为三个导航按钮设置对应的active属性及彩色/灰色图标，
 * 并强制刷新样式以应用QSS中的[active=true]状态。
 */
void MainWindow::updateNavButtons()
{
    bool isDark = ThemeManager::instance()->currentTheme() == ThemeManager::Dark;
    QColor activeColor = isDark ? QColor("#FFFFFF") : QColor("#FFFFFF");
    QColor inactiveColor = ThemeManager::instance()->textTertiary();

    m_navMain->setProperty("active", m_currentPage == 0);
    m_navModel->setProperty("active", m_currentPage == 1);
    m_navSkill->setProperty("active", m_currentPage == 2);

    m_navMain->setIcon(QIcon(IconHelper::message(20, m_currentPage == 0 ? activeColor : inactiveColor)));
    m_navModel->setIcon(QIcon(IconHelper::chip(20, m_currentPage == 1 ? activeColor : inactiveColor)));
    m_navSkill->setIcon(QIcon(IconHelper::layers(20, m_currentPage == 2 ? activeColor : inactiveColor)));

    // 强制重绘按钮以应用QSS属性变化
    for (auto* btn : {m_navMain, m_navModel, m_navSkill}) {
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

/**
 * @brief 导航按钮点击槽函数
 * @param pageIndex 目标页面索引（0=对话, 1=模型配置, 2=技能导入）
 *
 * 切换QStackedWidget当前页面，同步更新标题栏文本及导航按钮状态。
 */
void MainWindow::onNavButtonClicked(int pageIndex)
{
    m_currentPage = pageIndex;
    m_stack->setCurrentIndex(pageIndex);

    QStringList titles = {"对话", "模型配置", "技能导入"};
    m_headerTitle->setText(titles[pageIndex]);

    updateNavButtons();
}

/**
 * @brief 主题切换按钮点击槽函数
 *
 * 调用ThemeManager单例切换当前主题（深色/浅色）。
 */
void MainWindow::onThemeToggleClicked()
{
    ThemeManager::instance()->toggleTheme();
}

/**
 * @brief 主题变化响应槽函数
 *
 * 重新应用窗口样式、更新主题切换按钮图标、刷新主题标签文本，
 * 并同步更新导航按钮及全局QApplication样式表。
 */
void MainWindow::onThemeChanged()
{
    applyStyles();

    bool isDark = ThemeManager::instance()->currentTheme() == ThemeManager::Dark;
    QColor iconColor = ThemeManager::instance()->textSecondary();
    m_themeToggle->setIcon(QIcon(isDark ? IconHelper::sun(20, iconColor) : IconHelper::moon(20, iconColor)));
    m_themeLabel->setText(isDark ? "深色模式" : "浅色模式");

    updateNavButtons();
    qApp->setStyleSheet(ThemeManager::instance()->styleSheet());
}

/**
 * @brief 新建对话按钮点击槽函数
 *
 * 清空对话页面的聊天记录与任务计划状态。
 */
void MainWindow::onNewChatClicked()
{
    // 先保存当前对话历史
    saveConfig();

    // 创建新对话
    m_orchestrator->createNewConversation();

    // 同步对话列表到 ChatPage
    QJsonArray convHistory = m_orchestrator->conversationHistory();
    m_chatPage->loadConversations(convHistory);

    m_chatPage->clearChat();
    m_chatPage->clearPlan();
}

/**
 * @brief 搜索文本变化槽函数
 * @param text 当前搜索框文本
 *
 * 当前为占位实现，未启用实际搜索逻辑。
 */
void MainWindow::onSearchTextChanged(const QString& text)
{
    Q_UNUSED(text)
}

void MainWindow::onConversationSelected(int index)
{
    m_orchestrator->switchConversation(index);
    m_chatPage->loadConversationMessages(index);
}

/**
 * @brief 用户发送消息槽函数
 * @param message 用户输入的原始消息文本
 *
 * 若Orchestrator未处于处理状态，则在对话页面追加用户消息，
 * 显示停止按钮并将消息提交给Orchestrator进行异步处理。
 */
void MainWindow::onMessageSent(const QString& message)
{
    if (m_orchestrator->isProcessing()) {
        return;
    }

    m_chatPage->addMessage("用户", message, true);
    m_chatPage->showStopButton(true);

    m_orchestrator->processQuery(message);
}

/**
 * @brief 停止执行按钮点击槽函数
 *
 * 调用Orchestrator停止当前任务执行，并隐藏停止按钮。
 */
void MainWindow::onStopClicked()
{
    m_orchestrator->stopExecution();
    m_chatPage->showStopButton(false);
}

/**
 * @brief 任务计划生成完成槽函数
 * @param plan 生成的任务计划对象
 *
 * 将计划同步到对话页面，并在聊天区域渲染计划概览卡片（含步骤总数）。
 */
void MainWindow::onPlanGenerated(const TaskPlan& plan)
{
    m_chatPage->setPlan(plan);
    m_chatPage->appendOutput(
        QString("<div style='padding: 10px 14px; margin: 8px 0; background: rgba(59, 130, 246, 0.1); border: 1px solid #3B82F6; border-radius: 10px;'>"
                "<span style='color: #3B82F6; font-weight: 600;'>📋 计划生成</span> "
                "<span style='color: #94A3B8;'>共 %1 个步骤</span>"
                "</div>").arg(plan.totalSteps())
    );
}

/**
 * @brief 步骤开始执行槽函数
 * @param stepId 开始执行的步骤ID
 *
 * 更新对话页面中对应步骤的状态为Running。
 */
void MainWindow::onStepStarted(int stepId)
{
    m_chatPage->updateStepStatus(stepId, StepStatus::Running);
}

/**
 * @brief 步骤输出内容槽函数
 * @param stepId 产生输出的步骤ID
 * @param output 步骤输出的原始文本
 *
 * 根据当前计划查找步骤描述，若输出以"==="开头则渲染步骤标题，
 * 否则将输出内容追加到对应步骤的日志区域。
 */
void MainWindow::onStepOutput(int stepId, const QString& output)
{
    QString description;
    for (const auto& step : m_orchestrator->currentPlan().steps) {
        if (step.stepId == stepId) {
            description = step.description;
            break;
        }
    }

    if (output.startsWith("===")) {
        m_chatPage->appendStepHeader(stepId, description);
    } else {
        m_chatPage->appendStepOutput(stepId, output);
    }
}

/**
 * @brief 步骤执行成功完成槽函数
 * @param stepId 完成的步骤ID
 * @param result 步骤执行结果，包含result等字段
 *
 * 更新步骤状态为Completed，并将结果摘要（限制100字符）显示在对话页面。
 */
void MainWindow::onStepCompleted(int stepId, const QVariantMap& result)
{
    m_chatPage->updateStepStatus(stepId, StepStatus::Completed);
    QString resultMsg = result.value("result", "").toString();
    if (resultMsg.length() > 100) {
        resultMsg = resultMsg.left(100) + "...";
    }
    m_chatPage->appendStepResult(stepId, true, resultMsg);
}

/**
 * @brief 步骤执行失败槽函数
 * @param stepId 失败的步骤ID
 * @param error 错误描述文本
 *
 * 更新步骤状态为Failed，并将错误信息显示在对话页面。
 */
void MainWindow::onStepFailed(int stepId, const QString& error)
{
    m_chatPage->updateStepStatus(stepId, StepStatus::Failed);
    m_chatPage->appendStepResult(stepId, false, error);
}

/**
 * @brief 整个计划执行完成槽函数
 * @param plan 执行完毕的任务计划
 *
 * 统计各步骤的成功/失败数量，隐藏停止按钮，
 * 并在对话页面渲染任务完成的总结卡片。
 */
void MainWindow::onPlanCompleted(const TaskPlan& plan)
{
    m_chatPage->showStopButton(false);

    int success = 0, fail = 0;
    for (const auto& step : plan.steps) {
        if (step.status == StepStatus::Completed) success++;
        else if (step.status == StepStatus::Failed) fail++;
    }

    m_chatPage->appendOutput(
        QString("<div style='padding: 12px; margin: 8px 0; background: rgba(34, 197, 94, 0.1); border: 1px solid #22C55E; border-radius: 10px;'>"
                "<div style='color: #22C55E; font-weight: 700; font-size: 15px;'>🎉 任务执行完成</div>"
                "<div style='color: #94A3B8; font-size: 13px; margin-top: 4px;'>"
                "总计 %1 步 | 成功 %2 | 失败 %3"
                "</div></div>").arg(plan.totalSteps()).arg(success).arg(fail)
    );
}

/**
 * @brief Orchestrator收到AI消息槽函数
 * @param message AI回复的文本内容
 *
 * 将AI消息以非用户消息形式追加到对话页面。
 */
void MainWindow::onOrchestratorMessage(const QString& message)
{
    m_chatPage->addMessage("AI", message, false);
}

/**
 * @brief Orchestrator发生错误槽函数
 * @param error 错误描述文本
 *
 * 隐藏停止按钮，并在对话页面渲染红色错误提示卡片。
 */
void MainWindow::onOrchestratorError(const QString& error)
{
    m_chatPage->showStopButton(false);
    m_chatPage->finishCurrentMessage();
    m_chatPage->appendOutput(
        QString("<div style='padding: 10px 14px; margin: 4px 0; background: rgba(239, 68, 68, 0.1); border: 1px solid #EF4444; border-radius: 8px;'>"
                "<span style='color: #EF4444; font-weight: 600;'>❌ 错误:</span> "
                "<span style='color: #FCA5A5;'>%1</span>"
                "</div>").arg(error.toHtmlEscaped())
    );
}

void MainWindow::onResponseChunk(const QString& chunk)
{
    m_chatPage->appendMessageChunk(chunk);
}

void MainWindow::onResponseCompleted()
{
    m_chatPage->finishCurrentMessage();
}

void MainWindow::onLogMessage(const QString& logType, const QString& message)
{
    QString color;
    QString prefix;
    if (logType == "llm") {
        color = "#6366F1";
        prefix = "[LLM]";
    } else if (logType == "agent") {
        color = "#10B981";
        prefix = "[Agent]";
    } else if (logType == "system") {
        color = "#F59E0B";
        prefix = "[系统]";
    } else {
        color = "#6B7280";
        prefix = "[执行]";
    }

    QString html = QString("<div style='padding: 4px 8px; font-size: 12px;'>"
                           "<span style='color: %1; font-weight: 600;'>%2</span> "
                           "<span style='color: %3;'>%4</span>"
                           "</div>")
                          .arg(color)
                          .arg(prefix)
                          .arg(ThemeManager::instance()->textSecondary().name())
                          .arg(message.toHtmlEscaped());

    m_chatPage->appendLog(html);
}

/**
 * @brief 模型设置变更保存槽函数
 *
 * 从模型配置页面读取最新参数（API地址、模型名、最大Token数、温度值），
 * 同步到LlmClient实例，并弹出保存成功提示。
 */
void MainWindow::onSettingsChanged()
{
    m_llmClient->setApiUrl(m_modelPage->apiUrl());
    m_llmClient->setModelName(m_modelPage->modelName());
    m_llmClient->setMaxTokens(m_modelPage->maxTokens());
    m_llmClient->setTemperature(m_modelPage->temperature());

    saveConfig();

    QMessageBox::information(this, "设置已保存", "模型配置已成功保存。");
}

/**
 * @brief 测试连接按钮点击槽函数
 *
 * 向 API 端点发送 GET /v1/models 请求验证连通性。
 * 成功后获取已安装模型列表并更新连接状态显示。
 */
void MainWindow::onTestConnection()
{
    QString apiUrl = m_modelPage->apiUrl().trimmed();

    if (apiUrl.isEmpty()) {
        QMessageBox::warning(this, "测试连接", "请先输入模型端点地址。");
        return;
    }

    // 构造模型列表请求路径
    QString modelsUrl = apiUrl;
    if (modelsUrl.endsWith('/')) {
        modelsUrl.chop(1);
    }
    // 如果 URL 已包含 /v1/chat/completions，取其基础路径 + /models
    if (modelsUrl.contains("/v1/chat/completions")) {
        modelsUrl.replace("/v1/chat/completions", "/v1/models");
    } else if (modelsUrl.contains("/v1")) {
        modelsUrl += "/models";
    } else {
        modelsUrl += "/v1/models";
    }

    if (!m_testManager) {
        m_testManager = new QNetworkAccessManager(this);
    }

    QUrl url(modelsUrl);
    QNetworkRequest request(url);
    QNetworkReply* reply = m_testManager->get(request);

    m_modelPage->updateConnectionStatus(false, "正在连接...");
    m_modelPage->setTestButtonEnabled(false);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_modelPage->setTestButtonEnabled(true);

        if (reply->error() != QNetworkReply::NoError) {
            QString error = reply->errorString();
            m_modelPage->updateConnectionStatus(false, "连接失败: " + error);
            QMessageBox::warning(this, "连接失败",
                "无法连接到模型服务。\n地址: " + m_modelPage->apiUrl() + "\n错误: " + error);
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        QStringList modelNames;
        if (doc.isObject() && doc.object().contains("data")) {
            QJsonArray models = doc.object()["data"].toArray();
            for (const auto& model : models) {
                QString id = model.toObject()["id"].toString();
                if (!id.isEmpty()) {
                    modelNames.append(id);
                }
            }
        }

        if (modelNames.isEmpty()) {
            // 有些 API 可能返回不同格式，退化为直接尝试连通即可
            modelNames.append("default");
        }

        // 更新 LLM 客户端配置
        m_llmClient->setApiUrl(m_modelPage->apiUrl());

        // 更新模型详情显示
        QString detail = modelNames.first();
        m_modelPage->updateConnectionStatus(true, detail);
        m_modelPage->populateInstalledModels(modelNames);

        QMessageBox::information(this, "连接成功",
            QString("成功连接到模型服务。\n已发现 %1 个模型。").arg(modelNames.size()));

        saveConfig();
    });
}

/**
 * @brief 恢复默认设置按钮点击槽函数
 *
 * 将模型配置页面重置为初始状态。
 */
void MainWindow::onRestoreDefaults()
{
    m_modelPage->onRestoreDefaultsClicked();
    QMessageBox::information(this, "恢复默认", "已恢复默认设置。");
}

void MainWindow::saveConfig()
{
    QDir().mkpath(QCoreApplication::applicationDirPath());

    QJsonObject config;
    config["apiUrl"] = m_modelPage->apiUrl();
    config["modelName"] = m_modelPage->modelName();
    config["maxTokens"] = m_modelPage->maxTokens();
    config["temperature"] = m_modelPage->temperature();
    config["topP"] = m_modelPage->topP();
    config["contextLength"] = m_modelPage->contextLength();
    config["gpuMemoryLimit"] = m_modelPage->gpuMemoryLimit();
    config["parallelThreads"] = m_modelPage->parallelThreads();
    config["backend"] = m_modelPage->backend();

    QFile configFile(configFilePath());
    if (configFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(config);
        configFile.write(doc.toJson(QJsonDocument::Indented));
        configFile.close();
    }

    QFile convFile(conversationsFilePath());
    if (convFile.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_orchestrator->conversationHistory());
        convFile.write(doc.toJson(QJsonDocument::Indented));
        convFile.close();
    }
}

void MainWindow::loadConfig()
{
    QFile configFile(configFilePath());
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        configFile.close();

        if (doc.isObject()) {
            QJsonObject config = doc.object();
            m_modelPage->setApiUrl(config["apiUrl"].toString("http://localhost:8080/v1"));
            m_modelPage->setModelName(config["modelName"].toString("Qwen-7B-Instruct"));
            m_modelPage->setMaxTokens(config["maxTokens"].toInt(4096));
            m_modelPage->setTemperature(config["temperature"].toDouble(0.7));

            m_llmClient->setApiUrl(config["apiUrl"].toString("http://localhost:8080/v1"));
            m_llmClient->setModelName(config["modelName"].toString("Qwen-7B-Instruct"));
            m_llmClient->setMaxTokens(config["maxTokens"].toInt(4096));
            m_llmClient->setTemperature(config["temperature"].toDouble(0.7));

            QStringList modelNames;
            if (config.contains("installedModels")) {
                QJsonArray models = config["installedModels"].toArray();
                for (const auto& model : models) {
                    modelNames.append(model.toString());
                }
            }
            if (!modelNames.isEmpty()) {
                m_modelPage->populateInstalledModels(modelNames);
            }
        }
    }

    QFile convFile(conversationsFilePath());
    if (convFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(convFile.readAll());
        convFile.close();

        if (doc.isArray()) {
            m_orchestrator->loadConversationHistory(doc.array());
            m_chatPage->loadConversations(doc.array());

            int currentIndex = m_orchestrator->currentConversationIndex();
            if (currentIndex >= 0) {
                m_chatPage->loadConversationMessages(currentIndex);
            }
        }
    }
}
