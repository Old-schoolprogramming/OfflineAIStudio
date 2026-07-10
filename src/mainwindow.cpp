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

#include "mainwindow.h"          // 【引入自身头文件】包含MainWindow类声明
#include "ui/thememanager.h"     // 【引入主题管理器】提供深色/浅色主题切换
#include "ui/iconhelper.h"       // 【引入图标辅助类】生成各种SVG图标
#include "core/environmentdetector.h" // 【引入环境检测器】检测系统可用工具
#include <QMessageBox>           // 【引入消息框类】弹出提示/警告/错误对话框
#include <QApplication>          // 【引入应用类】访问全局应用实例
#include <QJsonDocument>         // 【引入JSON文档类】读写JSON格式数据
#include <QJsonObject>           // 【引入JSON对象类】存储键值对形式的JSON数据
#include <QJsonArray>            // 【引入JSON数组类】存储数组形式的JSON数据
#include <QDir>                  // 【引入目录类】创建目录、操作路径

// 【静态函数】返回配置文件config.json的完整路径（位于程序所在目录）
static QString configFilePath()
{
    return QCoreApplication::applicationDirPath() + "/config.json";  // applicationDirPath()获取.exe所在文件夹路径
}

// 【静态函数】返回对话历史文件conversations.json的完整路径
static QString conversationsFilePath()
{
    QDir().mkpath(QCoreApplication::applicationDirPath() + "/conversations");  // 【创建目录】如果不存在conversations文件夹则自动创建
    return QCoreApplication::applicationDirPath() + "/conversations/conversations.json";  // 【返回路径】拼接完整文件路径
}

/**
 * @brief 构造函数
 * @param parent 父QWidget（传nullptr表示这是顶级窗口）
 *
 * 初始化核心组件（LLM客户端、Orchestrator、Agent），
 * 设置UI布局和信号连接，并应用默认窗口尺寸与主题。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),      // 【调用基类构造函数】将parent传给QMainWindow
      m_currentPage(0),         // 【初始化成员】当前页面索引默认为0（对话页面）
      m_testManager(nullptr)    // 【初始化成员】测试连接管理器初始为空指针
{
    setupCoreComponents();  // 【步骤1】初始化LLM、Orchestrator、Agent等核心逻辑组件
    setupUI();              // 【步骤2】构建整体UI布局（侧边栏+主区域）
    applyStyles();          // 【步骤3】应用当前主题的CSS样式表

    setWindowTitle("Offline AI Studio");  // 【设置窗口标题】显示在标题栏和任务栏的文字
    setMinimumSize(900, 600);             // 【设置最小尺寸】窗口不允许缩小到900x600以下
    resize(1400, 900);                    // 【设置初始尺寸】窗口打开时默认大小为1400x900

    loadConfig();  // 【步骤4】从本地文件加载用户保存的模型配置和对话历史

    // 【信号槽连接】监听主题变化信号，当用户切换主题时调用onThemeChanged槽函数
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
    delete m_llmClient;      // 【释放内存】删除LLM客户端实例
    delete m_orchestrator;   // 【释放内存】删除总控协调器实例
    delete m_fileAgent;      // 【释放内存】删除文件Agent实例
    delete m_computerAgent;  // 【释放内存】删除系统命令Agent实例
    delete m_searchAgent;    // 【释放内存】删除搜索Agent实例
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
    m_llmClient = new LlmClient(this);  // 【创建LLM客户端】负责与AI模型API进行网络通信

    m_orchestrator = new Orchestrator(this);  // 【创建总控协调器】负责任务规划、Agent调度
    m_orchestrator->setLlmClient(m_llmClient);  // 【注入依赖】将LLM客户端交给协调器使用

    // 【创建各类Agent】每个Agent负责一种类型的任务
    m_fileAgent = new FileAgent(this);          // 文件操作：读写文件、目录遍历
    m_computerAgent = new ComputerAgent(this);  // 系统命令：执行shell/cmd命令
    m_searchAgent = new SearchAgent(this);      // 网络搜索：联网查询信息
    m_codeAgent = new CodeAgent(this);          // 代码开发：代码生成、编译运行

    m_envDetector = new EnvironmentDetector(this);  // 【创建环境检测器】检测系统可用工具
    m_codeAgent->setEnvironmentDetector(m_envDetector);  // 【注入环境检测器】让代码Agent知道系统有什么工具
    m_envDetector->detect();  // 【立即执行检测】在启动时扫描系统环境

    // 【注册Agent】将所有Agent添加到协调器中，协调器会根据任务类型分发给对应Agent
    m_orchestrator->addAgent(m_fileAgent);
    m_orchestrator->addAgent(m_computerAgent);
    m_orchestrator->addAgent(m_searchAgent);
    m_orchestrator->addAgent(m_codeAgent);

    // ========== 连接Orchestrator各阶段信号到主窗口槽函数 ==========
    // 【信号槽】计划生成完毕 -> 更新UI显示计划概览
    connect(m_orchestrator, &Orchestrator::planGenerated,
            this, &MainWindow::onPlanGenerated);
    // 【信号槽】步骤开始执行 -> 更新步骤状态为Running
    connect(m_orchestrator, &Orchestrator::stepStarted,
            this, &MainWindow::onStepStarted);
    // 【信号槽】步骤产生输出 -> 追加输出到对应步骤的日志区域
    connect(m_orchestrator, &Orchestrator::stepOutput,
            this, &MainWindow::onStepOutput);
    // 【信号槽】步骤成功完成 -> 更新状态并显示结果
    connect(m_orchestrator, &Orchestrator::stepCompleted,
            this, &MainWindow::onStepCompleted);
    // 【信号槽】步骤执行失败 -> 更新状态并显示错误
    connect(m_orchestrator, &Orchestrator::stepFailed,
            this, &MainWindow::onStepFailed);
    // 【信号槽】整个计划完成 -> 显示任务完成总结
    connect(m_orchestrator, &Orchestrator::planCompleted,
            this, &MainWindow::onPlanCompleted);
    // 【信号槽】收到AI消息 -> 显示在对话页面
    connect(m_orchestrator, &Orchestrator::messageReceived,
            this, &MainWindow::onOrchestratorMessage);
    // 【信号槽】发生错误 -> 显示红色错误提示
    connect(m_orchestrator, &Orchestrator::errorOccurred,
            this, &MainWindow::onOrchestratorError);
    // 【信号槽】收到流式响应片段 -> 实时追加到当前消息
    connect(m_orchestrator, &Orchestrator::responseChunkReceived,
            this, &MainWindow::onResponseChunk);
    // 【信号槽】收到日志消息 -> 显示在日志区域
    connect(m_orchestrator, &Orchestrator::logMessage,
            this, &MainWindow::onLogMessage);
    // 【信号槽】流式响应完成 -> 结束当前消息显示
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
    QWidget* centralWidget = new QWidget(this);  // 【创建中央容器】QMainWindow必须有中央部件才能放布局
    setCentralWidget(centralWidget);             // 【设置中央部件】将centralWidget设为主窗口的中央区域

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);  // 【创建水平布局】从左到右排列子控件
    mainLayout->setContentsMargins(8, 8, 8, 8);  // 【设置边距】左/上/右/下各留8像素空白
    mainLayout->setSpacing(8);                   // 【设置间距】子控件之间间隔8像素

    setupSidebar();   // 【构建侧边栏】调用setupSidebar创建左侧导航栏
    setupMainArea();  // 【构建主区域】调用setupMainArea创建右侧内容区

    mainLayout->addWidget(m_sidebar);       // 【添加侧边栏】将导航栏加入水平布局
    mainLayout->addWidget(m_mainArea, 1);   // 【添加主区域】将内容区加入水平布局，stretch=1表示占据所有剩余空间
}

/**
 * @brief 设置左侧导航边栏
 *
 * 构建固定宽度的垂直边栏，包含Logo、三个导航按钮（对话/模型配置/技能导入）
 * 及底部的主题切换按钮。导航按钮通过onNavButtonClicked切换页面。
 */
