/**
 * @file chatpage.cpp
 * @brief 聊天页面实现文件
 *
 * 【整体功能】
 * 本文件实现了 ChatPage 类的所有成员函数，构建完整的三栏式聊天界面：
 * 左侧边栏（对话历史）、中间聊天区（消息气泡+输入框）、右侧面板（输出日志/任务进度）。
 *
 * 【关键技术点】
 * 1. Markdown转HTML：将AI返回的Markdown格式文本转换为HTML显示（代码块、粗体、斜体、标题等）
 * 2. 打字机效果：通过QTimer每30ms追加5个字符，模拟流式输出
 * 3. 消息气泡：使用HTML表格+div实现左右对齐的彩色气泡
 * 4. 主题适配：通过ThemeManager动态获取颜色，应用到QSS样式表
 */

#include "chatpage.h"           // 本类头文件
#include "thememanager.h"       // 主题管理器（获取暗色/亮色颜色值）
#include "iconhelper.h"         // 图标辅助类（绘制矢量图标）
#include "outputpanel.h"        // 输出面板（右侧面板之一）
#include "tasklistpanel.h"      // 任务列表面板（右侧面板之二）
#include <QJsonArray>           // JSON数组（对话历史数据）
#include <QJsonObject>          // JSON对象
#include <QJsonDocument>        // JSON文档
#include <QDateTime>            // 日期时间（消息时间戳）
#include <QApplication>         // 应用程序（获取剪贴板等）
#include <QClipboard>           // 剪贴板（复制功能）
#include <QScrollBar>           // 滚动条（自动滚动到底部）
#include <QFrame>               // 框架控件（分隔线）
#include <QDesktopServices>     // 桌面服务（打开文件/链接）
#include <QUrl>                 // URL处理
#include <QFileDialog>          // 文件对话框（选择附件）
#include <QFileInfo>            // 文件信息（获取文件名、大小等）
#include <QLayout>              // 布局基类

/**
 * @brief 将Markdown格式文本转换为HTML
 * @param markdown Markdown原始文本
 * @return 转换后的HTML字符串
 *
 * 【转换规则详解】
 * 1. 先将特殊HTML字符转义（< > &等），防止XSS攻击
 * 2. 代码块 ```language\ncode\n``` → <pre><code class="language-xxx">code</code></pre>
 * 3. 行内代码 `code` → <code>code</code>
 * 4. 粗体 **text** → <strong>text</strong>
 * 5. 斜体 *text* → <em>text</em>
 * 6. 标题 # text → <h1>text</h1>（支持1-6级）
 * 7. 链接 [text](url) → <a href="url">text</a>
 * 8. 列表 - item → <li>item</li>（简单实现，自动包裹<ul>）
 * 9. 换行符 \n → <br>
 *
 * 【调试提示】若某些Markdown格式显示不正确，可在此函数中添加新的正则替换规则。
 */
static QString markdownToHtml(const QString& markdown)
{
    QString html = markdown.toHtmlEscaped();  // 【安全处理】将 &, <, > 等转义为HTML实体，防止注入

    // 【代码块】匹配 ```language\ncode\n``` 格式
    QRegularExpression codeBlock(R"(```(\w*)\n([\s\S]*?)```)");
    html.replace(codeBlock, "<pre><code class=\"language-\\1\">\\2</code></pre>");
    // 视觉效果：等宽字体显示，带有语言类名，可由外部CSS控制高亮

    // 【行内代码】匹配 `code` 格式
    QRegularExpression inlineCode(R"(``([^`]+)``)");
    html.replace(inlineCode, "<code>\\1</code>");

    // 【粗体】匹配 **text** 格式
    QRegularExpression bold(R"(\*\*([^*]+)\*\*)");
    html.replace(bold, "<strong>\\1</strong>");
    // 视觉效果：文字加粗显示

    // 【斜体】匹配 *text* 格式（注意要在粗体之后处理，避免冲突）
    QRegularExpression italic(R"(\*([^*]+)\*)");
    html.replace(italic, "<em>\\1</em>");
    // 视觉效果：文字倾斜显示

    // 【标题】匹配 # text 到 ###### text 格式
    QRegularExpression heading(R"(^(#{1,6})\s+(.+)$)", QRegularExpression::MultilineOption);
    {
        QString result;
        int lastEnd = 0;
        QRegularExpressionMatchIterator it = heading.globalMatch(html);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            result += html.mid(lastEnd, match.capturedStart() - lastEnd);
            int level = match.captured(1).length();  // #数量 = 标题级别
            QString text = match.captured(2);
            result += QString("<h%1>%2</h%1>").arg(level).arg(text);
            // 视觉效果：h1最大最粗，h6最小
            lastEnd = match.capturedEnd();
        }
        result += html.mid(lastEnd);
        html = result;
    }

    // 【链接】匹配 [text](url) 格式
    QRegularExpression link(R"(\[([^\]]+)\]\(([^)]+)\))");
    html.replace(link, "<a href=\"\\2\" target=\"_blank\">\\1</a>");
    // target="_blank" 表示点击后在新窗口/标签打开链接

    // 【列表项】匹配 - item 或 * item 或 + item 格式
    QRegularExpression list(R"(^\s*[-*+]\s+(.+)$)", QRegularExpression::MultilineOption);
    html.replace(list, "<li>\\1</li>");

    // 【换行处理】将普通换行转为<br>标签
    html.replace("\n", "<br>");

    // 【列表包裹】简单处理，将<li>前后包裹<ul>标签
    html.replace("<li>", "<ul><li>");
    html.replace("</li><ul>", "</li>");
    html.replace("</li><br>", "</li></ul><br>");
    html.replace("</li></ul></ul>", "</li></ul>");

    return html;
}

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 *
 * 【初始化流程详解】
 * 1. 初始化成员变量（当前对话索引0，打字机位置0）
 * 2. 创建打字机定时器（30ms间隔）→ 连接 onTypingTimer() 槽函数
 * 3. 创建自动保存定时器（3000ms=3秒间隔）→ 连接 onAutoSaveTimer() 槽函数 → 启动
 * 4. 调用 setupUI() 构建三栏界面
 * 5. 调用 applyTheme() 应用颜色主题
 * 6. 初始化对话列表（添加默认示例对话）
 * 7. 连接 ThemeManager 主题变化信号
 */
ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent),
      m_currentConversationIndex(0),  // 默认选中第0个对话
      m_pendingIndex(0)               // 打字机效果从第0个字符开始
{
    // 【打字机定时器】每30毫秒触发一次，每次显示5个字符
    m_typingTimer = new QTimer(this);
    m_typingTimer->setInterval(30);   // 间隔30ms；改大如100则打字更慢，改10则更快
    connect(m_typingTimer, &QTimer::timeout, this, &ChatPage::onTypingTimer);
    // 信号槽说明：QTimer::timeout 是定时器到期信号；连接后每30ms自动调用 onTypingTimer()

    // 【自动保存定时器】每3秒触发一次，自动同步对话标题和时间
    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setInterval(3000);  // 3000ms = 3秒；改大如同步间隔更长
    connect(m_autoSaveTimer, &QTimer::timeout, this, &ChatPage::onAutoSaveTimer);
    m_autoSaveTimer->start();         // 【启动定时器】开始自动保存循环

    setupUI();                        // 构建界面
    applyTheme();                     // 应用主题颜色

    updateConversationList();         // 初始化侧边栏对话列表

    // 连接主题变化信号，切换暗色/亮色时自动刷新样式
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ChatPage::onThemeChanged);
}

