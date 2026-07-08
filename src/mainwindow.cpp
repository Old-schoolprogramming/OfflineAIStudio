#include "mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_sidebarVisible(true)
{
    setupCoreComponents();
    setupUI();
    setupMenuBar();
    applyStyles();
    
    setWindowTitle("Offline AI Studio");
    setMinimumSize(1024, 768);
    resize(1280, 800);
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
    m_llmClient->setApiUrl("http://localhost:8080/v1/chat/completions");
    m_llmClient->setModelName("model");

    m_orchestrator = new Orchestrator(this);
    m_orchestrator->setLlmClient(m_llmClient);

    m_fileAgent = new FileAgent(this);
    m_computerAgent = new ComputerAgent(this);

    m_orchestrator->addAgent(m_fileAgent);
    m_orchestrator->addAgent(m_computerAgent);

    connect(m_orchestrator, &Orchestrator::messageReceived, this, &MainWindow::onLlmMessageReceived);
    connect(m_orchestrator, &Orchestrator::agentSelected, this, &MainWindow::onAgentSelected);
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    m_mainSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(m_mainSplitter);

    m_sidebarTabs = new QTabWidget(this);
    m_sidebarTabs->setMaximumWidth(280);

    m_agentSelector = new AgentSelector(this);
    m_agentSelector->addAgent(m_fileAgent);
    m_agentSelector->addAgent(m_computerAgent);
    m_sidebarTabs->addTab(m_agentSelector, "Agent");

    m_settingsPanel = new SettingsPanel(this);
    m_sidebarTabs->addTab(m_settingsPanel, "设置");

    m_mainSplitter->addWidget(m_sidebarTabs);

    m_chatWidget = new ChatWidget(this);
    m_mainSplitter->addWidget(m_chatWidget);

    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 4);

    connect(m_chatWidget, &ChatWidget::sendMessage, this, &MainWindow::onMessageSent);
    connect(m_settingsPanel, &SettingsPanel::settingsChanged, this, &MainWindow::onSettingsChanged);
}

void MainWindow::setupMenuBar()
{
    QMenuBar* menuBar = this->menuBar();

    QMenu* fileMenu = menuBar->addMenu("文件");
    m_toggleSidebarAction = fileMenu->addAction("显示/隐藏侧边栏");
    connect(m_toggleSidebarAction, &QAction::triggered, this, &MainWindow::onToggleSidebarAction);

    QAction* clearChatAction = fileMenu->addAction("清空聊天");
    connect(clearChatAction, &QAction::triggered, this, &MainWindow::onClearChatAction);

    QMenu* helpMenu = menuBar->addMenu("帮助");
    QAction* aboutAction = helpMenu->addAction("关于");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutAction);
}

void MainWindow::applyStyles()
{
    QString style = R"(
        QMainWindow {
            background-color: #0f172a;
        }
        
        QMenuBar {
            background-color: #1e293b;
            color: #f1f5f9;
            padding: 4px;
        }
        
        QMenuBar::item {
            padding: 6px 16px;
            margin: 2px;
            border-radius: 4px;
        }
        
        QMenuBar::item:selected {
            background-color: #334155;
        }
        
        QMenu {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 4px;
        }
        
        QMenu::item {
            padding: 8px 24px;
            border-radius: 4px;
        }
        
        QMenu::item:selected {
            background-color: #334155;
        }
        
        QTabWidget {
            background-color: #1e293b;
            border: none;
        }
        
        QTabBar {
            background-color: #1e293b;
        }
        
        QTabBar::tab {
            background-color: #334155;
            color: #94a3b8;
            padding: 8px 16px;
            margin-right: 4px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            border: none;
        }
        
        QTabBar::tab:selected {
            background-color: #0f172a;
            color: #f1f5f9;
        }
        
        QListWidget {
            background-color: #1e293b;
            color: #f1f5f9;
            border: none;
            border-radius: 8px;
        }
        
        QListWidget::item {
            padding: 12px;
            border-bottom: 1px solid #334155;
        }
        
        QListWidget::item:hover {
            background-color: #334155;
        }
        
        QListWidget::item:selected {
            background-color: #475569;
            color: #f1f5f9;
        }
        
        QLineEdit {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 8px 12px;
        }
        
        QLineEdit:focus {
            border-color: #6366f1;
            outline: none;
        }
        
        QSpinBox, QDoubleSpinBox {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 8px 12px;
        }
        
        QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: #6366f1;
            outline: none;
        }
        
        QPushButton {
            background-color: #6366f1;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 16px;
            font-weight: 500;
        }
        
        QPushButton:hover {
            background-color: #4f46e5;
        }
        
        QPushButton:pressed {
            background-color: #4338ca;
        }
        
        #sendButton {
            background-color: #6366f1;
            min-width: 80px;
        }
        
        #saveButton {
            background-color: #22c55e;
        }
        
        #saveButton:hover {
            background-color: #16a34a;
        }
        
        QTextEdit#chatDisplay {
            background-color: #0f172a;
            color: #f1f5f9;
            border: none;
            padding: 12px;
            font-family: 'Consolas', 'Monaco', monospace;
            font-size: 14px;
        }
        
        QLabel#agentTitle, QLabel#settingsTitle {
            color: #f1f5f9;
            font-weight: 600;
            font-size: 14px;
            padding-bottom: 8px;
            border-bottom: 1px solid #334155;
        }
        
        QSplitter::handle {
            background-color: #334155;
        }
        
        QSplitter::handle:hover {
            background-color: #475569;
        }
    )";

    this->setStyleSheet(style);
}

void MainWindow::onMessageSent(const QString& message)
{
    m_chatWidget->addMessage("用户", message, true);
    m_orchestrator->processQuery(message);
}

void MainWindow::onLlmMessageReceived(const QString& message)
{
    m_chatWidget->addMessage("AI", message, false);
}

void MainWindow::onAgentSelected(const QString& agentName)
{
    statusBar()->showMessage("正在使用: " + agentName, 3000);
}

void MainWindow::onSettingsChanged()
{
    m_llmClient->setApiUrl(m_settingsPanel->apiUrl());
    m_llmClient->setModelName(m_settingsPanel->modelName());
    m_llmClient->setMaxTokens(m_settingsPanel->maxTokens());
    m_llmClient->setTemperature(m_settingsPanel->temperature());
    
    statusBar()->showMessage("设置已保存", 2000);
}

void MainWindow::onAboutAction()
{
    QMessageBox::about(this, "关于 Offline AI Studio", 
        "Offline AI Studio v1.0.0\n\n"
        "一个可以完全离线运行的AI助手应用程序，支持调用本地大模型和多种Agent。\n\n"
        "功能特性:\n"
        "- FileAgent: 文件读写和目录管理\n"
        "- ComputerAgent: 系统命令执行\n"
        "- Orchestrator: 智能协调调度\n\n"
        "技术栈: C++ / Qt6");
}

void MainWindow::onClearChatAction()
{
    m_chatWidget->clearChat();
}

void MainWindow::onToggleSidebarAction()
{
    m_sidebarVisible = !m_sidebarVisible;
    m_sidebarTabs->setVisible(m_sidebarVisible);
}