void MainWindow::setupSidebar()
{
    m_sidebar = new QWidget(this);      // 【创建侧边栏容器】作为所有导航控件的父级
    m_sidebar->setFixedWidth(56);       // 【固定宽度】导航栏宽度56像素，不可拉伸
    m_sidebar->setObjectName("appSidebar");  // 【设置对象名】用于QSS样式选择器定位

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);  // 【创建垂直布局】从上到下排列
    sidebarLayout->setContentsMargins(8, 12, 8, 12);  // 【设置边距】左8/上12/右8/下12
    sidebarLayout->setSpacing(4);                     // 【设置间距】控件之间间隔4像素
    sidebarLayout->setAlignment(Qt::AlignHCenter);    // 【水平居中】所有子控件在水平方向居中

    // ========== Logo区域 ==========
    QLabel* logoLabel = new QLabel(m_sidebar);  // 【创建Logo标签】显示应用图标
    logoLabel->setFixedSize(32, 32);  // 【固定大小】图标尺寸32x32像素
    logoLabel->setPixmap(IconHelper::logo(32, QColor("#FFFFFF")));  // 【设置图标】使用IconHelper生成白色Logo
    logoLabel->setToolTip("AI Assistant");  // 【鼠标悬停提示】当鼠标停在Logo上时显示"AI Assistant"
    sidebarLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);  // 【添加Logo】0表示不拉伸，水平居中
    sidebarLayout->addSpacing(8);  // 【添加间距】Logo和按钮之间留8像素空隙

    // ========== 主界面导航按钮（切换到对话页面） ==========
    m_navMain = new QPushButton(m_sidebar);  // 【创建按钮】第一个导航按钮
    m_navMain->setFixedSize(40, 40);         // 【固定大小】40x40像素
    m_navMain->setObjectName("navButton");   // 【对象名】用于QSS样式选择
    m_navMain->setProperty("navKey", "main"); // 【自定义属性】存储导航标识"main"
    m_navMain->setToolTip("主界面");          // 【悬停提示】显示"主界面"
    m_navMain->setIconSize(QSize(20, 20));   // 【图标尺寸】20x20像素
    m_navMain->setCursor(Qt::PointingHandCursor);  // 【鼠标样式】悬停时显示手型光标
    // 【信号槽连接】点击按钮时调用onNavButtonClicked(0)，0表示对话页面
    connect(m_navMain, &QPushButton::clicked, this, [this]() { onNavButtonClicked(0); });
    sidebarLayout->addWidget(m_navMain, 0, Qt::AlignHCenter);  // 【加入布局】水平居中

    // ========== 模型配置导航按钮 ==========
    m_navModel = new QPushButton(m_sidebar);  // 【创建按钮】第二个导航按钮
    m_navModel->setFixedSize(40, 40);         // 【固定大小】40x40像素
    m_navModel->setObjectName("navButton");   // 【对象名】QSS样式选择用
    m_navModel->setProperty("navKey", "model"); // 【自定义属性】标识为"model"
    m_navModel->setToolTip("模型配置");         // 【悬停提示】
    m_navModel->setIconSize(QSize(20, 20));   // 【图标尺寸】
    m_navModel->setCursor(Qt::PointingHandCursor);  // 【手型光标】
    // 【信号槽】点击切换到页面索引1（模型配置页面）
    connect(m_navModel, &QPushButton::clicked, this, [this]() { onNavButtonClicked(1); });
    sidebarLayout->addWidget(m_navModel, 0, Qt::AlignHCenter);

    // ========== 技能导入导航按钮 ==========
    m_navSkill = new QPushButton(m_sidebar);  // 【创建按钮】第三个导航按钮
    m_navSkill->setFixedSize(40, 40);         // 【固定大小】40x40像素
    m_navSkill->setObjectName("navButton");   // 【对象名】QSS样式选择用
    m_navSkill->setProperty("navKey", "skill"); // 【自定义属性】标识为"skill"
    m_navSkill->setToolTip("技能导入");         // 【悬停提示】
    m_navSkill->setIconSize(QSize(20, 20));   // 【图标尺寸】
    m_navSkill->setCursor(Qt::PointingHandCursor);  // 【手型光标】
    // 【信号槽】点击切换到页面索引2（技能导入页面）
    connect(m_navSkill, &QPushButton::clicked, this, [this]() { onNavButtonClicked(2); });
    sidebarLayout->addWidget(m_navSkill, 0, Qt::AlignHCenter);

    sidebarLayout->addStretch();  // 【添加弹簧】将剩余空间推到底部，使后续按钮靠下对齐

    // ========== 主题切换按钮 ==========

    m_themeToggle = new QPushButton(m_sidebar);  // 【创建按钮】主题切换
    m_themeToggle->setFixedSize(40, 40);         // 【固定大小】40x40像素
    m_themeToggle->setObjectName("navButton");   // 【对象名】QSS样式选择用
    m_themeToggle->setToolTip("切换主题");        // 【悬停提示】
    m_themeToggle->setIconSize(QSize(20, 20));   // 【图标尺寸】
    m_themeToggle->setCursor(Qt::PointingHandCursor);  // 【手型光标】
    // 【信号槽】点击调用onThemeToggleClicked切换深色/浅色主题
    connect(m_themeToggle, &QPushButton::clicked, this, &MainWindow::onThemeToggleClicked);
    sidebarLayout->addWidget(m_themeToggle, 0, Qt::AlignHCenter);

    updateNavButtons();  // 【初始化按钮状态】设置当前激活按钮的图标和样式
}