/**
 * @brief 析构函数
 *
 * 【Qt自动内存管理】所有子控件在创建时都指定了this作为父对象，
 * ChatPage销毁时Qt会自动删除所有子控件（定时器、布局、按钮等）。
 */
ChatPage::~ChatPage()
{
}

/**
 * @brief 构建整体三栏布局
 *
 * 【布局结构】
 * ChatPage (this)
 * └── mainLayout (QHBoxLayout, 无间距无内边距)
 *     ├── m_sidebar (左侧边栏, 固定260px)
 *     └── m_mainSplitter (QSplitter, 水平分割)
 *         ├── m_chatArea (中间聊天区, 拉伸因子3)
 *         └── m_rightPanel (右侧面板, 拉伸因子1)
 *
 * 【拉伸因子说明】setStretchFactor(0,3)表示聊天区占3份，setStretchFactor(1,1)表示右侧面板占1份，
 * 因此聊天区宽度是右侧面板的3倍。当窗口大小变化时，按比例分配额外空间。
 */
void ChatPage::setupUI()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);  // 无内边距，三栏紧贴窗口边缘
    mainLayout->setSpacing(0);                   // 控件之间无间距

    setupSidebar();      // 构建左侧边栏
    setupChatArea();     // 构建中间聊天区
    setupRightPanel();   // 构建右侧面板

    // 【主分割器】可拖拽调整中间聊天区和右侧面板的宽度
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setChildrenCollapsible(false);  // 不允许子面板被完全折叠；改true则可拖没
    m_mainSplitter->addWidget(m_chatArea);          // 添加聊天区
    m_mainSplitter->addWidget(m_rightPanel);        // 添加右侧面板
    m_mainSplitter->setStretchFactor(0, 3);         // 聊天区占3份宽度
    m_mainSplitter->setStretchFactor(1, 1);         // 右侧面板占1份宽度

    mainLayout->addWidget(m_sidebar);               // 左侧边栏固定宽度
    mainLayout->addWidget(m_mainSplitter, 1);       // 分割器占据剩余所有空间；1是拉伸因子
}

/**
 * @brief 构建左侧对话历史侧边栏
 *
 * 【视觉效果】
 * - 固定宽度260px，暗色背景（#1E293B）
 * - 顶部："对话记录"标题 + "+ 新建"蓝色按钮
 * - 中间：对话列表（带悬停和选中效果）
 * - 底部：绿色状态点 + "离线模式 · 本地运行"文字
 *
 * 【QSS样式】在 applyTheme() 中通过 #chatSidebar、#conversationList 等设置颜色和边框
 */
void ChatPage::setupSidebar()
{
    m_sidebar = new QWidget(this);
    m_sidebar->setFixedWidth(260);              // 【固定宽度】260像素；改大则边栏变宽，改小则变窄
    m_sidebar->setObjectName("chatSidebar");    // QSS定位用

    QVBoxLayout* sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);  // 无内边距
    sidebarLayout->setSpacing(0);                   // 无默认间距

    // 【头部区域】水平布局：标题（左）+ 新建按钮（右）
    QWidget* headerWidget = new QWidget(m_sidebar);
    headerWidget->setObjectName("sidebarHeader");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 12, 12, 12);  // 四边12像素内边距；改大则内容与边缘更远
    headerLayout->setSpacing(8);                       // 标题与按钮之间8像素

    m_sidebarTitle = new QLabel("对话记录", headerWidget);
    m_sidebarTitle->setObjectName("sidebarTitle");
    m_sidebarTitle->setStyleSheet("font-size: 12px; font-weight: 600; letter-spacing: -0.02em;");
    // font-size:12px → 小字标题；letter-spacing:-0.02em → 字间距略微收紧，更紧凑
    headerLayout->addWidget(m_sidebarTitle);

    headerLayout->addStretch();  // 【弹性空间】将标题推到左侧，按钮留在右侧

    // 【新建对话按钮】蓝色小按钮，固定高度28px
    m_newChatButton = new QPushButton("+ 新建", headerWidget);
    m_newChatButton->setObjectName("primaryButton");   // QSS中蓝色背景+白色文字
    m_newChatButton->setFixedHeight(28);               // 固定高度28px；改大则按钮更高
    m_newChatButton->setCursor(Qt::PointingHandCursor);
    connect(m_newChatButton, &QPushButton::clicked, this, &ChatPage::onNewChatClicked);
    headerLayout->addWidget(m_newChatButton);

    sidebarLayout->addWidget(headerWidget);

    // 【分隔线】1像素高的水平线，分隔头部和列表
    QFrame* separator = new QFrame(m_sidebar);
    separator->setFrameShape(QFrame::HLine);   // 水平线形状
    separator->setFixedHeight(1);              // 固定高度1像素；改大则线更粗
    separator->setObjectName("sidebarSeparator");
    sidebarLayout->addWidget(separator);

    // 【对话历史列表】显示所有过往对话
    m_conversationList = new QListWidget(m_sidebar);
    m_conversationList->setObjectName("conversationList");
    m_conversationList->setFrameShape(QFrame::NoFrame);  // 无边框
    m_conversationList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);    // 需要时显示垂直滚动条
    m_conversationList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 永远隐藏水平滚动条
    connect(m_conversationList, &QListWidget::itemClicked, this, &ChatPage::onConversationClicked);
    sidebarLayout->addWidget(m_conversationList, 1);  // 拉伸因子1，占据侧边栏剩余所有垂直空间

    // 【底部状态栏】显示连接状态
    QWidget* statusWidget = new QWidget(m_sidebar);
    statusWidget->setObjectName("statusWidget");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(12, 10, 12, 10);  // 四边留白
    statusLayout->setSpacing(8);                       // 状态点与文字间距

    // 【绿色状态点】8×8像素圆点，表示本地运行中
    QLabel* statusDot = new QLabel(statusWidget);
    statusDot->setFixedSize(8, 8);                     // 8×8像素小圆点；改大则更明显
    statusDot->setObjectName("statusDot");             // QSS中设置为绿色圆形
    statusLayout->addWidget(statusDot);

    m_statusLabel = new QLabel("离线模式 · 本地运行", statusWidget);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setStyleSheet("font-size: 12px;");
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();  // 将状态信息推到左侧

    sidebarLayout->addWidget(statusWidget);
}

