#include "mainwindow.h"
#include "ui/thememanager.h"
#include "ui/iconhelper.h"
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_currentPage(0)
{
    setupCoreComponents();
    setupUI();
    applyStyles();

    setWindowTitle("Offline AI Studio");
    setMinimumSize(900, 600);
    resize(1400, 900);

    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
}

MainWindow::~MainWindow()
{
    delete m_llmClient;
    delete m_orchestrator;
    delete m_fileAgent;
    delete m_computerAgent;
}

void MainWindow::setupCoreComponents()
{
    m_llmClient = new LlmClient(this);
    m_llmClient->setApiUrl("http://localhost:8080/v1");
    m_llmClient->setModelName("Qwen-7B-Instruct");

    m_orchestrator = new Orchestrator(this);
    m_orchestrator->setLlmClient(m_llmClient);

    m_fileAgent = new FileAgent(this);
    m_computerAgent = new ComputerAgent(this);

    m_orchestrator->addAgent(m_fileAgent);
    m_orchestrator->addAgent(m_computerAgent);

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
}

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

void MainWindow::setupSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setFixedWidth(56);
    m_sidebar->setObjectName("appSidebar");

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(8, 12, 8, 12);
    sidebarLayout->setSpacing(4);
    sidebarLayout->setAlignment(Qt::AlignHCenter);

    QLabel* logoLabel = new QLabel(m_sidebar);
    logoLabel->setFixedSize(32, 32);
    logoLabel->setPixmap(IconHelper::logo(32, QColor("#FFFFFF")));
    logoLabel->setToolTip("AI Assistant");
    sidebarLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    sidebarLayout->addSpacing(8);

    m_navMain = new QPushButton(m_sidebar);
    m_navMain->setFixedSize(40, 40);
    m_navMain->setObjectName("navButton");
    m_navMain->setProperty("navKey", "main");
    m_navMain->setToolTip("主界面");
    m_navMain->setIconSize(QSize(20, 20));
    m_navMain->setCursor(Qt::PointingHandCursor);
    connect(m_navMain, &QPushButton::clicked, this, [this]() { onNavButtonClicked(0); });
    sidebarLayout->addWidget(m_navMain, 0, Qt::AlignHCenter);

    m_navModel = new QPushButton(m_sidebar);
    m_navModel->setFixedSize(40, 40);
    m_navModel->setObjectName("navButton");
    m_navModel->setProperty("navKey", "model");
    m_navModel->setToolTip("模型配置");
    m_navModel->setIconSize(QSize(20, 20));
    m_navModel->setCursor(Qt::PointingHandCursor);
    connect(m_navModel, &QPushButton::clicked, this, [this]() { onNavButtonClicked(1); });
    sidebarLayout->addWidget(m_navModel, 0, Qt::AlignHCenter);

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

    m_searchInput = new QLineEdit(m_header);
    m_searchInput->setObjectName("headerSearch");
    m_searchInput->setPlaceholderText("搜索...");
    m_searchInput->setFixedSize(200, 32);
    connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    headerLayout->addWidget(m_searchInput);

    m_themeLabel = new QLabel(
        ThemeManager::instance()->currentTheme() == ThemeManager::Dark ? "深色模式" : "浅色模式",
        m_header
    );
    m_themeLabel->setObjectName("themeLabel");
    m_themeLabel->setStyleSheet("font-size: 11px;");
    headerLayout->addWidget(m_themeLabel);
}

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

    connect(m_chatPage, &ChatPage::sendMessage, this, &MainWindow::onMessageSent);
    connect(m_chatPage, &ChatPage::stopClicked, this, &MainWindow::onStopClicked);
    connect(m_chatPage, &ChatPage::newChatClicked, this, &MainWindow::onNewChatClicked);

    connect(m_modelPage, &ModelConfigPage::settingsChanged, this, &MainWindow::onSettingsChanged);
    connect(m_modelPage, &ModelConfigPage::testConnectionClicked, this, &MainWindow::onTestConnection);
    connect(m_modelPage, &ModelConfigPage::restoreDefaultsClicked, this, &MainWindow::onRestoreDefaults);
}

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

    for (auto* btn : {m_navMain, m_navModel, m_navSkill}) {
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

void MainWindow::onNavButtonClicked(int pageIndex)
{
    m_currentPage = pageIndex;
    m_stack->setCurrentIndex(pageIndex);

    QStringList titles = {"对话", "模型配置", "技能导入"};
    m_headerTitle->setText(titles[pageIndex]);

    updateNavButtons();
}

void MainWindow::onThemeToggleClicked()
{
    ThemeManager::instance()->toggleTheme();
}

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

void MainWindow::onNewChatClicked()
{
    m_chatPage->clearChat();
    m_chatPage->clearPlan();
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    Q_UNUSED(text)
}

void MainWindow::onMessageSent(const QString& message)
{
    if (m_orchestrator->isProcessing()) {
        return;
    }

    m_chatPage->addMessage("用户", message, true);
    m_chatPage->showStopButton(true);

    m_orchestrator->processQuery(message);
}

void MainWindow::onStopClicked()
{
    m_orchestrator->stopExecution();
    m_chatPage->showStopButton(false);
}

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

void MainWindow::onStepStarted(int stepId)
{
    m_chatPage->updateStepStatus(stepId, StepStatus::Running);
}

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

void MainWindow::onStepCompleted(int stepId, const QVariantMap& result)
{
    m_chatPage->updateStepStatus(stepId, StepStatus::Completed);
    QString resultMsg = result.value("result", "").toString();
    if (resultMsg.length() > 100) {
        resultMsg = resultMsg.left(100) + "...";
    }
    m_chatPage->appendStepResult(stepId, true, resultMsg);
}

void MainWindow::onStepFailed(int stepId, const QString& error)
{
    m_chatPage->updateStepStatus(stepId, StepStatus::Failed);
    m_chatPage->appendStepResult(stepId, false, error);
}

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

void MainWindow::onOrchestratorMessage(const QString& message)
{
    m_chatPage->addMessage("AI", message, false);
}

void MainWindow::onOrchestratorError(const QString& error)
{
    m_chatPage->showStopButton(false);
    m_chatPage->appendOutput(
        QString("<div style='padding: 10px 14px; margin: 4px 0; background: rgba(239, 68, 68, 0.1); border: 1px solid #EF4444; border-radius: 8px;'>"
                "<span style='color: #EF4444; font-weight: 600;'>❌ 错误:</span> "
                "<span style='color: #FCA5A5;'>%1</span>"
                "</div>").arg(error.toHtmlEscaped())
    );
}

void MainWindow::onSettingsChanged()
{
    m_llmClient->setApiUrl(m_modelPage->apiUrl());
    m_llmClient->setModelName(m_modelPage->modelName());
    m_llmClient->setMaxTokens(m_modelPage->maxTokens());
    m_llmClient->setTemperature(m_modelPage->temperature());

    QMessageBox::information(this, "设置已保存", "模型配置已成功保存。");
}

void MainWindow::onTestConnection()
{
    QMessageBox::information(this, "测试连接", "正在测试与模型的连接...\n地址: " + m_modelPage->apiUrl());
}

void MainWindow::onRestoreDefaults()
{
    QMessageBox::information(this, "恢复默认", "已恢复默认设置。");
}