/**
 * @brief 设置右侧主内容区
 *
 * 主内容区采用垂直布局，顶部为标题栏（setupHeader），
 * 下方为QStackedWidget页面栈（setupPages），用于承载对话、模型配置、技能导入三个页面。
 */
void MainWindow::setupMainArea()
{
    m_mainArea = new QWidget(this);       // 【创建主区域容器】
    m_mainArea->setObjectName("mainArea"); // 【对象名】QSS样式定位用

    QVBoxLayout* mainLayout = new QVBoxLayout(m_mainArea);  // 【创建垂直布局】从上到下排列
    mainLayout->setContentsMargins(0, 0, 0, 0);  // 【无边距】内容紧贴边缘
    mainLayout->setSpacing(0);                   // 【无间距】标题栏和页面栈之间无空隙

    setupHeader();  // 【构建标题栏】顶部区域
    setupPages();   // 【构建页面栈】下方内容区域

    mainLayout->addWidget(m_header);   // 【添加标题栏】到垂直布局顶部
    mainLayout->addWidget(m_stack, 1); // 【添加页面栈】stretch=1占据所有剩余垂直空间
}

/**
 * @brief 设置顶部标题栏
 *
 * 构建固定高度的水平标题栏，左侧显示当前页面标题，
 * 右侧包含搜索输入框和当前主题模式标签。
 */
void MainWindow::setupHeader()
{
    m_header = new QWidget(m_mainArea);       // 【创建标题栏容器】父级为主区域
    m_header->setFixedHeight(48);             // 【固定高度】标题栏高48像素
    m_header->setObjectName("appHeader");      // 【对象名】QSS样式定位

    QHBoxLayout* headerLayout = new QHBoxLayout(m_header);  // 【创建水平布局】从左到右排列
    headerLayout->setContentsMargins(20, 0, 20, 0);  // 【边距】左右各20，上下为0
    headerLayout->setSpacing(16);                    // 【间距】控件之间16像素

    m_headerTitle = new QLabel("对话", m_header);  // 【创建标题标签】默认显示"对话"
    m_headerTitle->setObjectName("headerTitle");    // 【对象名】QSS样式定位
    m_headerTitle->setStyleSheet("font-size: 14px; font-weight: 600;");  // 【内联样式】14像素字体，半粗体
    headerLayout->addWidget(m_headerTitle);  // 【添加标题】到水平布局左侧

    headerLayout->addStretch();  // 【添加弹簧】将后续控件推到右侧

    // ========== 搜索输入框 ==========
    //m_searchInput = new QLineEdit(m_header);  // 【创建输入框】单行文本输入
    //m_searchInput->setObjectName("headerSearch");  // 【对象名】QSS样式定位
    //m_searchInput->setPlaceholderText("搜索...");   // 【占位文本】未输入时显示灰色提示
    //m_searchInput->setFixedSize(200, 32);           // 【固定尺寸】宽200高32像素
    // 【信号槽】文本变化时调用onSearchTextChanged
    //connect(m_searchInput, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    //headerLayout->addWidget(m_searchInput);  // 【添加搜索框】到标题栏右侧

    // ========== 主题模式标签 ==========
    m_themeLabel = new QLabel(
        ThemeManager::instance()->currentTheme() == ThemeManager::Dark ? "深色模式" : "浅色模式",
        m_header
    );  // 【创建标签】根据当前主题显示"深色模式"或"浅色模式"
    m_themeLabel->setObjectName("themeLabel");  // 【对象名】QSS样式定位
    m_themeLabel->setStyleSheet("font-size: 11px;");  // 【内联样式】11像素小字体
    headerLayout->addWidget(m_themeLabel);  // 【添加标签】到标题栏最右侧
}

/**
 * @brief 设置QStackedWidget页面栈
 *
 * 创建ChatPage、ModelConfigPage、SkillImportPage三个页面实例，
 * 按顺序添加到QStackedWidget中，并连接各页面发出的信号到主窗口对应槽函数。
 */