/**
 * @brief 构建中间聊天区域
 *
 * 【布局结构】
 * m_chatArea
 * └── chatLayout (QVBoxLayout, 无间距)
 *     ├── m_chatDisplay (QTextBrowser, 拉伸因子1, 占据大部分空间)
 *     └── inputWidget (输入区域容器)
 *         ├── m_messageInput (QTextEdit, 多行输入框)
 *         └── controlsLayout (控制按钮行)
 *             ├── 附件按钮 + 代码按钮（左）
 *             ├── 弹性空间
 *             ├── 发送/停止按钮（右）
 *             └── 模型选择下拉框（最右）
 *
 * 【视觉效果】
 * - 消息显示区：透明背景，无边框，20px内边距
 * - 输入区：顶部1px分隔线，20px左右内边距
 * - 输入框：次级背景色，圆角8px，获取焦点时边框变蓝色
 */
void ChatPage::setupChatArea()
{
    m_chatArea = new QWidget(this);
    m_chatArea->setObjectName("chatArea");  // QSS定位

    QVBoxLayout* chatLayout = new QVBoxLayout(m_chatArea);
    chatLayout->setContentsMargins(0, 0, 0, 0);  // 无内边距
    chatLayout->setSpacing(0);                   // 无默认间距

    // 【消息显示区】只读的富文本浏览器，支持HTML渲染和超链接点击
    m_chatDisplay = new QTextBrowser(m_chatArea);
    m_chatDisplay->setObjectName("chatDisplay");
    m_chatDisplay->setFrameShape(QFrame::NoFrame);      // 无边框
    m_chatDisplay->setOpenExternalLinks(true);          // 允许点击外部链接（用系统浏览器打开）
    m_chatDisplay->setOpenLinks(true);                  // 允许点击链接
    m_chatDisplay->setStyleSheet(
        "QTextBrowser { border: none; background: transparent; padding: 20px; }"
        // 样式说明：
        // - border: none → 无可见边框
        // - background: transparent → 透明背景，显示父控件颜色
        // - padding: 20px → 内容与边缘20像素间距；改大则文字离边缘更远
    );
    chatLayout->addWidget(m_chatDisplay, 1);  // 拉伸因子1，占据聊天区大部分垂直空间

    // 【输入区域容器】固定在聊天区底部
    QWidget* inputWidget = new QWidget(m_chatArea);
    inputWidget->setObjectName("inputWidget");  // QSS中设置顶部边框作为分隔线
    QVBoxLayout* inputLayout = new QVBoxLayout(inputWidget);
    inputLayout->setContentsMargins(20, 12, 20, 12);  // 左右20、上下12像素内边距
    inputLayout->setSpacing(8);                       // 输入框与按钮行之间8像素

    // 【消息输入框】多行文本编辑，支持Shift+Enter换行
    m_messageInput = new QTextEdit(inputWidget);
    m_messageInput->setObjectName("messageInput");
    m_messageInput->setPlaceholderText("输入消息，按 Enter 发送，Shift+Enter 换行...");
    // placeholderText：输入框为空时显示的灰色提示文字
    m_messageInput->setMaximumHeight(120);      // 最大高度120px；输入多行时自动增高到此上限
    m_messageInput->setMinimumHeight(42);       // 最小高度42px；保证至少一行可见
    m_messageInput->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);  // 内容超出时显示垂直滚动条
    chatLayout->addWidget(m_messageInput);      // 将输入框直接添加到聊天布局（在inputWidget上方）

    // 【控制按钮行】水平排列各种操作按钮
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(8);              // 按钮之间8像素间距

    // 【附件按钮】回形针图标，点击选择文件
    m_attachButton = new QPushButton(inputWidget);
    m_attachButton->setObjectName("iconButton");
    m_attachButton->setFixedSize(32, 32);       // 固定32×32像素；改大则按钮更大
    m_attachButton->setIconSize(QSize(16, 16)); // 图标16×16像素；改大则图标更大
    m_attachButton->setCursor(Qt::PointingHandCursor);
    connect(m_attachButton, &QPushButton::clicked, this, &ChatPage::onAttachButtonClicked);
    controlsLayout->addWidget(m_attachButton);

    // 【代码模式按钮】代码图标，切换代码输入模式
    m_codeButton = new QPushButton(inputWidget);
    m_codeButton->setObjectName("iconButton");
    m_codeButton->setFixedSize(32, 32);
    m_codeButton->setIconSize(QSize(16, 16));
    m_codeButton->setCursor(Qt::PointingHandCursor);
    controlsLayout->addWidget(m_codeButton);

    controlsLayout->addStretch();  // 【弹性空间】将左侧按钮和右侧按钮分开

    // 【发送按钮】蓝色圆形图标按钮（纸飞机图标）
    m_sendButton = new QPushButton(inputWidget);
    m_sendButton->setObjectName("primaryButton");
    m_sendButton->setFixedSize(32, 32);         // 32×32像素圆形按钮
    m_sendButton->setIconSize(QSize(18, 18));   // 图标18×18，稍大更醒目
    m_sendButton->setCursor(Qt::PointingHandCursor);
    connect(m_sendButton, &QPushButton::clicked, this, &ChatPage::onSendClicked);
    controlsLayout->addWidget(m_sendButton);

    // 【停止按钮】红色矩形按钮，AI生成时显示，平时隐藏
    m_stopButton = new QPushButton("停止", inputWidget);
    m_stopButton->setObjectName("stopButton");   // QSS中红色背景
    m_stopButton->setFixedHeight(32);            // 高度32px；改大则按钮更高
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setVisible(false);             // 【初始隐藏】平时不显示，发送消息后 showStopButton(true) 显示
    connect(m_stopButton, &QPushButton::clicked, this, &ChatPage::onStopClicked);
    controlsLayout->addWidget(m_stopButton);

    // 【模型选择下拉框】选择当前使用的AI模型
    m_modelSelector = new QComboBox(inputWidget);
    m_modelSelector->setObjectName("modelSelector");
    m_modelSelector->setPlaceholderText("选择模型");  // 未选择时显示的提示文字
    // 模型列表在测试连接后由 MainWindow 动态填充
    m_modelSelector->setStyleSheet(
        "QComboBox { border: 1px solid; border-radius: 8px; padding: 4px 12px; font-size: 12px; }"
        // border: 1px solid → 1像素实线边框；改2px则边框更粗
        // border-radius: 8px → 圆角半径；改0则直角，改16则更圆
        // padding: 4px 12px → 上下4、左右12像素内边距
        "QComboBox::drop-down { border-left: 1px solid; width: 20px; }"
        // drop-down：下拉箭头按钮区域；width:20px控制箭头区宽度
        "QComboBox::down-arrow { image: none; }"
        // down-arrow：隐藏默认下拉箭头图标（使用自定义样式或保留空白）
    );
    controlsLayout->addWidget(m_modelSelector);

    inputLayout->addLayout(controlsLayout);
    chatLayout->addWidget(inputWidget);
}

/**
 * @brief 构建右侧输出/任务面板
 *
 * 【功能说明】使用 QStackedWidget 在三种视图之间切换：
 * - 空状态：显示提示文字"执行日志、步骤进度和文件结果将显示在这里"
 * - 输出面板（OutputPanel）：显示Multi-Agent执行日志和步骤输出
 * - 任务列表面板（TaskListPanel）：显示任务计划和步骤进度
 *
 * 【堆叠控件说明】QStackedWidget 同时管理多个子页面，但只显示其中一个。
 * 通过 setCurrentIndex() 切换显示内容，常用于选项卡、向导等场景。
 */
void ChatPage::setupRightPanel()
{
    m_rightPanel = new QWidget(this);
    m_rightPanel->setObjectName("rightPanel");
    m_rightPanel->setMinimumWidth(280);  // 【最小宽度】280像素；改小则可更窄，改大则至少这么宽

    QVBoxLayout* rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);  // 无内边距
    rightLayout->setSpacing(0);                   // 无间距

    // 【堆叠控件】管理三个子面板
    m_rightStack = new QStackedWidget(m_rightPanel);
    rightLayout->addWidget(m_rightStack);

    // 【空状态页面】初始显示，提示用户此处将显示执行结果
    QWidget* emptyPanel = new QWidget(m_rightPanel);
    QVBoxLayout* emptyLayout = new QVBoxLayout(emptyPanel);
    emptyLayout->setContentsMargins(20, 20, 20, 20);  // 四边20像素内边距
    QLabel* emptyLabel = new QLabel("执行日志、步骤进度和文件结果将显示在这里", emptyPanel);
    emptyLabel->setStyleSheet("color: #94a3b8; font-size: 13px; text-align: center;");
    // color: #94a3b8 → 灰色提示文字；改 #ffffff 则变白色
    emptyLabel->setWordWrap(true);  // 允许自动换行
    emptyLayout->addWidget(emptyLabel, 0, Qt::AlignCenter);  // 居中显示
    m_rightStack->addWidget(emptyPanel);  // 索引0 = 空状态

    // 【输出面板】实际类型为 OutputPanel，显示执行日志
    m_outputPanel = new OutputPanel(m_rightPanel);
    m_rightStack->addWidget(m_outputPanel);  // 索引1 = 输出面板

    // 【任务列表面板】实际类型为 TaskListPanel，显示任务步骤
    m_taskListPanel = new TaskListPanel(m_rightPanel);
    m_rightStack->addWidget(m_taskListPanel);  // 索引2 = 任务列表
}

/**
 * @brief 应用当前主题样式
 *
 * 【QSS样式详解】从ThemeManager获取颜色值，应用到侧边栏、聊天区、右侧面板。
 * 颜色变量含义与 modelconfigpage.cpp 中相同（bg=背景, bg2=次级背景, pri=主色调等）。
 *
 * 【关键样式效果】
 * - #chatSidebar：背景色+右侧1px边框（分隔边栏和聊天区）
 * - #conversationList::item:selected：选中项显示左侧2px蓝色边框+淡蓝背景
 * - #messageInput：输入框背景色+边框+圆角；focus时边框变蓝色
 * - #primaryButton：蓝色背景+白色文字+圆角；hover时颜色稍浅
 * - #stopButton：红色背景(#EF4444)+白色文字；hover时深红色(#DC2626)
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

    // 【侧边栏样式】
    m_sidebar->setStyleSheet(QString(
        // 侧边栏整体：背景色+右侧1px边框（分隔边栏和聊天区）
        "QWidget#chatSidebar { background-color: %1; border-right: 1px solid %4; }"
        // 头部：与侧边栏同背景色
        "QWidget#sidebarHeader { background-color: %1; }"
        // 底部状态栏：同背景色+顶部1px边框
        "QWidget#statusWidget { background-color: %1; border-top: 1px solid %4; }"
        // 标题文字颜色
        "QLabel#sidebarTitle { color: %6; }"
        // 状态标签颜色（第三级文字色，较淡）
        "QLabel#statusLabel { color: %8; }"
        // 状态圆点：绿色圆形（8×8 + 4px圆角 = 正圆）
        "QLabel#statusDot { background-color: #22C55E; border-radius: 4px; }"
        // 对话列表：透明背景+无边框+无焦点虚线框
        "QListWidget#conversationList { background-color: %1; border: none; outline: none; }"
        // 列表项：圆角8px+内边距+灰色文字
        "QListWidget#conversationList::item { border-radius: 8px; padding: 10px 12px; margin: 2px 8px; color: %6; }"
        // 列表项悬停：第三级背景色（略深/略浅）
        "QListWidget#conversationList::item:hover { background-color: %3; }"
        // 列表项选中：主色弱变体背景+左侧2px蓝色边框
        "QListWidget#conversationList::item:selected { background-color: %9; color: %6; border-left: 2px solid %5; }"
        // 分隔线颜色
        "QFrame { border-color: %4; }"
    ).arg(bg2, bg, bg3, bdr, pri, txtPri, txtSec, txtTer, priSub));
    // arg顺序：%1=bg2, %2=bg, %3=bg3, %4=bdr, %5=pri, %6=txtPri, %7=txtSec, %8=txtTer, %9=priSub

    // 【聊天区样式】
    m_chatArea->setStyleSheet(QString(
        // 聊天区背景
        "QWidget#chatArea { background-color: %1; }"
        // 输入区容器：顶部1px边框（分隔消息区和输入区）
        "QWidget#inputWidget { background-color: %1; border-top: 1px solid %4; }"
        // 消息输入框：次级背景+主文字色+边框+圆角
        "QTextEdit#messageInput { background-color: %2; color: %6; border: 1px solid %4; border-radius: 8px; padding: 8px 14px; font-size: 14px; }"
        // 输入框聚焦：边框变蓝色
        "QTextEdit#messageInput:focus { border-color: %5; }"
        // 发送按钮：蓝色背景+白色文字
        "QPushButton#primaryButton { background-color: %5; color: white; border-radius: 8px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton#primaryButton:hover { background-color: %7; }"
        // 停止按钮：红色背景+白色文字（固定颜色，不随主题变化，确保醒目）
        "QPushButton#stopButton { background-color: #EF4444; color: white; border-radius: 8px; padding: 6px 16px; font-weight: 500; }"
        "QPushButton#stopButton:hover { background-color: #DC2626; }"
        // 图标按钮：透明背景+第三级文字色；悬停显示第三级背景色
        "QPushButton#iconButton { background-color: transparent; color: %8; border-radius: 8px; font-size: 13px; }"
        "QPushButton#iconButton:hover { background-color: %3; }"
        // 模型选择器：文字色+边框色+背景色
        "QComboBox#modelSelector { color: %7; border-color: %4; background-color: %2; }"
    ).arg(bg, bg2, bg3, bdr, pri, txtPri, pri, txtSec, txtTer));

    // 【右侧面板样式】
    m_rightPanel->setStyleSheet(QString(
        // 背景色+左侧1px边框（分隔聊天区和右侧面板）
        "QWidget#rightPanel { background-color: %1; border-left: 1px solid %2; }"
    ).arg(bg, bdr));

    // 【设置图标】使用 IconHelper 绘制矢量图标，颜色随主题变化
    QColor iconColor = ThemeManager::instance()->textSecondary();  // 次要文字色
    m_attachButton->setIcon(QIcon(IconHelper::attach(18, iconColor)));  // 回形针图标，18px wjh 0710
    m_codeButton->setIcon(QIcon(IconHelper::code(18, iconColor)));      // 代码图标，18px
    m_sendButton->setIcon(QIcon(IconHelper::send(18, QColor("#FFFFFF")))); // 发送图标，18px白色
}

/**
 * @brief 刷新对话历史列表显示
 *
 * 【功能说明】根据 m_conversations 内存数据，重新构建侧边栏列表。
 * 每个对话项显示：标题（13px加粗/中等）、最后一条消息预览（11px灰色）、时间戳（11px灰色）。
 * 当前活跃对话显示左侧3px蓝色边框+淡蓝背景。
 *
 * 【视觉效果参数】
 * - item->setSizeHint(QSize(width, 80))：每项高度80像素；改大则每项更高
 * - container->setStyleSheet("border-left: 3px solid xxx")：左侧边框3像素；改大则更粗
 * - titleLabel：活跃对话600(加粗)，非活跃500(中等)
 */