void MainWindow::setupPages()
{
    m_stack = new QStackedWidget(m_mainArea);  // 【创建堆叠控件】管理多个页面，每次只显示一个
    m_stack->setObjectName("pageStack");        // 【对象名】QSS样式定位

    m_chatPage = new ChatPage(m_stack);        // 【创建对话页面】聊天交互界面
    m_modelPage = new ModelConfigPage(m_stack); // 【创建模型配置页面】API地址、模型名等设置
    m_skillPage = new SkillImportPage(m_stack); // 【创建技能导入页面】导入外部技能包

    m_stack->addWidget(m_chatPage);   // 【添加页面】将对话页面加入堆叠，索引为0
    m_stack->addWidget(m_modelPage);  // 【添加页面】将模型配置页面加入堆叠，索引为1
    m_stack->addWidget(m_skillPage);  // 【添加页面】将技能导入页面加入堆叠，索引为2

    // ========== 连接对话页面的用户操作信号 ==========
    // 【信号槽】用户发送消息 -> 主窗口onMessageSent处理
    connect(m_chatPage, &ChatPage::sendMessage, this, &MainWindow::onMessageSent);
    // 【信号槽】用户点击停止 -> 主窗口onStopClicked中断任务
    connect(m_chatPage, &ChatPage::stopClicked, this, &MainWindow::onStopClicked);
    // 【信号槽】用户点击新建对话 -> 主窗口onNewChatClicked
    connect(m_chatPage, &ChatPage::newChatClicked, this, &MainWindow::onNewChatClicked);
    // 【信号槽】用户选择历史会话 -> 主窗口onConversationSelected切换会话
    connect(m_chatPage, &ChatPage::conversationSelected, this, &MainWindow::onConversationSelected);
    // 【信号槽】用户选择文件 -> 主窗口onFilesSelected保存文件列表
    connect(m_chatPage, &ChatPage::filesSelected, this, &MainWindow::onFilesSelected);
    // 【信号槽】用户删除会话 -> 主窗口onDeleteConversation删除并刷新列表
    connect(m_chatPage, &ChatPage::deleteConversation, this, &MainWindow::onDeleteConversation);

    // ========== 连接模型配置页面的设置变更信号 ==========
    // 【信号槽】设置变更 -> 主窗口onSettingsChanged同步到LLM客户端
    connect(m_modelPage, &ModelConfigPage::settingsChanged, this, &MainWindow::onSettingsChanged);
    // 【信号槽】点击测试连接 -> 主窗口onTestConnection验证API连通性
    connect(m_modelPage, &ModelConfigPage::testConnectionClicked, this, &MainWindow::onTestConnection);
    // 【信号槽】点击恢复默认 -> 主窗口onRestoreDefaults重置配置
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
    ThemeManager* tm = ThemeManager::instance();  // 【获取主题管理器单例】
    QString bg = tm->background().name();         // 【背景色】主背景色，如#1E1E1E
    QString bg2 = tm->bgSecondary().name();       // 【次要背景色】用于标题栏等
    QString bg3 = tm->bgTertiary().name();        // 【三级背景色】用于悬停效果
    QString bdr = tm->border().name();            // 【边框色】分割线颜色
    QString pri = tm->primary().name();           // 【主题色】品牌主色，如蓝色
    QString txtPri = tm->textPrimary().name();    // 【主文字色】标题等重要文字
    QString txtSec = tm->textSecondary().name();  // 【次要文字色】正文文字
    QString txtTer = tm->textTertiary().name();   // 【三级文字色】提示文字、禁用状态

    // 【设置侧边栏样式】圆角卡片效果，导航按钮透明背景+悬停变色+激活高亮
    m_sidebar->setStyleSheet(QString(
        "QWidget#appSidebar { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        "QPushButton#navButton { background-color: transparent; border: none; border-radius: 10px; }"
        "QPushButton#navButton:hover { background-color: %3; }"
        "QPushButton#navButton[active=true] { background-color: %5; }"
    ).arg(bg, bg2, bg3, bdr, pri, txtPri, txtSec, txtTer, txtTer));
    // 【说明】arg按顺序填充%1~%9占位符，%1=bg, %2=bg2, %3=bg3, %4=bdr, %5=pri, ...

    // 【设置主区域样式】包含主区域背景、标题栏、标题文字、搜索框、主题标签、页面栈
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
    bool isDark = ThemeManager::instance()->currentTheme() == ThemeManager::Dark;  // 【判断主题】是否为深色模式
    QColor activeColor = isDark ? QColor("#FFFFFF") : QColor("#FFFFFF");  // 【激活图标色】白色（深色/浅色下都用白色）
    QColor inactiveColor = ThemeManager::instance()->textTertiary();       // 【未激活图标色】灰色

    // 【设置active属性】用于QSS选择器[active=true]匹配
    m_navMain->setProperty("active", m_currentPage == 0);   // 如果当前是第0页，设active为true
    m_navModel->setProperty("active", m_currentPage == 1);  // 如果当前是第1页，设active为true
    m_navSkill->setProperty("active", m_currentPage == 2);  // 如果当前是第2页，设active为true

    // 【设置图标】当前页用白色图标，其他页用灰色图标
    m_navMain->setIcon(QIcon(IconHelper::message(20, m_currentPage == 0 ? activeColor : inactiveColor)));
    m_navModel->setIcon(QIcon(IconHelper::chip(20, m_currentPage == 1 ? activeColor : inactiveColor)));
    m_navSkill->setIcon(QIcon(IconHelper::layers(20, m_currentPage == 2 ? activeColor : inactiveColor)));

    // 【强制重绘按钮】unpolish移除旧样式，polish重新应用新样式，确保[active=true]生效
    for (auto* btn : {m_navMain, m_navModel, m_navSkill}) {
        btn->style()->unpolish(btn);  // 【清除旧样式缓存】让样式引擎重新评估
        btn->style()->polish(btn);    // 【重新应用样式】根据当前属性值应用QSS
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
    m_currentPage = pageIndex;  // 【更新当前页索引】记录当前显示的页面
    m_stack->setCurrentIndex(pageIndex);  // 【切换页面】让QStackedWidget显示对应索引的页面

    QStringList titles = {"对话", "模型配置", "技能导入"};  // 【标题列表】三个页面对应的标题
    m_headerTitle->setText(titles[pageIndex]);  // 【更新标题栏文字】显示当前页面名称

    updateNavButtons();  // 【刷新按钮状态】更新导航按钮的激活样式和图标
}

/**
 * @brief 主题切换按钮点击槽函数
 *
 * 调用ThemeManager单例切换当前主题（深色/浅色）。
 */
void MainWindow::onThemeToggleClicked()
{
    ThemeManager::instance()->toggleTheme();  // 【切换主题】ThemeManager内部保存当前主题状态并反转
}

/**
 * @brief 主题变化响应槽函数
 *
 * 重新应用窗口样式、更新主题切换按钮图标、刷新主题标签文本，
 * 并同步更新导航按钮及全局QApplication样式表。
 */
void MainWindow::onThemeChanged()
{
    applyStyles();  // 【重新应用样式】根据新主题生成并设置QSS

    bool isDark = ThemeManager::instance()->currentTheme() == ThemeManager::Dark;  // 【判断新主题】
    QColor iconColor = ThemeManager::instance()->textSecondary();  // 【获取图标颜色】
    // 【更新主题按钮图标】深色模式显示太阳图标（切到浅色），浅色模式显示月亮图标（切到深色）
    m_themeToggle->setIcon(QIcon(isDark ? IconHelper::sun(20, iconColor) : IconHelper::moon(20, iconColor)));
    m_themeLabel->setText(isDark ? "深色模式" : "浅色模式");  // 【更新主题标签文字】

    updateNavButtons();  // 【刷新导航按钮】图标颜色可能随主题变化
    qApp->setStyleSheet(ThemeManager::instance()->styleSheet());  // 【更新全局样式表】影响所有子窗口
}

/**
 * @brief 新建对话按钮点击槽函数
 *
 * 清空对话页面的聊天记录与任务计划状态。
 */
void MainWindow::onNewChatClicked()
{
    // 【保存当前对话】在创建新对话前，先将当前对话历史保存到文件
    saveConfig();

    // 【创建新对话】让Orchestrator在内部创建一个新的会话对象
    m_orchestrator->createNewConversation();

    // 【同步对话列表】从Orchestrator获取最新对话历史，刷新左侧列表显示
    QJsonArray convHistory = m_orchestrator->conversationHistory();
    m_chatPage->loadConversations(convHistory);

    m_chatPage->clearChat();  // 【清空聊天区】移除所有消息气泡
    m_chatPage->clearPlan();  // 【清空计划区】移除任务计划卡片
}

/**
 * @brief 搜索文本变化槽函数
 * @param text 当前搜索框文本
 *
 * 当前为占位实现，未启用实际搜索逻辑。
 */
void MainWindow::onSearchTextChanged(const QString& text)
{
    Q_UNUSED(text)  // 【消除未使用参数警告】text参数当前未使用，Q_UNUSED告诉编译器忽略
}

/**
 * @brief 用户选择历史会话槽函数
 * @param index 选中会话的索引
 *
 * 切换Orchestrator到指定会话，并在对话页面加载该会话的消息记录。
 */
void MainWindow::onConversationSelected(int index)
{
    m_orchestrator->switchConversation(index);       // 【切换会话】让Orchestrator激活指定索引的会话
    m_chatPage->loadConversationMessages(index);     // 【加载消息】将选中会话的消息显示到聊天区
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
    if (m_orchestrator->isProcessing()) {  // 【检查是否忙】如果正在处理上一条消息，则忽略新消息
        return;  // 【直接返回】不处理重复提交
    }

    m_chatPage->addMessage("用户", message, true);  // 【显示用户消息】在对话区添加用户发送的内容，true表示是用户消息
    m_chatPage->showStopButton(true);                // 【显示停止按钮】让用户可以中断执行

    QStringList files = m_chatPage->selectedFiles();  // 【获取附件】读取用户选中的文件列表
    if (!files.isEmpty()) {  // 【判断是否有附件】如果有文件，走带文件的处理流程
        m_orchestrator->processQueryWithFiles(message, files);  // 【提交消息+文件】给Orchestrator处理
        m_chatPage->clearSelectedFiles();  // 【清空附件】发送后清除已选文件
    } else {
        m_orchestrator->processQuery(message);  // 【仅提交文本】给Orchestrator处理
    }
}

/**
 * @brief 文件选择处理槽函数
 * @param files 用户选中的文件路径列表
 *
 * 保存用户选中的文件列表，供后续发送消息时使用。
 */
void MainWindow::onFilesSelected(const QStringList& files)
{
    m_selectedFiles = files;  // 【保存文件列表】将选中的文件路径存储到成员变量
}

/**
 * @brief 删除对话槽函数
 * @param index 要删除的对话索引
 *
 * 让Orchestrator删除指定对话，刷新对话列表显示，并保存配置。
 */
void MainWindow::onDeleteConversation(int index)
{
    m_orchestrator->deleteConversation(index);  // 【删除对话】从Orchestrator中移除
    m_chatPage->loadConversations(m_orchestrator->conversationHistory());  // 【刷新列表】重新加载并显示
    saveConfig();  // 【保存配置】将更新后的对话历史写入文件
}

/**
 * @brief 停止执行按钮点击槽函数
 *
 * 调用Orchestrator停止当前任务执行，并隐藏停止按钮。
 */
void MainWindow::onStopClicked()
{
    m_orchestrator->stopExecution();   // 【停止执行】向Orchestrator发送停止信号
    m_chatPage->showStopButton(false); // 【隐藏停止按钮】任务已停止，无需再显示
}

/**
 * @brief 任务计划生成完成槽函数
 * @param plan 生成的任务计划对象
 *
 * 将计划同步到对话页面，并在聊天区域渲染计划概览卡片（含步骤总数）。
 */
void MainWindow::onPlanGenerated(const TaskPlan& plan)
{
    m_chatPage->setPlan(plan);  // 【设置计划】将计划数据传给对话页面显示
    m_chatPage->appendOutput(   // 【追加HTML卡片】显示蓝色计划生成提示框
        QString("<div style='padding: 10px 14px; margin: 8px 0; background: rgba(59, 130, 246, 0.1); border: 1px solid #3B82F6; border-radius: 10px;'>"
                "<span style='color: #3B82F6; font-weight: 600;'>📋 计划生成</span> "
                "<span style='color: #94A3B8;'>共 %1 个步骤</span>"
                "</div>").arg(plan.totalSteps())  // 【替换%1】为实际步骤数
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
    m_chatPage->updateStepStatus(stepId, StepStatus::Running);  // 【更新状态】将该步骤UI设为运行中（显示转圈动画）
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
    QString description;  // 【临时变量】存储步骤描述
    // 【查找步骤描述】遍历当前计划的所有步骤，找到stepId匹配的描述
    for (const auto& step : m_orchestrator->currentPlan().steps) {
        if (step.stepId == stepId) {  // 【匹配ID】找到对应步骤
            description = step.description;  // 【保存描述】如"读取文件"、"编译代码"
            break;  // 【跳出循环】已找到，无需继续遍历
        }
    }

    if (output.startsWith("===")) {  // 【判断输出类型】以"==="开头表示步骤标题分隔线
        m_chatPage->appendStepHeader(stepId, description);  // 【渲染步骤标题】显示步骤名称
    } else {
        m_chatPage->appendStepOutput(stepId, output);  // 【追加普通输出】将日志追加到步骤输出区
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
    m_chatPage->updateStepStatus(stepId, StepStatus::Completed);  // 【更新状态】该步骤标记为已完成
    QString resultMsg = result.value("result", "").toString();    // 【提取结果】从结果字典中取"result"字段
    if (resultMsg.length() > 100) {  // 【限制长度】如果结果超过100字符
        resultMsg = resultMsg.left(100) + "...";  // 【截断】只保留前100字符并加省略号
    }
    m_chatPage->appendStepResult(stepId, true, resultMsg);  // 【显示结果】在步骤卡片中显示成功和摘要
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
    m_chatPage->updateStepStatus(stepId, StepStatus::Failed);  // 【更新状态】该步骤标记为失败
    m_chatPage->appendStepResult(stepId, false, error);        // 【显示错误】在步骤卡片中显示错误信息
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
    m_chatPage->showStopButton(false);  // 【隐藏停止按钮】任务已完成，不需要停止

    int success = 0, fail = 0;  // 【计数器】统计成功和失败的步骤数
    for (const auto& step : plan.steps) {  // 【遍历所有步骤】
        if (step.status == StepStatus::Completed) success++;  // 【成功+1】
        else if (step.status == StepStatus::Failed) fail++;   // 【失败+1】
    }

    m_chatPage->appendOutput(  // 【追加总结卡片】绿色背景的成功提示框
        QString("<div style='padding: 12px; margin: 8px 0; background: rgba(34, 197, 94, 0.1); border: 1px solid #22C55E; border-radius: 10px;'>"
                "<div style='color: #22C55E; font-weight: 700; font-size: 15px;'>🎉 任务执行完成</div>"
                "<div style='color: #94A3B8; font-size: 13px; margin-top: 4px;'>"
                "总计 %1 步 | 成功 %2 | 失败 %3"
                "</div></div>").arg(plan.totalSteps()).arg(success).arg(fail)  // 【填充统计数字】
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
    m_chatPage->addMessage("AI", message, false);  // 【显示AI消息】false表示这是AI回复（显示在左侧）
}

/**
 * @brief Orchestrator发生错误槽函数
 * @param error 错误描述文本
 *
 * 隐藏停止按钮，并在对话页面渲染红色错误提示卡片。
 */
void MainWindow::onOrchestratorError(const QString& error)
{
    m_chatPage->showStopButton(false);   // 【隐藏停止按钮】出错后任务已终止
    m_chatPage->finishCurrentMessage();  // 【结束当前消息】停止流式输出动画
    m_chatPage->appendOutput(  // 【显示错误卡片】红色背景和文字
        QString("<div style='padding: 10px 14px; margin: 4px 0; background: rgba(239, 68, 68, 0.1); border: 1px solid #EF4444; border-radius: 8px;'>"
                "<span style='color: #EF4444; font-weight: 600;'>❌ 错误:</span> "
                "<span style='color: #FCA5A5;'>%1</span>"
                "</div>").arg(error.toHtmlEscaped())  // 【转义HTML】防止错误消息中的特殊字符破坏布局
    );
}

/**
 * @brief 收到流式响应片段槽函数
 * @param chunk AI返回的文本片段（逐字或逐句）
 *
 * 将流式片段实时追加到对话页面当前正在显示的消息中。
 */
void MainWindow::onResponseChunk(const QString& chunk)
{
    m_chatPage->appendMessageChunk(chunk);  // 【追加片段】实时显示打字机效果
}

/**
 * @brief 流式响应完成槽函数
 *
 * 结束对话页面当前消息的流式显示状态。
 */
void MainWindow::onResponseCompleted()
{
    m_chatPage->finishCurrentMessage();  // 【结束消息】停止打字机动画，显示完整内容
}

/**
 * @brief 收到日志消息槽函数
 * @param logType 日志类型（"llm"/"agent"/"system"/其他）
 * @param message 日志内容
 *
 * 根据日志类型显示不同颜色和前缀，将格式化的日志追加到对话页面。
 */
void MainWindow::onLogMessage(const QString& logType, const QString& message)
{
    QString color;   // 【颜色变量】根据类型设置不同颜色
    QString prefix;  // 【前缀变量】日志类型标识
    if (logType == "llm") {  // 【LLM类型】紫色
        color = "#6366F1";
        prefix = "[LLM]";
    } else if (logType == "agent") {  // 【Agent类型】绿色
        color = "#10B981";
        prefix = "[Agent]";
    } else if (logType == "system") {  // 【系统类型】橙色
        color = "#F59E0B";
        prefix = "[系统]";
    } else {  // 【其他类型】灰色
        color = "#6B7280";
        prefix = "[执行]";
    }

    // 【构建HTML】带颜色的日志条目
    QString html = QString("<div style='padding: 4px 8px; font-size: 12px;'>"
                           "<span style='color: %1; font-weight: 600;'>%2</span> "  // 彩色前缀
                           "<span style='color: %3;'>%4</span>"                      // 灰色内容
                           "</div>")
                          .arg(color)                                           // %1=前缀颜色
                          .arg(prefix)                                          // %2=前缀文字
                          .arg(ThemeManager::instance()->textSecondary().name()) // %3=内容颜色
                          .arg(message.toHtmlEscaped());                        // %4=转义后的日志内容

    m_chatPage->appendLog(html);  // 【追加日志】将格式化后的HTML添加到日志区域
}

/**
 * @brief 模型设置变更保存槽函数
 *
 * 从模型配置页面读取最新参数（API地址、模型名、最大Token数、温度值），
 * 同步到LlmClient实例，并弹出保存成功提示。
 */
void MainWindow::onSettingsChanged()
{
    m_llmClient->setApiUrl(m_modelPage->apiUrl());              // 【同步API地址】
    m_llmClient->setModelName(m_modelPage->modelName());        // 【同步模型名称】
    m_llmClient->setMaxTokens(m_modelPage->maxTokens());        // 【同步最大Token数】
    m_llmClient->setTemperature(m_modelPage->temperature());    // 【同步温度值】控制AI回答随机性

    saveConfig();  // 【保存配置】将新设置写入本地config.json文件

    QMessageBox::information(this, "设置已保存", "模型配置已成功保存。");  // 【弹窗提示】告诉用户保存成功
}

/**
 * @brief 测试连接按钮点击槽函数
 *
 * 向 API 端点发送 GET /v1/models 请求验证连通性。
 * 成功后获取已安装模型列表并更新连接状态显示。
 */
void MainWindow::onTestConnection()
{
    QString apiUrl = m_modelPage->apiUrl().trimmed();  // 【获取并清理】去除首尾空白字符

    if (apiUrl.isEmpty()) {  // 【检查空地址】如果用户没填API地址
        QMessageBox::warning(this, "测试连接", "请先输入模型端点地址。");  // 【警告提示】
        return;  // 【直接返回】不再继续执行
    }

    // 【构造模型列表请求路径】
    QString modelsUrl = apiUrl;  // 【复制地址】基于用户输入的API地址
    if (modelsUrl.endsWith('/')) {  // 【去除末尾斜杠】避免路径出现双斜杠
        modelsUrl.chop(1);  // 【删除最后一个字符】即/
    }
    // 【适配不同API路径格式】转换为/v1/models端点
    if (modelsUrl.contains("/v1/chat/completions")) {  // 【OpenAI格式】聊天补全路径
        modelsUrl.replace("/v1/chat/completions", "/v1/models");  // 【替换为模型列表路径】
    } else if (modelsUrl.contains("/v1")) {  // 【已有/v1】直接追加/models
        modelsUrl += "/models";
    } else {  // 【基础路径】追加完整/v1/models
        modelsUrl += "/v1/models";
    }

    if (!m_testManager) {  // 【懒加载】如果测试管理器还没创建
        m_testManager = new QNetworkAccessManager(this);  // 【创建网络管理器】用于发起HTTP请求
    }

    QUrl url(modelsUrl);  // 【构造URL】将字符串转为QUrl对象
    QNetworkRequest request(url);  // 【构造请求】
    QNetworkReply* reply = m_testManager->get(request);  // 【发送GET请求】异步获取模型列表

    m_modelPage->updateConnectionStatus(false, "正在连接...");  // 【更新UI】显示连接中状态
    m_modelPage->setTestButtonEnabled(false);  // 【禁用按钮】防止重复点击

    // 【Lambda回调】请求完成后执行
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();  // 【延迟删除】请求完成后自动释放reply内存
        m_modelPage->setTestButtonEnabled(true);  // 【恢复按钮】允许再次点击

        if (reply->error() != QNetworkReply::NoError) {  // 【检查错误】如果网络请求失败
            QString error = reply->errorString();  // 【获取错误信息】
            m_modelPage->updateConnectionStatus(false, "连接失败: " + error);  // 【更新UI】显示失败
            QMessageBox::warning(this, "连接失败",  // 【弹窗警告】
                "无法连接到模型服务。\n地址: " + m_modelPage->apiUrl() + "\n错误: " + error);
            return;  // 【返回】不再处理响应数据
        }

        QByteArray data = reply->readAll();  // 【读取响应】获取服务器返回的全部数据
        QJsonDocument doc = QJsonDocument::fromJson(data);  // 【解析JSON】将字节流转为JSON文档

        QStringList modelNames;  // 【存储模型名】
        if (doc.isObject() && doc.object().contains("data")) {  // 【检查格式】OpenAI标准格式包含data数组
            QJsonArray models = doc.object()["data"].toArray();  // 【提取数组】
            for (const auto& model : models) {  // 【遍历模型】
                QString id = model.toObject()["id"].toString();  // 【提取ID】模型唯一标识
                if (!id.isEmpty()) {  // 【过滤空值】
                    modelNames.append(id);  // 【加入列表】
                }
            }
        }

        if (modelNames.isEmpty()) {  // 【无模型列表】某些API可能返回不同格式
            modelNames.append("default");  // 【兜底】至少显示一个默认值
        }

        // 【更新LLM客户端配置】
        m_llmClient->setApiUrl(m_modelPage->apiUrl());

        // 【更新模型详情显示】
        QString detail = modelNames.first();  // 【取第一个模型名】显示在状态栏
        m_modelPage->updateConnectionStatus(true, detail);  // 【更新UI】显示连接成功和模型名
        m_modelPage->populateInstalledModels(modelNames);   // 【填充下拉框】将所有模型加入选择列表

        QMessageBox::information(this, "连接成功",  // 【弹窗提示】
            QString("成功连接到模型服务。\n已发现 %1 个模型。").arg(modelNames.size()));

        saveConfig();  // 【保存配置】将检测到的模型列表写入文件
    });
}

/**
 * @brief 恢复默认设置按钮点击槽函数
 *
 * 将模型配置页面重置为初始状态。
 */
void MainWindow::onRestoreDefaults()
{
    m_modelPage->onRestoreDefaultsClicked();  // 【调用页面方法】让模型配置页面执行恢复默认逻辑
    QMessageBox::information(this, "恢复默认", "已恢复默认设置。");  // 【弹窗提示】
}

/**
 * @brief 保存配置到文件
 *
 * 将模型配置参数和对话历史分别保存到config.json和conversations/conversations.json。
 * 如果目录不存在会自动创建。
 */
void MainWindow::saveConfig()
{
    QDir().mkpath(QCoreApplication::applicationDirPath());  // 【确保目录存在】创建程序所在目录（如果不存在）

    QJsonObject config;  // 【创建JSON对象】存储所有配置项
    config["apiUrl"] = m_modelPage->apiUrl();              // 【保存API地址】如http://localhost:8080/v1
    config["modelName"] = m_modelPage->modelName();        // 【保存模型名称】如Qwen-7B-Instruct
    config["maxTokens"] = m_modelPage->maxTokens();        // 【保存最大Token数】如4096
    config["temperature"] = m_modelPage->temperature();    // 【保存温度值】如0.7
    config["topP"] = m_modelPage->topP();                  // 【保存topP值】核采样参数
    config["contextLength"] = m_modelPage->contextLength(); // 【保存上下文长度】
    config["gpuMemoryLimit"] = m_modelPage->gpuMemoryLimit(); // 【保存GPU显存限制】
    config["parallelThreads"] = m_modelPage->parallelThreads(); // 【保存并行线程数】
    config["backend"] = m_modelPage->backend();            // 【保存后端类型】如llama.cpp

    QStringList installedModels = m_modelPage->installedModelNames();  // 【获取已安装模型列表】
    if (!installedModels.isEmpty()) {  // 【如果有模型】才保存
        QJsonArray modelsArray;  // 【创建JSON数组】
        for (const QString& name : installedModels) {  // 【遍历模型名】
            modelsArray.append(name);  // 【加入数组】
        }
        config["installedModels"] = modelsArray;  // 【存入配置对象】
    }

    QFile configFile(configFilePath());  // 【打开配置文件】
    if (configFile.open(QIODevice::WriteOnly)) {  // 【以只写模式打开】会覆盖旧内容
        QJsonDocument doc(config);  // 【创建JSON文档】
        configFile.write(doc.toJson(QJsonDocument::Indented));  // 【写入文件】Indented表示格式化缩进
        configFile.close();  // 【关闭文件】释放文件句柄
    }

    QDir convDir(QCoreApplication::applicationDirPath() + "/conversations");  // 【对话目录对象】
    convDir.mkpath(".");  // 【确保目录存在】创建conversations文件夹

    QFile convFile(QCoreApplication::applicationDirPath() + "/conversations/conversations.json");  // 【对话历史文件】
    if (convFile.open(QIODevice::WriteOnly)) {  // 【以只写模式打开】
        QJsonDocument doc(m_orchestrator->conversationHistory());  // 【获取对话历史】从Orchestrator取JSON数组
        convFile.write(doc.toJson(QJsonDocument::Indented));  // 【写入文件】格式化JSON
        convFile.close();  // 【关闭文件】
    }
}

/**
 * @brief 从文件加载配置
 *
 * 读取config.json恢复模型设置并同步到LLM客户端，
 * 读取conversations.json恢复对话历史和当前会话。
 */
void MainWindow::loadConfig()
{
    QFile configFile(configFilePath());  // 【打开配置文件】
    if (configFile.open(QIODevice::ReadOnly)) {  // 【以只读模式打开】
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());  // 【读取并解析JSON】
        configFile.close();  // 【关闭文件】

        if (doc.isObject()) {  // 【检查是否为对象】
            QJsonObject config = doc.object();  // 【提取JSON对象】
            // 【恢复模型页面设置】如果键不存在则使用默认值
            m_modelPage->setApiUrl(config["apiUrl"].toString("http://localhost:8080/v1"));  // 默认本地8080端口
            m_modelPage->setModelName(config["modelName"].toString("Qwen-7B-Instruct"));     // 默认Qwen模型
            m_modelPage->setMaxTokens(config["maxTokens"].toInt(4096));                      // 默认4096 tokens
            m_modelPage->setTemperature(config["temperature"].toDouble(0.7));                // 默认温度0.7

            // 【同步到LLM客户端】让网络层也使用加载的配置
            m_llmClient->setApiUrl(config["apiUrl"].toString("http://localhost:8080/v1"));
            m_llmClient->setModelName(config["modelName"].toString("Qwen-7B-Instruct"));
            m_llmClient->setMaxTokens(config["maxTokens"].toInt(4096));
            m_llmClient->setTemperature(config["temperature"].toDouble(0.7));

            QStringList modelNames;  // 【读取已安装模型】
            if (config.contains("installedModels")) {  // 【检查是否存在】
                QJsonArray models = config["installedModels"].toArray();  // 【提取数组】
                for (const auto& model : models) {  // 【遍历】
                    modelNames.append(model.toString());  // 【加入列表】
                }
            }
            if (!modelNames.isEmpty()) {  // 【如果有模型】
                m_modelPage->populateInstalledModels(modelNames);  // 【填充下拉框】
            }
        }
    }

    QFile convFile(conversationsFilePath());  // 【打开对话历史文件】
    if (convFile.open(QIODevice::ReadOnly)) {  // 【以只读模式打开】
        QJsonDocument doc = QJsonDocument::fromJson(convFile.readAll());  // 【读取并解析】
        convFile.close();  // 【关闭文件】

        if (doc.isArray()) {  // 【检查是否为数组】
            m_orchestrator->loadConversationHistory(doc.array());  // 【加载到协调器】恢复所有会话
            m_chatPage->loadConversations(doc.array());            // 【加载到对话页面】显示会话列表

            int currentIndex = m_orchestrator->currentConversationIndex();  // 【获取当前会话索引】
            if (currentIndex >= 0) {  // 【如果有有效索引】
                m_chatPage->loadConversationMessages(currentIndex);  // 【加载消息】显示当前会话的聊天内容
            }
        }
    }
}