void ChatPage::updateConversationList()
{
    m_conversationList->clear();  // 清空现有列表

    ThemeManager* tm = ThemeManager::instance();
    QString txtPri = tm->textPrimary().name();
    QString txtTer = tm->textTertiary().name();
    QString pri = tm->primary().name();
    QString priSub = tm->primarySubtle().name();
    QString danger = "#EF4444";  // 删除按钮的红色（固定值，确保醒目）

    // 遍历所有对话，为每个对话创建列表项
    for (int i = 0; i < m_conversations.size(); ++i) {
        const auto& conv = m_conversations[i];

        QListWidgetItem* item = new QListWidgetItem(m_conversationList);
        item->setData(Qt::UserRole, i);  // 存储索引，点击时识别是哪个对话
        item->setSizeHint(QSize(m_conversationList->width(), 100));  // 每项固定高度80px

        // 【容器控件】包含按钮和删除按钮
        QWidget* container = new QWidget(m_conversationList);
        container->setObjectName(conv.isActive ? "convActive" : "convItem");
        container->setCursor(Qt::PointingHandCursor);  // 鼠标悬停变手型

        QHBoxLayout* containerLayout = new QHBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);  // 无内边距
        containerLayout->setSpacing(10);

        // 【对话按钮】占据主要区域，显示标题、预览、时间
        QPushButton* btn = new QPushButton(container);
        btn->setObjectName("convButton");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(false);
        btn->setFlat(true);  // 扁平样式，无默认按钮边框和背景

        QVBoxLayout* layout = new QVBoxLayout(btn);
        layout->setContentsMargins(3, 1, 3, 1);  // 按钮内部边距
        layout->setSpacing(1);                     // 标题、预览、时间之间2像素间距

        // 【标题标签】
        QLabel* titleLabel = new QLabel(conv.title, btn);
        titleLabel->setStyleSheet(QString("font-size: 13px; font-weight: %1; color: %2;")
            .arg(conv.isActive ? "600" : "500", txtPri));
        // 活跃对话用600(加粗)，非活跃用500(中等)；改700则更粗
        titleLabel->setAlignment(Qt::AlignLeft);
        layout->addWidget(titleLabel);


        // 【时间戳】
        QLabel* timeLabel = new QLabel(conv.timestamp, btn);
        timeLabel->setStyleSheet(QString("font-size: 11px; color: %1;").arg(txtTer));
        timeLabel->setAlignment(Qt::AlignLeft);
        layout->addWidget(timeLabel);

        containerLayout->addWidget(btn, 1);  // 拉伸因子1，占据大部分宽度

        // 【删除按钮】红色叉号图标，点击删除该对话
        QPushButton* deleteBtn = new QPushButton(container);
        deleteBtn->setObjectName("deleteButton");
        deleteBtn->setFixedSize(28, 28);  // 28×28像素小按钮；改大则更大
        deleteBtn->setCursor(Qt::PointingHandCursor);
        deleteBtn->setIcon(QIcon(IconHelper::stop(12, QColor(danger))));  // 12px红色停止图标
        deleteBtn->setStyleSheet(QString(
            "QPushButton#deleteButton { background-color: transparent; border-radius: 6px; }"
            "QPushButton#deleteButton:hover { background-color: rgba(239, 68, 68, 0.15); }"
            // hover时淡红半透明背景；rgba(239,68,68,0.15) = 15%透明度红色
        ));
        // 【lambda连接】点击删除按钮时发射 deleteConversation 信号
        connect(deleteBtn, &QPushButton::clicked, this, [this, i](bool) {
            Q_UNUSED(i);
            emit deleteConversation(i);  // 通知主窗口删除第i个对话
        });

        containerLayout->addWidget(deleteBtn);

        // 【容器背景样式】活跃对话有蓝色左边框+淡蓝背景，非活跃透明
        QString containerStyle = conv.isActive ?
            QString("background-color: %1; border-left: 3px solid %2; border-radius: 8px;")
                .arg(priSub, pri) :
            QString("background-color: transparent; border-radius: 8px;");
        // border-left: 3px solid %2 → 左侧3像素蓝色竖条；改大如5px则更粗
        container->setStyleSheet(containerStyle);

        // 【点击对话项】切换当前对话
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            onConversationClicked(nullptr);
            m_currentConversationIndex = i;
            for (int j = 0; j < m_conversations.size(); ++j) {
                m_conversations[j].isActive = (j == i);  // 设置活跃标记
            }
            updateConversationList();  // 刷新列表显示高亮变化
        });

        m_conversationList->addItem(item);
        m_conversationList->setItemWidget(item, container);  // 将自定义控件设为列表项显示内容
    }
}

/**
 * @brief 添加一条聊天消息
 * @param sender 发送者名称（"用户"或"AI"）
 * @param content 消息内容（纯文本）
 * @param isUser true=用户消息, false=AI消息
 * @param saveToConversation true=保存到对话历史
 *
 * 【消息气泡HTML结构】
 * 使用100%宽度表格，用户消息<td align='right'>，AI消息<td align='left'>。
 * 内部div设置：
 * - display:inline-block：让气泡宽度随内容自适应，而不是填满整行
 * - border-radius：16px大圆角，其中一个角2px形成"聊天气泡尖角"
 * - padding：10px 14px，文字与气泡边缘的间距
 * - max-width:75%：气泡最大占聊天区宽度的75%，避免过长
 */
void ChatPage::addMessage(const QString& sender, const QString& content, bool isUser, bool saveToConversation)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");  // 当前时间，如 "14:32"
    ThemeManager* tm = ThemeManager::instance();

    // 【内容转换】用户消息直接HTML转义；AI消息将Markdown转为HTML
    QString formattedContent = isUser ? content.toHtmlEscaped() : markdownToHtml(content);

    QString messageHtml;
    if (isUser) {
        // 【用户消息气泡】蓝色背景(#4F46E5)，白色文字，右侧对齐，左下角2px尖角
        QString userBg = "#4F46E5";   // 靛蓝色背景；改 #EF4444 则变红色，改 #22C55E 则变绿色
        QString userText = "#FFFFFF";   // 白色文字；改 #000000 则变黑色
        messageHtml = QString(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin:8px 0;'><tr>"
            "<td align='right'>"
            "<div style='display:inline-block;background:%1;border-radius:16px 16px 2px 16px;padding:10px 14px;max-width:75%%;text-align:left;'>"
            // border-radius:16px 16px 2px 16px → 左上、右上、右下16px圆角，左下2px尖角
            // max-width:75%% → 最大宽度75%（%%转义为%）；改85%%则更宽
            "<div style='font-size:11px;color:rgba(255,255,255,0.7);margin-bottom:3px;'>%2 %3</div>"
            // rgba(255,255,255,0.7) → 70%透明度白色小字（发送者和时间）
            "<div style='font-size:13px;color:%4;line-height:1.5;'>%5</div>"
            // font-size:13px → 消息正文13像素；改15px则更大
            // line-height:1.5 → 行高1.5倍；改1.8则更宽松
            "</div>"
            "</td></tr></table>"
        ).arg(userBg, sender, time, userText, formattedContent);
    } else {
        // 【AI消息气泡】灰色背景（主题色），深色文字，左侧对齐，右下角2px尖角
        QString aiBg = tm->bgSecondary().name();    // 次级背景色，如 #1E293B
        QString aiText = tm->textPrimary().name();  // 主文字色，如 #F1F5F9
        QString aiNameColor = "#6366f1";            // AI名称颜色（靛蓝色）；改 #EC4899 则变粉色
        messageHtml = QString(
            "<table width='100%' cellpadding='0' cellspacing='0' style='margin:8px 0;'><tr>"
            "<td align='left'>"
            "<div style='display:inline-block;background:%1;border-radius:16px 16px 16px 2px;padding:10px 14px;max-width:75%%;text-align:left;'>"
            // border-radius:16px 16px 16px 2px → 左上、右上、左下16px圆角，右下2px尖角
            "<div style='font-size:11px;color:%2;margin-bottom:3px;'>%3 %4</div>"
            "<div style='font-size:13px;color:%5;line-height:1.5;'>%6</div>"
            "</div>"
            "</td></tr></table>"
        ).arg(aiBg, aiNameColor, sender, time, aiText, formattedContent);
    }

    m_chatDisplay->append(messageHtml);  // 追加HTML到富文本浏览器

    // 【自动滚动到底部】确保最新消息可见
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());

    // 【保存到对话历史】
    if (saveToConversation && m_currentConversationIndex >= 0 && m_currentConversationIndex < m_conversations.size()) {
        if (isUser) {
            m_conversations[m_currentConversationIndex].messages.append("用户: " + content);
        } else {
            m_conversations[m_currentConversationIndex].messages.append("AI: " + content);
        }
    }
}

/**
 * @brief 追加消息片段（流式输出）
 * @param chunk 新收到的文本片段
 *
 * 【处理流程】
 * 1. 将片段追加到 m_currentAiMessage（完整消息缓存）
 * 2. 设置打字机待显示文本为完整缓存内容
 * 3. 如果定时器未启动，启动打字机效果
 */
void ChatPage::appendMessageChunk(const QString& chunk)
{
    m_currentAiMessage += chunk;      // 累积到完整消息缓存

    m_pendingText = m_currentAiMessage;  // 设置打字机目标文本
    m_pendingIndex = 0;                  // 从第0个字符开始显示

    if (!m_typingTimer->isActive()) {
        m_typingTimer->start();         // 启动打字机定时器（每30ms显示5个字符）
    }
}

/**
 * @brief 打字机定时器回调
 *
 * 【实现逻辑】
 * 每次定时器触发（30ms）：
 * 1. 检查是否已显示完所有文本，是则停止定时器
 * 2. 每次取最多5个未显示字符
 * 3. 更新聊天显示区的HTML内容
 * 4. 自动滚动到底部
 *
 * 【HTML更新策略】
 * - 如果聊天区中不存在 id='currentAiContent' 的div，创建一个新的AI消息气泡
 * - 如果已存在，找到该div并替换其内部HTML为当前完整文本（实现"逐字追加"效果）
 */
void ChatPage::onTypingTimer()
{
    if (m_pendingIndex >= m_pendingText.length()) {
        m_typingTimer->stop();  // 全部显示完毕，停止定时器
        return;
    }

    // 每次显示5个字符（或剩余全部）；改大如10则打字更快，改1则逐字显示
    int chunkSize = qMin(5, m_pendingText.length() - m_pendingIndex);
    QString chunk = m_pendingText.mid(m_pendingIndex, chunkSize);
    m_pendingIndex += chunkSize;  // 移动已显示位置

    QString time = QDateTime::currentDateTime().toString("HH:mm");
    ThemeManager* tm = ThemeManager::instance();

    QString aiBg = tm->bgSecondary().name();
    QString aiText = tm->textPrimary().name();
    QString aiNameColor = "#6366f1";  // AI名称颜色

    QString currentContent = m_chatDisplay->toHtml();
    QString searchMarker = "id='currentAiContent'";
    int idIndex = currentContent.lastIndexOf(searchMarker);

    if (idIndex == -1) {
        // 【首次显示】创建新的AI消息气泡，标记 id='currentAiContent'
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
        // 【更新已有气泡】找到id所在div，替换其内容
        int divStart = currentContent.lastIndexOf(">", idIndex);
        if (divStart != -1) {
            int divEnd = currentContent.indexOf("</div>", divStart + 1);
            if (divEnd != -1) {
                QString before = currentContent.left(divStart + 1);
                QString after = currentContent.mid(divEnd);
                QString newContent = before + markdownToHtml(m_currentAiMessage) + after;
                m_chatDisplay->setHtml(newContent);  // 替换整个HTML内容
            }
        }
    }

    // 自动滚动到底部
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @brief 结束当前AI消息
 *
 * 【处理流程】
 * 1. 停止打字机定时器
 * 2. 将完整AI消息保存到当前对话历史
 * 3. 清空临时缓存和待显示文本
 */
void ChatPage::finishCurrentMessage()
{
    m_typingTimer->stop();  // 停止打字效果

    if (!m_currentAiMessage.isEmpty() && m_currentConversationIndex >= 0 && m_currentConversationIndex < m_conversations.size()) {
        m_conversations[m_currentConversationIndex].messages.append("AI: " + m_currentAiMessage);
    }

    m_currentAiMessage.clear();
    m_pendingText.clear();
    m_pendingIndex = 0;
}

/**
 * @brief 添加文件卡片到聊天区
 * @param name 文件名
 * @param path 文件路径
 * @param isDir 是否为文件夹
 * @param size 文件大小（字节）
 * @param modified 修改时间
 *
 * 【视觉效果】水平卡片，左侧文件图标（📁/📄），中间文件名和路径，右侧"打开"按钮。
 * 背景为次级背景色，带1px边框和10px圆角。
 */
void ChatPage::addFileCard(const QString& name, const QString& path, bool isDir, qint64 size, const QString& modified)
{
    ThemeManager* tm = ThemeManager::instance();
    QString bg = tm->bgSecondary().name();
    QString bdr = tm->border().name();
    QString pri = tm->primary().name();
    QString txtPri = tm->textPrimary().name();
    QString txtTer = tm->textTertiary().name();

    QString icon = isDir ? "📁" : "📄";  // 文件夹或文件图标
    QString sizeStr = isDir ? "文件夹" : QString("%1 KB").arg(size / 1024);

    // 【HTML卡片】使用flex布局
    QString html = QString(
        "<div style='display: flex; align-items: center; gap: 12px; margin: 8px 0; padding: 10px 14px; background: %1; border: 1px solid %2; border-radius: 10px;'>"
        // gap: 12px → 子元素之间12像素间距；改大则图标与文字更远
        // padding: 10px 14px → 上下10、左右14像素内边距
        // border-radius: 10px → 10像素圆角；改0则直角
        "<span style='font-size: 18px;'>%3</span>"
        // 图标18px；改大则更大
        "<div style='flex: 1; min-width: 0;'>"
        "<div style='font-size: 13px; color: %4; font-weight: 500;'>%5</div>"
        "<div style='font-size: 11px; color: %6; margin-top: 2px;'>%7 · %8</div>"
        // margin-top: 2px → 文件名与路径之间2像素间距
        "</div>"
        "<a href='file:///%9' style='color: %10; font-size: 12px; padding: 4px 12px; border: 1px solid %10; border-radius: 6px; text-decoration: none;' onclick=\"QDesktopServices::openUrl(QUrl('file:///%9'))\">打开</a>"
        // 打开按钮：主色文字+边框，6px圆角；hover时（由QTextBrowser处理）可点击打开文件
        "</div>"
    ).arg(bg, bdr, icon, txtPri, name.toHtmlEscaped(), txtTer, path.toHtmlEscaped(), sizeStr, path, pri);

    m_chatDisplay->append(html);

    // 自动滚动到底部
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @brief 追加输出文本
 * @param text HTML文本
 */
void ChatPage::appendOutput(const QString& text)
{
    m_chatDisplay->append(text);
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @brief 追加日志
 * @param html HTML格式日志
 */
void ChatPage::appendLog(const QString& html)
{
    m_chatDisplay->append(html);
    QScrollBar* sb = m_chatDisplay->verticalScrollBar();
    sb->setValue(sb->maximum());
}

/**
 * @brief 追加步骤头部
 * @param stepId 步骤编号
 * @param description 步骤描述
 *
 * 【视觉效果】淡蓝色/淡紫色背景卡片，带边框和圆角，显示"▶ 步骤 X — 描述"和时间戳。
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
    // priSub（淡蓝背景）+ pri（蓝色边框和标题）形成协调的强调色卡片

    appendOutput(html);
}

/**
 * @brief 追加步骤输出
 * @param stepId 步骤编号
 * @param output 输出文本
 *
 * 【视觉效果】等宽字体（Consolas）显示，12px灰色文字，适合显示命令行输出和代码。
 */
void ChatPage::appendStepOutput(int stepId, const QString& output)
{
    Q_UNUSED(stepId)
    ThemeManager* tm = ThemeManager::instance();
    QString txtSec = tm->textSecondary().name();

    QString formatted = output.toHtmlEscaped();  // 转义特殊字符
    formatted.replace("\n", "<br>");             // 换行转HTML换行

    QString html = QString(
        "<div style='padding: 4px 12px; color: %1; font-family: Consolas, monospace; font-size: 12px;'>%2</div>"
        // font-family: Consolas, monospace → 等宽字体，适合代码和日志
    ).arg(txtSec, formatted);

    appendOutput(html);
}

/**
 * @brief 追加步骤结果
 * @param stepId 步骤编号
 * @param success true=成功, false=失败
 * @param message 结果描述
 *
 * 【视觉效果】
 * - 成功：淡绿背景+绿色边框，显示"✓ 成功"
 * - 失败：淡红背景+红色边框，显示"✗ 失败"
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
    // margin-left: 8px → 状态图标与描述之间8像素间距

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
 * @brief 清空输出面板
 */
void ChatPage::clearOutput()
{
    clearChat();  // 当前实现中输出也显示在聊天区
}

/**
 * @brief 设置当前计划
 * @param plan 任务计划
 */
void ChatPage::setPlan(const TaskPlan& plan)
{
    m_currentPlan = plan;
}

/**
 * @brief 更新步骤状态
 * @param stepId 步骤编号
 * @param status 新状态
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
 * @brief 清除当前计划
 */
void ChatPage::clearPlan()
{
    m_currentPlan = TaskPlan();  // 重置为空计划
}

/**
 * @brief 设置输入框可用状态
 * @param enabled true=可用, false=禁用
 */
void ChatPage::setInputEnabled(bool enabled)
{
    m_messageInput->setEnabled(enabled);   // 输入框变灰/正常
    m_sendButton->setEnabled(enabled);     // 发送按钮变灰/正常
}

/**
 * @brief 显示/隐藏停止按钮
 * @param show true=显示停止按钮并隐藏发送按钮, false=反之
 */
void ChatPage::showStopButton(bool show)
{
    m_stopButton->setVisible(show);     // 停止按钮显示/隐藏
    m_sendButton->setVisible(!show);    // 发送按钮相反状态
}

/**
 * @brief 发送按钮点击处理
 *
 * 【流程】
 * 1. 获取输入框文本并去除首尾空白
 * 2. 若非空，发射 sendMessage 信号通知主窗口发送消息
 * 3. 清空输入框
 */
void ChatPage::onSendClicked()
{
    QString message = m_messageInput->toPlainText().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);    // 发射信号，携带消息内容
        m_messageInput->clear();      // 清空输入框，准备下一条输入
    }
}

/**
 * @brief 停止按钮点击处理
 */
void ChatPage::onStopClicked()
{
    emit stopClicked();  // 通知主窗口中断AI生成
}

/**
 * @brief 同步当前对话信息
 *
 * 【功能】将当前聊天内容同步到对话列表的标题和时间戳。
 * 取第一条用户消息的前20字作为标题，更新当前时间。
 */
void ChatPage::syncCurrentConversation()
{
    if (m_currentConversationIndex < 0 || m_currentConversationIndex >= m_conversations.size()) {
        return;
    }

    if (!m_conversations[m_currentConversationIndex].messages.isEmpty()) {
        QString firstMsg = m_conversations[m_currentConversationIndex].messages.first();
        if (firstMsg.startsWith("用户: ")) {
            QString title = firstMsg.mid(4).left(20);  // 去掉"用户: "前缀，取前20字
            if (firstMsg.mid(4).length() > 20) title += "...";
            m_conversations[m_currentConversationIndex].title = title;
        }
    }
    m_conversations[m_currentConversationIndex].timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
}

/**
 * @brief 新建对话按钮点击处理
 */
void ChatPage::onNewChatClicked()
{
    syncCurrentConversation();   // 先保存当前对话
    clearChat();                 // 清空聊天区
    emit newChatClicked();       // 通知主窗口创建新对话
}

/**
 * @brief 对话历史点击处理
 * @param item 被点击的列表项
 */
void ChatPage::onConversationClicked(QListWidgetItem* item)
{
    if (!item) return;
    int index = item->data(Qt::UserRole).toInt();  // 获取存储的对话索引

    syncCurrentConversation();   // 保存当前对话

    for (int i = 0; i < m_conversations.size(); ++i) {
        m_conversations[i].isActive = (i == index);  // 更新活跃标记
    }
    m_currentConversationIndex = index;
    updateConversationList();    // 刷新列表高亮
    emit conversationSelected(index);  // 通知主窗口切换对话
}

/**
 * @brief 主题变化响应
 */
void ChatPage::onThemeChanged()
{
    applyTheme();               // 重新应用颜色样式
    updateConversationList();   // 刷新列表（因为颜色变化了）
}

/**
 * @brief 自动保存定时器回调
 *
 * 【功能】每3秒自动调用一次，同步当前对话标题和时间，刷新侧边栏显示。
 * 这样即使不切换对话，侧边栏也会实时更新最后消息预览和时间。
 */
void ChatPage::onAutoSaveTimer()
{
    syncCurrentConversation();
    updateConversationList();
}

/**
 * @brief 加载对话列表
 * @param conversations JSON格式对话数据
 *
 * 【JSON结构】
 * [
 *   {
 *     "title": "对话标题",
 *     "createdAt": "2024-01-01 12:00:00",
 *     "isActive": true,
 *     "messages": [
 *       {"role": "user", "content": "你好"},
 *       {"role": "assistant", "content": "你好！有什么可以帮你的？"}
 *     ]
 *   }
 * ]
 */
void ChatPage::loadConversations(const QJsonArray& conversations)
{
    m_conversations.clear();

    for (const auto& item : conversations) {
        QJsonObject obj = item.toObject();
        Conversation conv;
        conv.title = obj["title"].toString("新对话");    // 默认标题"新对话"
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

    updateConversationList();  // 刷新侧边栏显示
}

/**
 * @brief 加载指定对话的消息
 * @param index 对话索引
 */
void ChatPage::loadConversationMessages(int index)
{
    if (index < 0 || index >= m_conversations.size()) {
        return;
    }

    clearChat();                 // 清空聊天区
    m_currentConversationIndex = index;

    const auto& conv = m_conversations[index];
    for (const QString& msg : conv.messages) {
        if (msg.startsWith("用户: ")) {
            addMessage("用户", msg.mid(4), true, false);   // false=不重复保存到历史
        } else if (msg.startsWith("AI: ")) {
            addMessage("AI", msg.mid(3), false, false);
        }
    }
}

/**
 * @brief 附件按钮点击处理
 *
 * 【流程】
 * 1. 弹出多文件选择对话框，支持多种文件类型过滤
 * 2. 用户选择文件后，将文件信息添加到聊天区显示
 * 3. 发射 filesSelected 信号通知主窗口处理文件上传
 */
void ChatPage::onAttachButtonClicked()
{
    // 【文件过滤器】按类型分组，方便用户快速筛选
    QStringList filters;
    filters << "所有文件 (*.*)";
    filters << "文本文件 (*.txt *.md *.json *.xml *.csv)";
    filters << "代码文件 (*.cpp *.h *.hpp *.py *.js *.ts *.java *.go)";
    filters << "配置文件 (*.ini *.cfg *.yaml *.yml *.toml)";
    filters << "文档文件 (*.doc *.docx *.pdf *.rtf)";
    filters << "表格文件 (*.xls *.xlsx *.ods)";
    filters << "图像文件 (*.png *.jpg *.jpeg *.gif *.bmp *.svg)";
    filters << "日志文件 (*.log)";
    filters << "数据文件 (*.sql *.db)";
    filters << "Markdown文件 (*.md)";

    // 【打开文件对话框】多选模式，默认在用户主目录
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "选择文件",                    // 对话框标题
        QDir::homePath(),              // 默认打开用户主目录；可改为上次目录
        filters.join(";;")             // 用";;"连接各过滤器
    );

    if (!files.isEmpty()) {
        m_selectedFiles.append(files);  // 保存到成员变量

        // 为每个文件添加信息卡片到聊天区
        for (const QString& filePath : files) {
            QFileInfo info(filePath);
            QString sizeStr;
            if (info.size() < 1024) {
                sizeStr = QString("%1 B").arg(info.size());
            } else if (info.size() < 1024 * 1024) {
                sizeStr = QString("%1 KB").arg(info.size() / 1024);
            } else {
                sizeStr = QString("%1 MB").arg(static_cast<double>(info.size()) / (1024 * 1024), 0, 'f', 2);
            }

            // 构建文件信息消息
            QString message = QString(
                "用户上传了文件: %1\n"
                "路径: %2\n"
                "大小: %3\n"
                "最后修改: %4"
            ).arg(info.fileName())
             .arg(filePath)
             .arg(sizeStr)
             .arg(info.lastModified().toString("yyyy-MM-dd HH:mm:ss"));

            addMessage("用户", message, true, false);  // 以用户消息形式显示文件信息
        }

        emit filesSelected(files);  // 通知主窗口处理文件
    }
}

/**
 * @brief 移除已选文件
 */
void ChatPage::onRemoveFileClicked()
{
    m_selectedFiles.clear();
}

/**
 * @brief 获取已选文件列表
 * @return 文件路径列表
 */
QStringList ChatPage::selectedFiles() const
{
    return m_selectedFiles;
}

/**
 * @brief 清空已选文件列表
 */
void ChatPage::clearSelectedFiles()
{
    m_selectedFiles.clear();
}
