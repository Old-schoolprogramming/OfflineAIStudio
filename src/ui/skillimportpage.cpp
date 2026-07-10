/**
 * @file skillimportpage.cpp
 * @brief 技能导入页面实现 —— 技能包上传导入与已安装技能管理
 *
 * 【整体说明】
 * SkillImportPage 是一个完整的技能管理页面，采用滚动布局设计，包含：
 * 1. 顶部拖拽上传区域（setupUploadZone）
 * 2. 路径输入行（手动输入技能配置文件路径并导入）
 * 3. "已安装的技能"标题+分隔线
 * 4. 技能卡片网格（setupSkillGrid，两列排列）
 * 5. 底部信息栏（技能统计数量+外部链接）
 *
 * 页面颜色通过ThemeManager动态获取，支持暗色/亮色主题切换。
 */

#include "skillimportpage.h"   // 【引入头文件】包含SkillImportPage类的声明，使本文件知道有哪些成员和方法需要实现
#include "thememanager.h"      // 【引入主题管理器】使用ThemeManager::instance()获取单例，以及background()/primary()等颜色方法
#include "iconhelper.h"        // 【引入图标辅助类】使用IconHelper::cloudUpload()获取48×48的云上传SVG图标
#include <QFileDialog>         // 【引入文件对话框类】用于弹出文件选择窗口；用户可选择.zip/.json/.yaml/.yml格式的技能配置文件
#include <QFrame>              // 【引入框架类】QFrame是QLabel和QWidget的基类，提供边框、阴影等样式支持；用于绘制分隔线（QFrame::HLine）

/**
 * @brief 构造函数
 * @param parent 父QWidget
 *
 * 【初始化流程详解】
 * 1. 调用setupUI()：创建滚动区域、内容面板、上传区、路径输入行、技能网格、底部信息栏等所有UI控件
 * 2. 调用applyTheme()：从ThemeManager获取当前主题颜色值，生成QSS样式表并应用到页面
 * 3. 连接ThemeManager::themeChanged信号到onThemeChanged槽函数：当用户切换主题时自动刷新样式
 *
 * 【对象树关系】传入parent建立Qt父子对象树，确保本页面随父窗口销毁而自动释放
 */
SkillImportPage::SkillImportPage(QWidget *parent)
    : QWidget(parent)  // 【初始化列表】调用父类QWidget构造函数，传入parent建立Qt对象树关系
{
    setupUI();  // 【初始化UI】创建并排列所有界面控件；详细过程见setupUI()的注释
    applyTheme();  // 【应用主题】从ThemeManager获取颜色值并生成QSS样式表；详细过程见applyTheme()的注释

    // 【连接主题变化信号】
    // 发送者：ThemeManager::instance()单例对象
    // 信号：&ThemeManager::themeChanged（当用户切换暗色/亮色主题时发射）
    // 接收者：this（当前SkillImportPage对象）
    // 槽：&SkillImportPage::onThemeChanged（重新调用applyTheme()刷新样式）
    // 解耦优势：SkillImportPage无需主动轮询主题变化，由ThemeManager统一通知所有关注者
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &SkillImportPage::onThemeChanged);
}

/**
 * @brief 析构函数
 *
 * 【Qt自动内存管理】SkillImportPage(this)作为父对象，其所有子对象（m_scrollArea、m_contentWidget、m_uploadZone等）
 * 会自动递归销毁。因此本析构函数无需手动delete任何子控件，保持为空即可。
 */
SkillImportPage::~SkillImportPage()
{
    // 此处无需编写任何代码；Qt对象树会自动释放所有子控件
}

/**
 * @brief 初始化整体界面布局
 *
 * 【布局层次结构】
 * SkillImportPage (this)
 * └── mainLayout (QVBoxLayout, 0边距0间距) —— 仅包裹滚动区域
 *     └── m_scrollArea (QScrollArea) —— 负责滚动，无边框，隐藏水平滚动条
 *         └── m_contentWidget (QWidget) —— 真实内容容器，四边32px内边距
 *             └── contentLayout (QVBoxLayout, 32px内边距, 0间距)
 *                 ├── m_uploadZone (拖拽上传区)
 *                 ├── pathLayout (路径输入行: 标签+输入框+导入按钮)
 *                 ├── dividerLayout (分隔线: 左横线+标题+右横线)
 *                 ├── m_skillGrid (QGridLayout, 技能卡片网格, 每行2列)
 *                 └── bottomLayout (底部信息栏: 统计+链接按钮)
 *
 * 【关键设计决策】
 * - 使用QScrollArea包裹内容面板，确保内容超出窗口高度时可上下滚动
 * - contentLayout->setSpacing(0)由各个子布局自行控制间距，避免间距叠加
 * - contentLayout->addStretch()在底部添加弹性空间，确保网格不会过度拉伸
 */
void SkillImportPage::setupUI()
{
    // 【创建页面主布局】QVBoxLayout使子控件垂直堆叠排列；参数this表示此布局绑定到当前SkillImportPage面板
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 【设置主布局四周边距为0】setContentsMargins(左, 上, 右, 下) = (0, 0, 0, 0)
    // 视觉效果：滚动区域直接贴到SkillImportPage的边缘，没有额外空白
    // 若改为setContentsMargins(8, 8, 8, 8)，则页面四周会出现8像素的灰色边框（如果父容器有背景色）
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 【设置主布局控件间距为0】setSpacing(0)表示滚动区域与页面边缘之间没有额外间距
    // 由于主布局中只有m_scrollArea一个控件，此设置实际上无显著视觉效果
    mainLayout->setSpacing(0);

    // 【创建滚动区域】QScrollArea为内部内容提供垂直滚动条支持
    // 参数this表示父窗口是SkillImportPage，建立正确的父子对象树关系
    m_scrollArea = new QScrollArea(this);

    // 【设置滚动区域自动调整大小】setWidgetResizable(true)表示当内容面板大小变化时，
    // QScrollArea会自动调整滚动范围和滚动条位置，确保内容始终可见
    m_scrollArea->setWidgetResizable(true);

    // 【设置滚动区域无边框】setFrameShape(QFrame::NoFrame)移除QScrollArea默认的3D边框
    // 视觉效果：滚动区域与内容面板之间没有边框线条，外观更加简洁现代
    // 若改为QFrame::StyledPanel，则会显示系统风格的边框
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    // 【隐藏水平滚动条】setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff)确保水平方向永不显示滚动条
    // 视觉效果：页面内容在水平方向上必须自适应窗口宽度，不允许水平滚动
    // 若改为Qt::ScrollBarAsNeeded，则当内容宽度超出窗口时会显示水平滚动条（不符合设计意图）
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 【创建滚动内容面板】QWidget是承载所有页面内容的实际容器
    // 参数m_scrollArea表示父窗口是滚动区域，使内容面板随滚动区域一起管理
    m_contentWidget = new QWidget(m_scrollArea);

    // 【设置内容面板对象名称】setObjectName("skillContent")用于在QSS样式表中通过#skillContent精确定位此面板
    // 例如："QWidget#skillContent { background-color: #0f172a; }" 设置暗色主题背景
    // 若未设置对象名，则QSS只能通过类型选择器QWidget设置样式，影响所有QWidget
    m_contentWidget->setObjectName("skillContent");

    // 【创建内容布局】QVBoxLayout使子控件垂直堆叠排列；参数m_contentWidget表示此布局绑定到内容面板
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);

    // 【设置内容布局四周边距】setContentsMargins(左, 上, 右, 下) = (32, 32, 32, 32)
    // 视觉效果：内容面板内的所有控件与面板边框之间保持32像素的留白内边距，营造宽松现代的呼吸感
    // 若改为(16, 16, 16, 16)，则留白减半，内容更紧凑
    // 若改为(0, 0, 0, 0)，则所有控件直接贴到滚动条边缘，视觉压抑
    contentLayout->setContentsMargins(32, 32, 32, 32);

    // 【设置内容布局控件间距为0】contentLayout本身不控制子控件间距，由各个子布局自行管理
    // 好处：uploadZone、pathLayout、dividerLayout等各自控制与相邻元素的间距，避免间距叠加导致过大
    contentLayout->setSpacing(0);

    // 【调用setupUploadZone()创建拖拽上传区域】
    // 详细过程见setupUploadZone()的注释；会设置m_uploadZone成员变量
    setupUploadZone();

    // 【调用setupSkillGrid()创建技能卡片网格】
    // 详细过程见setupSkillGrid()的注释；会设置m_skillGrid成员变量，并预置4个示例技能卡片
    setupSkillGrid();

    // 【将上传区域加入内容布局】上传区位于页面最上方
    contentLayout->addWidget(m_uploadZone);

    // ============================================
    // 【路径输入行】标签 + 输入框 + 导入按钮
    // ============================================

    // 【创建水平布局】QHBoxLayout使子控件水平排列；用于标签、输入框、按钮在同一行
    QHBoxLayout* pathLayout = new QHBoxLayout();

    // 【设置路径行控件间距】setSpacing(12)表示标签、输入框、按钮之间保持12像素的间隙
    // 视觉效果：三者之间有均匀的呼吸空间，不会挤在一起
    // 若改为setSpacing(4)，则控件之间过于紧凑；若改为setSpacing(24)，则间距过大显得松散
    pathLayout->setSpacing(12);

    // 【设置路径行四周边距】setContentsMargins(左, 上, 右, 下) = (0, 16, 0, 0)
    // 视觉效果：路径行顶部有16像素的内边距，与上方的上传区之间形成16px的垂直间隙
    // 左右下三边为0，表示路径行紧贴内容面板左右边缘（但受contentLayout的32px内边距限制）
    // 若改为(0, 32, 0, 0)，则上传区与路径行之间距离增大到32px
    pathLayout->setContentsMargins(0, 16, 0, 0);

    // 【创建路径标签】QLabel显示静态文本"或输入技能路径"
    // 参数m_contentWidget表示父窗口是内容面板，建立正确的父子对象树关系
    QLabel* pathLabel = new QLabel("或输入技能路径", m_contentWidget);

    // 【设置路径标签内联样式】通过setStyleSheet直接设置字体大小和粗细
    // font-size: 13px → 文字高度约13像素；改大如16px则文字变大，改小如11px则文字变小
    // font-weight: 500 → 中等粗细（400=正常，700=加粗）；改700则加粗，改400则恢复正常
    pathLabel->setStyleSheet("font-size: 13px; font-weight: 500;");

    // 【将路径标签加入路径行】标签位于行最左侧
    pathLayout->addWidget(pathLabel);

    // 【创建路径输入框】QLineEdit提供单行文本输入；用户可手动输入技能配置文件路径
    m_pathEdit = new QLineEdit(m_contentWidget);

    // 【设置输入框对象名称】setObjectName("pathInput")用于在QSS样式表中通过#pathInput精确定位此输入框
    // 在applyTheme()中会设置其背景色、文字色、边框、圆角、内边距等样式
    m_pathEdit->setObjectName("pathInput");

    // 【设置输入框占位提示文字】当输入框为空时显示的灰色提示文本
    // 提示用户输入格式，如"例如: /path/to/skill-config.yaml"
    m_pathEdit->setPlaceholderText("例如: /path/to/skill-config.yaml");

    // 【将输入框加入路径行，并赋予拉伸系数1】addWidget(widget, stretch=1)
    // 视觉效果：输入框会占据路径行中所有剩余的可用水平空间
    // 若stretch=0，则输入框只保持默认宽度，按钮会被推到右侧边缘
    pathLayout->addWidget(m_pathEdit, 1);

    // 【创建导入按钮】QPushButton显示"导入"文本；点击后执行导入操作
    m_importBtn = new QPushButton("导入", m_contentWidget);

    // 【设置导入按钮对象名称】setObjectName("primaryButton")用于在QSS样式表中通过#primaryButton精确定位
    // 在applyTheme()中会将其设为蓝色主按钮样式（背景蓝色、文字白色、8px圆角）
    m_importBtn->setObjectName("primaryButton");

    // 【设置鼠标悬停光标】setCursor(Qt::PointingHandCursor)将鼠标指针变为手型（点击手势）
    // 视觉效果：当鼠标悬停在导入按钮上时，指针从箭头变为手型，提示用户此处可点击
    m_importBtn->setCursor(Qt::PointingHandCursor);

    // 【连接信号与槽：导入按钮点击→导入处理】
    // connect语法说明：发送者(m_importBtn)，信号(&QPushButton::clicked)，接收者(this即SkillImportPage)，槽(&SkillImportPage::onImportClicked)
    // 含义：当用户点击"导入"按钮时，QPushButton发射clicked信号，Qt自动调用SkillImportPage的onImportClicked()方法
    connect(m_importBtn, &QPushButton::clicked, this, &SkillImportPage::onImportClicked);

    // 【将导入按钮加入路径行】按钮位于行最右侧（输入框之后）
    pathLayout->addWidget(m_importBtn);

    // 【将路径行加入内容布局】路径行位于上传区下方
    contentLayout->addLayout(pathLayout);

    // ============================================
    // 【分隔线】左右横线 + 中间"已安装的技能"标题
    // ============================================

    // 【创建水平布局】用于左右横线夹中间标题的结构
    QHBoxLayout* dividerLayout = new QHBoxLayout();

    // 【设置分隔线布局四周边距】setContentsMargins(左, 上, 右, 下) = (0, 32, 0, 24)
    // 视觉效果：分隔线上方有32px的顶部内边距（与路径行之间形成32px间隙）
    // 下方有24px的底部内边距（与技能网格之间形成24px间隙）
    // 若改为(0, 16, 0, 16)，则间距减半，显得紧凑
    dividerLayout->setContentsMargins(0, 32, 0, 24);

    // 【设置分隔线布局控件间距】setSpacing(16)表示左横线与标题之间、标题与右横线之间各16像素
    dividerLayout->setSpacing(16);

    // 【创建左侧横线】QFrame是带边框的框架控件；设置形状为水平线(HLine)
    QFrame* leftLine = new QFrame(m_contentWidget);

    // 【设置左侧横线形状】QFrame::HLine表示水平线；在暗色主题下显示为 subtle 分隔线
    leftLine->setFrameShape(QFrame::HLine);

    // 【设置左侧横线对象名称】setObjectName("dividerLine")用于在QSS样式表中通过#dividerLine精确定位
    // 在applyTheme()中会设置其颜色为边框色（暗色主题通常为#1e293b或类似）
    leftLine->setObjectName("dividerLine");

    // 【将左侧横线加入分隔线布局，并赋予拉伸系数1】addWidget(widget, stretch=1)
    // 视觉效果：左侧横线会占据标题左侧所有剩余的可用水平空间，形成从边缘到标题的完整线条
    dividerLayout->addWidget(leftLine, 1);

    // 【创建中间标题标签】显示"已安装的技能"文本
    QLabel* dividerLabel = new QLabel("已安装的技能", m_contentWidget);

    // 【设置标题标签内联样式】font-size: 13px表示文字高度13像素；font-weight: 500表示中等粗细
    // white-space: nowrap表示文字不换行，始终在一行显示（防止"已安装的技能"被拆成两行）
    dividerLabel->setStyleSheet("font-size: 13px; font-weight: 500; white-space: nowrap;");

    // 【将标题标签加入分隔线布局】标题位于左右横线中间
    dividerLayout->addWidget(dividerLabel);

    // 【创建右侧横线】与左侧横线对称
    QFrame* rightLine = new QFrame(m_contentWidget);
    rightLine->setFrameShape(QFrame::HLine);
    rightLine->setObjectName("dividerLine");

    // 【将右侧横线加入分隔线布局，并赋予拉伸系数1】
    // 视觉效果：右侧横线会占据标题右侧所有剩余的可用水平空间
    dividerLayout->addWidget(rightLine, 1);

    // 【将分隔线布局加入内容布局】分隔线位于路径行下方
    contentLayout->addLayout(dividerLayout);

    // 【将技能网格加入内容布局】网格位于分隔线下方
    contentLayout->addLayout(m_skillGrid);

    // ============================================
    // 【底部信息栏】技能统计 + 外部链接按钮
    // ============================================

    // 【创建底部信息栏布局】QHBoxLayout使统计标签和链接按钮水平排列
    QHBoxLayout* bottomLayout = new QHBoxLayout();

    // 【设置底部布局四周边距】setContentsMargins(左, 上, 右, 下) = (0, 24, 0, 0)
    // 视觉效果：底部信息栏顶部有24px的内边距，与上方的技能网格之间形成24px间隙
    bottomLayout->setContentsMargins(0, 24, 0, 0);

    // 【设置底部布局控件间距】setSpacing(8)表示统计标签与链接按钮之间8像素间隙
    bottomLayout->setSpacing(8);

    // 【创建技能统计标签】显示已安装技能数量和启用数量
    QLabel* countLabel = new QLabel("共 4 个技能 · 3 个已启用", m_contentWidget);

    // 【设置统计标签字体大小】font-size: 12px表示文字较小，作为辅助信息呈现
    // 改大如14px则文字更醒目，改小如10px则更难阅读
    countLabel->setStyleSheet("font-size: 12px;");

    // 【将统计标签加入底部布局】标签位于行最左侧
    bottomLayout->addWidget(countLabel);

    // 【添加水平弹性空间】addStretch()在统计标签和链接按钮之间添加可伸缩的空白区域
    // 视觉效果：统计标签靠左，链接按钮靠右，两者之间自动拉开最大距离
    bottomLayout->addStretch();

    // 【创建外部链接按钮】显示"从仓库获取更多技能 →"文本；点击后可打开外部网页或仓库
    QPushButton* linkBtn = new QPushButton("从仓库获取更多技能 →", m_contentWidget);

    // 【设置链接按钮对象名称】setObjectName("linkButton")用于在QSS样式表中通过#linkButton精确定位
    // 在applyTheme()中会将其设为透明背景、蓝色文字、无边框；悬停时显示下划线
    linkBtn->setObjectName("linkButton");

    // 【设置鼠标悬停光标】指针变为手型，提示可点击
    linkBtn->setCursor(Qt::PointingHandCursor);

    // 【将链接按钮加入底部布局】按钮位于行最右侧
    bottomLayout->addWidget(linkBtn);

    // 【将底部信息栏加入内容布局】位于技能网格下方
    contentLayout->addLayout(bottomLayout);

    // 【添加垂直弹性空间】addStretch()在底部信息栏下方添加可伸缩的空白区域
    // 视觉效果：当页面内容不足以填满窗口高度时，底部信息栏下方会出现空白，
    // 使底部信息栏不会过度下移，技能网格也不会被过度拉伸
    contentLayout->addStretch();

    // 【将内容面板设置到滚动区域】setWidget(m_contentWidget)将内容面板注册为滚动区域的滚动内容
    // 滚动区域会根据内容面板的大小自动计算滚动范围并显示垂直滚动条
    m_scrollArea->setWidget(m_contentWidget);

    // 【将滚动区域加入主布局】滚动区域占据SkillImportPage的全部可用空间
    mainLayout->addWidget(m_scrollArea);
}

/**
 * @brief 构建拖拽上传区域
 *
 * 【视觉效果详解】
 * 上传区域是一个固定高度180像素的垂直居中面板，包含：
 * 1. 云上传图标（48×48像素，灰色#94A3B8）—— 位于区域正中央上方
 * 2. 主标题"拖拽技能包到此处，或点击上传"（15px字体，600粗细，居中）
 * 3. 格式提示"支持 .zip, .json, .yaml 格式的技能配置文件"（13px字体，居中）
 * 4. "选择文件"按钮（带边框的轮廓按钮样式，居中）
 *
 * 【QSS样式说明】在applyTheme()中通过#uploadZone设置：
 * - background-color: %2 → 使用次级背景色（暗色主题通常为#1e293b）；改#ff0000则变红色
 * - border: 2px dashed %4 → 2像素虚线边框，颜色为边框色；改solid则为实线，改3px则边框更粗
 * - border-radius: 16px → 圆角半径16像素；改0则变成直角，改32px则更圆润
 * - hover状态: border-color变为主色调蓝色(%6)，背景变为第三级背景色(%3)
 *
 * 【交互设计】
 * - 整个上传区设置了PointingHandCursor手型光标，提示用户可点击
 * - 点击上传区会触发onUploadClicked()弹出文件选择对话框
 * - 工具提示"点击上传技能文件"在鼠标悬停几秒后显示
 */
void SkillImportPage::setupUploadZone()
{
    // 【创建上传区域容器】QWidget作为承载上传图标、标题、提示和按钮的面板
    // 参数m_contentWidget表示父窗口是内容面板，建立正确的父子对象树关系
    m_uploadZone = new QWidget(m_contentWidget);

    // 【设置上传区域对象名称】setObjectName("uploadZone")用于在QSS样式表中通过#uploadZone精确定位此面板
    // 在applyTheme()中会设置其背景色、虚线边框、圆角等样式
    m_uploadZone->setObjectName("uploadZone");

    // 【设置鼠标悬停光标】setCursor(Qt::PointingHandCursor)将鼠标指针变为手型
    // 视觉效果：当鼠标悬停在上传区任何位置时，指针变为手型，提示用户此处可点击
    m_uploadZone->setCursor(Qt::PointingHandCursor);

    // 【设置上传区域固定高度】setFixedHeight(180)将高度锁定为180像素
    // 视觉效果：上传区高度始终为180px，不会随窗口大小变化而拉伸或压缩
    // 若改为setFixedHeight(240)，则上传区变高，图标和文字之间空间更大
    // 若改为setFixedHeight(120)，则上传区变矮，内容更紧凑
    m_uploadZone->setFixedHeight(180);

    // 【创建上传区内部布局】QVBoxLayout使子控件垂直居中堆叠排列
    QVBoxLayout* zoneLayout = new QVBoxLayout(m_uploadZone);

    // 【设置布局对齐方式为居中】setAlignment(Qt::AlignCenter)使所有子控件在垂直和水平方向上都居中对齐
    // 视觉效果：图标、标题、提示文字、按钮都位于上传区的正中央
    zoneLayout->setAlignment(Qt::AlignCenter);

    // 【设置上传区内部控件间距】setSpacing(12)表示图标与标题之间、标题与提示之间、提示与按钮之间各12像素
    // 视觉效果：元素之间有均匀的呼吸空间，层次分明但不疏离
    zoneLayout->setSpacing(12);

    // ============================================
    // 【上传图标】
    // ============================================

    // 【创建图标标签】QLabel用于显示图片；这里显示从IconHelper获取的云上传SVG图标
    QLabel* iconLabel = new QLabel(m_uploadZone);

    // 【设置图标图片】IconHelper::cloudUpload(48, QColor("#94A3B8"))生成48×48像素的云上传图标
    // 参数说明：48 → 图标宽度和高度均为48像素；改大如64则图标更大，改小如32则图标更小
    // QColor("#94A3B8") → 图标颜色为浅灰色（暗色主题中常见的次要文字色）；改#3B82F6则图标变蓝色
    iconLabel->setPixmap(IconHelper::cloudUpload(48, QColor("#94A3B8")));

    // 【设置图标标签居中对齐】setAlignment(Qt::AlignCenter)使图标在标签区域内水平和垂直居中
    iconLabel->setAlignment(Qt::AlignCenter);

    // 【将图标标签加入上传区布局】图标位于最上方
    zoneLayout->addWidget(iconLabel);

    // ============================================
    // 【主标题】
    // ============================================

    // 【创建主标题标签】显示"拖拽技能包到此处，或点击上传"
    QLabel* titleLabel = new QLabel("拖拽技能包到此处，或点击上传", m_uploadZone);

    // 【设置主标题内联样式】
    // font-size: 15px → 标题文字高度15像素；改大如18px则标题更醒目，改小如13px则与正文一样大
    // font-weight: 600 → 半加粗（400=正常，700=加粗）；改700则加粗，改400则恢复正常
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 600;");

    // 【设置主标题居中对齐】使标题文字在上传区水平居中
    titleLabel->setAlignment(Qt::AlignCenter);

    // 【将主标题加入上传区布局】标题位于图标下方
    zoneLayout->addWidget(titleLabel);

    // ============================================
    // 【格式提示】
    // ============================================

    // 【创建格式提示标签】显示支持的文件格式
    QLabel* hintLabel = new QLabel("支持 .zip, .json, .yaml 格式的技能配置文件", m_uploadZone);

    // 【设置提示文字字体大小】font-size: 13px表示比标题小两级，作为辅助说明呈现
    hintLabel->setStyleSheet("font-size: 13px;");

    // 【设置提示标签居中对齐】使提示文字在上传区水平居中
    hintLabel->setAlignment(Qt::AlignCenter);

    // 【将提示标签加入上传区布局】提示位于标题下方
    zoneLayout->addWidget(hintLabel);

    // ============================================
    // 【选择文件按钮】
    // ============================================

    // 【创建选择文件按钮】QPushButton显示"选择文件"文本
    QPushButton* selectBtn = new QPushButton("选择文件", m_uploadZone);

    // 【设置按钮对象名称】setObjectName("outlineButton")用于在QSS样式表中通过#outlineButton精确定位
    // 在applyTheme()中会将其设为透明背景、蓝色边框的轮廓按钮样式；悬停时背景变蓝、文字变白
    selectBtn->setObjectName("outlineButton");

    // 【设置鼠标悬停光标】指针变为手型，提示可点击
    selectBtn->setCursor(Qt::PointingHandCursor);

    // 【连接信号与槽：选择文件按钮点击→上传处理】
    // 当用户点击"选择文件"按钮时，弹出QFileDialog文件选择对话框
    connect(selectBtn, &QPushButton::clicked, this, &SkillImportPage::onUploadClicked);

    // 【将选择文件按钮加入上传区布局】按钮位于提示文字下方
    // 参数0表示无拉伸系数；Qt::AlignCenter表示按钮在布局中水平居中（不占用整行宽度）
    zoneLayout->addWidget(selectBtn, 0, Qt::AlignCenter);

    // 【设置上传区工具提示】当鼠标悬停在上传区上几秒后，显示"点击上传技能文件"气泡提示
    m_uploadZone->setToolTip("点击上传技能文件");
}

/**
 * @brief 构建已安装技能网格
 *
 * 【功能说明】创建QGridLayout网格布局（每行2列，控件间距16像素），
 * 并预置4个示例技能数据，为每个技能调用createSkillCard()创建卡片并添加到网格。
 *
 * 【预置示例数据】
 * 1. Python 代码助手（v2.1.0，代码生成类型，蓝橙渐变"#3B82F6, #F59E0B"，已启用true）
 * 2. 数据分析技能（v1.3.2，数据分析类型，绿青渐变"#16A34A, #22D3EE"，已启用true）
 * 3. 技术文档撰写（v1.0.0，文档撰写类型，橙红渐变"#F59E0B, #F97316"，已禁用false）
 * 4. 自动化工作流（v3.2.1，自动化类型，紫蓝渐变"#8B5CF6, #6366F1"，已启用true）
 *
 * 【网格布局参数】
 * - setSpacing(16)：网格内控件之间16像素间距；改大则卡片之间空隙更大，改小则更紧凑
 * - 每行2列：由createSkillCard()中的 count()/2 和 count()%2 自动计算行列位置
 */
void SkillImportPage::setupSkillGrid()
{
    // 【创建技能网格布局】QGridLayout按行和列排列控件；此处每行2列
    m_skillGrid = new QGridLayout();

    // 【设置网格控件间距】setSpacing(16)表示行与行之间、列与列之间均为16像素
    // 视觉效果：技能卡片之间有16px的均匀间隙，卡片不会贴在一起
    // 若改为setSpacing(8)，则间隙减半，网格更紧凑
    // 若改为setSpacing(24)，则间隙增大，网格更松散
    m_skillGrid->setSpacing(16);

    // 【初始化示例技能数据】使用C++11列表初始化语法一次性填充4个技能信息
    // 每个SkillInfo包含：name(名称), version(版本), description(描述), type(类型), gradient(渐变色), enabled(启用状态)
    m_skills = {
        {"Python 代码助手", "v2.1.0", "智能 Python 代码生成、优化与调试辅助，支持主流框架", "代码生成", "#3B82F6, #F59E0B", true},
        {"数据分析技能", "v1.3.2", "自动数据清洗、可视化与统计分析，支持 Pandas 和 SQL", "数据分析", "#16A34A, #22D3EE", true},
        {"技术文档撰写", "v1.0.0", "根据代码仓库自动生成 API 文档、使用指南与变更日志", "文档撰写", "#F59E0B, #F97316", false},
        {"自动化工作流", "v3.2.1", "创建自定义自动化流水线，支持定时任务与事件触发", "自动化", "#8B5CF6, #6366F1", true}
    };

    // 【遍历所有技能并创建卡片】
    // i从0开始递增，每次循环取出一个SkillInfo，调用createSkillCard()将其添加到网格
    // row = m_skillGrid->count() / 2：当前网格中已有控件数量除以2得到行号（每行2列）
    // col = m_skillGrid->count() % 2：当前网格中已有控件数量取模2得到列号（0或1）
    for (int i = 0; i < m_skills.size(); ++i) {
        const auto& skill = m_skills[i];  // 获取当前技能的常量引用，避免复制开销
        createSkillCard(skill.name, skill.version, skill.description,
                        skill.type, skill.gradient, skill.enabled);
    }
}

/**
 * @brief 创建单个技能卡片并添加到网格
 * @param name 技能名称（如 "Python 代码助手"）
 * @param version 技能版本号（如 "v2.1.0"）
 * @param description 技能描述文本（自动换行显示）
 * @param type 技能类型标签（如 "代码生成"）
 * @param gradient 渐变色定义（格式如"#3B82F6, #F59E0B"，两个HEX颜色逗号分隔）
 * @param enabled 初始启用状态（true=启用，false=禁用）
 *
 * 【卡片内部结构】
 * 技能卡片是一个QWidget容器，内部使用QVBoxLayout垂直排列：
 * ┌──────────────────────────────────────────┐
 * │ ┌────┬───────────────┬──────────┐        │ ← 头部(header)：图标 + 名称 + 版本徽章
 * │ │图标│ 技能名称      │ v2.1.0   │        │
 * │ └────┘               └──────────┘        │
 * │ 智能 Python 代码生成、优化与调试辅助...     │ ← 描述(descLabel)
 * │ ┌────────────┐          ┌────┬────┐      │ ← 底部(footer)：类型标签 + 配置按钮 + 开关
 * │ │ 代码生成   │          │配置│ ●  │      │
 * │ └────────────┘          └────┴────┘      │
 * └──────────────────────────────────────────┘
 *
 * 【视觉效果参数详解】
 * - 卡片内边距：20px（四边）；改大如32px则内容与边缘距离更大，改小如12px则更紧凑
 * - 卡片内部间距：12px（header、desc、footer之间）；改大如16px则元素间距更大
 * - 渐变色图标：32×32像素，8px圆角；使用qlineargradient实现从左上到右下的双色渐变
 *   · 改setFixedSize(48, 48)则图标变大，改(24, 24)则图标变小
 *   · 改border-radius: 8px为16px则图标更圆润，改0px则变成直角方块
 * - 版本徽章：背景第三级背景色，文字第三级文字色，1px上下内边距，8px左右内边距，999px圆角形成胶囊形状
 *   · 改padding: 1px 8px为2px 12px则徽章变大，文字周围留白更多
 *   · 改border-radius: 999px为4px则变成圆角矩形而非胶囊
 * - 启用开关：44×24像素，12px圆角形成药丸形状；蓝色(#3B82F6)=启用，灰色(#475569)=禁用
 *   · 改setFixedSize(56, 32)则开关变大，改(36, 20)则开关变小
 *   · 改border-radius: 12px为16px则更接近圆形，改6px则更扁
 */
void SkillImportPage::createSkillCard(const QString& name, const QString& version,
                                      const QString& description, const QString& type,
                                      const QString& gradient, bool enabled)
{
    // 【计算卡片在网格中的行列位置】
    // row = 当前网格中已有控件数量 / 2：整数除法得到行号（每行2列，所以除以2）
    // 例如：已有0个控件 → row=0；已有2个控件 → row=1；已有4个控件 → row=2
    int row = m_skillGrid->count() / 2;

    // 【计算列号】
    // col = 当前网格中已有控件数量 % 2：取模运算得到列号（0表示左列，1表示右列）
    // 例如：已有0个控件 → col=0（左列）；已有1个控件 → col=1（右列）；已有2个控件 → col=0（左列）
    int col = m_skillGrid->count() % 2;

    // 【创建卡片容器】QWidget作为承载卡片所有内容的独立面板
    // 参数m_contentWidget表示父窗口是内容面板，建立正确的父子对象树关系
    QWidget* card = new QWidget(m_contentWidget);

    // 【设置卡片对象名称】setObjectName("skillCard")用于在QSS样式表中通过#skillCard精确定位所有技能卡片
    // 在applyTheme()中会设置其背景色（bgSecondary）、1px边框（borderColor）、12px圆角
    card->setObjectName("skillCard");

    // 【创建卡片内部垂直布局】QVBoxLayout使头部、描述、底部垂直堆叠排列
    QVBoxLayout* cardLayout = new QVBoxLayout(card);

    // 【设置卡片内部四周边距】setContentsMargins(左, 上, 右, 下) = (20, 20, 20, 20)
    // 视觉效果：卡片内容与卡片边框之间保持20像素的留白内边距，显得宽松现代
    // 若改为(16, 16, 16, 16)，则留白减半，内容更紧凑
    // 若改为(32, 32, 32, 32)，则留白翻倍，卡片内部空间更大
    cardLayout->setContentsMargins(20, 20, 20, 20);

    // 【设置卡片内部控件间距】setSpacing(12)表示头部、描述、底部之间各12像素
    // 视觉效果：三个区域之间有均匀的呼吸空间，层次分明
    // 若改为setSpacing(8)，则间距更小更紧凑；若改为setSpacing(16)，则更松散
    cardLayout->setSpacing(12);

    // ============================================
    // 【头部】渐变色图标 + 技能名称 + 版本徽章
    // ============================================

    // 【创建头部水平布局】QHBoxLayout使图标、名称、徽章水平排列
    QHBoxLayout* header = new QHBoxLayout();

    // 【设置头部控件间距】setSpacing(12)表示图标与名称之间、名称与徽章之间各12像素
    header->setSpacing(12);

    // 【创建渐变色方形图标】QWidget作为纯色/渐变背景的色块
    QWidget* iconWidget = new QWidget(card);

    // 【设置图标固定大小】setFixedSize(32, 32)将图标锁定为32×32像素
    // 视觉效果：所有技能卡片的图标大小一致，整齐划一
    // 若改为setFixedSize(48, 48)，则图标更大更醒目；若改为(24, 24)，则图标更小巧
    iconWidget->setFixedSize(32, 32);

    // 【设置图标样式表】使用qlineargradient实现从左上到右下的双色渐变
    // gradient.split(",")将"#3B82F6, #F59E0B"按逗号分割为两个颜色字符串
    // x1:0, y1:0, x2:1, y2:1 表示渐变方向为从左上角到右下角
    // stop:0 %1 表示渐变起点（左上）使用第一个颜色；stop:1 %2 表示渐变终点（右下）使用第二个颜色
    // border-radius: 8px 表示图标圆角半径8像素；改0则变成直角方块，改16px则更圆润接近圆形
    iconWidget->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border-radius: 8px;")
                               .arg(gradient.split(",")[0], gradient.split(",")[1]));

    // 【将图标加入头部布局】图标位于行最左侧
    header->addWidget(iconWidget);

    // 【创建技能名称标签】显示技能名称文本（如"Python 代码助手"）
    QLabel* nameLabel = new QLabel(name, card);

    // 【设置名称标签样式】font-size: 14px表示名称文字高度14像素；font-weight: 600表示半加粗
    // 视觉效果：名称在卡片中最为醒目，突出技能身份
    // 若改font-size: 16px则名称更大，改12px则更小
    // 若改font-weight: 700则加粗，改400则恢复正常
    nameLabel->setStyleSheet("font-size: 14px; font-weight: 600;");

    // 【设置名称标签尺寸策略】setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred)
    // 水平策略Expanding：名称标签会占据头部中图标与徽章之间所有剩余的水平空间
    // 垂直策略Preferred：标签保持其首选高度，不额外拉伸
    // 视觉效果：名称标签尽可能宽，推动徽章靠右，形成左中右三段式布局
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // 【将名称标签加入头部布局，并赋予拉伸系数1】
    // 拉伸系数1确保名称标签优先占用可用空间，将徽章推到右侧
    header->addWidget(nameLabel, 1);

    // 【创建版本徽章标签】显示版本号文本（如"v2.1.0"）
    QLabel* versionLabel = new QLabel(version, card);

    // 【设置版本徽章样式】
    // font-size: 11px → 版本号文字较小，作为辅助信息呈现；改大如13px则更醒目
    // font-weight: 500 → 中等粗细；改700则加粗
    // padding: 1px 8px → 上下内边距1像素，左右内边距8像素；改大则徽章变大，文字周围留白更多
    // border-radius: 999px → 极大的圆角半径形成胶囊/药丸形状；改4px则为圆角矩形
    // 注意：背景色和文字色在applyTheme()中通过#versionBadge统一设置
    versionLabel->setStyleSheet("font-size: 11px; font-weight: 500; padding: 1px 8px; border-radius: 999px;");

    // 【设置版本徽章对象名称】用于在applyTheme()中通过#versionBadge设置背景色和文字色
    versionLabel->setObjectName("versionBadge");

    // 【将版本徽章加入头部布局】徽章位于行最右侧
    header->addWidget(versionLabel);

    // 【将头部布局加入卡片垂直布局】头部位于卡片最上方
    cardLayout->addLayout(header);

    // ============================================
    // 【描述文本】
    // ============================================

    // 【创建描述标签】显示技能描述文本（自动换行）
    QLabel* descLabel = new QLabel(description, card);

    // 【设置描述样式】
    // font-size: 13px → 描述文字高度13像素，比名称小一级；改大如14px则更醒目
    // line-height: 1.5 → 行高为字体大小的1.5倍；改大如2.0则行间距更大，改1.0则行间距更小
    descLabel->setStyleSheet("font-size: 13px; line-height: 1.5;");

    // 【启用自动换行】setWordWrap(true)使描述文本在到达标签宽度边界时自动换行
    // 视觉效果：长描述不会超出卡片边界，而是折成多行显示
    // 若改为setWordWrap(false)，则长描述会在卡片边缘被截断（通常显示省略号）
    descLabel->setWordWrap(true);

    // 【将描述标签加入卡片垂直布局】描述位于头部下方
    cardLayout->addWidget(descLabel);

    // ============================================
    // 【底部】类型标签 + 配置按钮 + 启用/禁用开关
    // ============================================

    // 【创建底部水平布局】QHBoxLayout使类型标签、配置按钮、开关水平排列
    QHBoxLayout* footer = new QHBoxLayout();

    // 【设置底部控件间距】setSpacing(8)表示类型标签、按钮、开关之间各8像素
    footer->setSpacing(8);

    // 【创建类型标签】显示技能类型文本（如"代码生成"）
    QLabel* typeBadge = new QLabel(type, card);

    // 【设置类型标签样式】
    // font-size: 11px → 类型文字较小；改大如13px则更醒目
    // font-weight: 500 → 中等粗细
    // padding: 2px 10px → 上下内边距2像素，左右内边距10像素；改大则标签变大
    // border-radius: 999px → 胶囊形状；改4px则为圆角矩形
    // 注意：背景色和文字色在applyTheme()中通过#typeBadge统一设置
    typeBadge->setStyleSheet("font-size: 11px; font-weight: 500; padding: 2px 10px; border-radius: 999px;");

    // 【设置类型标签对象名称】用于在applyTheme()中通过#typeBadge设置背景色和文字色
    typeBadge->setObjectName("typeBadge");

    // 【将类型标签加入底部布局】类型标签位于行最左侧
    footer->addWidget(typeBadge);

    // 【添加水平弹性空间】addStretch()在类型标签和配置按钮之间添加可伸缩的空白区域
    // 视觉效果：类型标签靠左，配置按钮和开关靠右，两者之间自动拉开最大距离
    footer->addStretch();

    // 【创建配置按钮】QPushButton显示"配置"文本；点击后弹出技能配置对话框
    QPushButton* configBtn = new QPushButton("配置", card);

    // 【设置配置按钮对象名称】setObjectName("ghostButton")用于在QSS样式表中通过#ghostButton精确定位
    // 在applyTheme()中会将其设为透明背景、边框色边框、第三级文字色的幽灵按钮样式；悬停时背景变第三级背景色
    configBtn->setObjectName("ghostButton");

    // 【设置配置按钮固定高度】setFixedHeight(28)将按钮高度锁定为28像素
    // 视觉效果：配置按钮比默认按钮更小巧，作为辅助操作按钮呈现
    // 若改为setFixedHeight(32)，则按钮更高更醒目；若改为24，则更小巧
    configBtn->setFixedHeight(28);

    // 【设置鼠标悬停光标】指针变为手型，提示可点击
    configBtn->setCursor(Qt::PointingHandCursor);

    // 【连接信号与槽：配置按钮点击→配置信号发射】
    // 使用Lambda表达式作为槽函数：捕获this指针和name值
    // 当用户点击"配置"按钮时，发射skillConfigClicked(name)信号通知外部打开配置对话框
    connect(configBtn, &QPushButton::clicked, this, [this, name]() {
        emit skillConfigClicked(name);
    });

    // 【将配置按钮加入底部布局】按钮位于类型标签右侧（靠右对齐）
    footer->addWidget(configBtn);

    // ============================================
    // 【启用/禁用开关按钮】
    // ============================================

    // 【创建开关按钮】QPushButton无文本，仅作为视觉开关使用
    QPushButton* toggleBtn = new QPushButton(card);

    // 【设置开关对象名称】根据enabled状态决定初始对象名
    // enabled为true时对象名为"toggleOn"，false时为"toggleOff"
    // 在applyTheme()中分别设置背景色：toggleOn为蓝色(#3B82F6)，toggleOff为灰色(#475569)
    toggleBtn->setObjectName(enabled ? "toggleOn" : "toggleOff");

    // 【设置开关固定大小】setFixedSize(44, 24)将开关锁定为44×24像素
    // 视觉效果：开关呈药丸形状（因border-radius: 12px = 高度的一半）
    // 若改为setFixedSize(56, 32)，则开关变大，border-radius需同步改为16px才能保持药丸形状
    // 若改为setFixedSize(36, 20)，则开关更小更紧凑
    toggleBtn->setFixedSize(44, 24);

    // 【设置鼠标悬停光标】指针变为手型，提示可点击
    toggleBtn->setCursor(Qt::PointingHandCursor);

    // 【启用按钮的可切换模式】setCheckable(true)使按钮具有"按下/弹起"两种状态
    // 与setChecked配合，可模拟Toggle Switch（开关）的视觉效果
    toggleBtn->setCheckable(true);

    // 【设置按钮初始选中状态】setChecked(enabled)根据传入的enabled参数设置初始状态
    // true → 按钮处于"按下"状态（显示蓝色背景）；false → "弹起"状态（显示灰色背景）
    toggleBtn->setChecked(enabled);

    // 【设置开关初始样式表】根据enabled状态设置不同背景色
    // enabled=true：背景蓝色#3B82F6（主色调），border-radius: 12px形成药丸形状
    // enabled=false：背景灰色#475569（边框色/次要背景），border-radius: 12px形成药丸形状
    toggleBtn->setStyleSheet(enabled ?
        "QPushButton { background-color: #3B82F6; border-radius: 12px; }" :
        "QPushButton { background-color: #475569; border-radius: 12px; }");

    // 【连接信号与槽：开关状态切换→状态更新+信号发射】
    // 使用Lambda表达式作为槽函数：捕获this指针、name值和toggleBtn指针
    // 信号：&QPushButton::toggled(bool checked) —— 当按钮切换状态时发射，checked表示新状态
    connect(toggleBtn, &QPushButton::toggled, this, [this, name, toggleBtn](bool checked) {
        // 【更新开关对象名称】根据新状态切换对象名，便于QSS样式表定位
        toggleBtn->setObjectName(checked ? "toggleOn" : "toggleOff");

        // 【更新开关样式表】根据新状态切换背景色
        // checked=true → 背景变蓝色#3B82F6（启用状态）
        // checked=false → 背景变灰色#475569（禁用状态）
        toggleBtn->setStyleSheet(checked ?
            "QPushButton { background-color: #3B82F6; border-radius: 12px; }" :
            "QPushButton { background-color: #475569; border-radius: 12px; }");

        // 【发射技能状态切换信号】通知外部技能管理器启用或禁用对应技能
        // checked=true → skillToggled(name, true)表示启用该技能
        // checked=false → skillToggled(name, false)表示禁用该技能
        emit skillToggled(name, checked);
    });

    // 【将开关按钮加入底部布局】开关位于配置按钮右侧（最右侧）
    footer->addWidget(toggleBtn);

    // 【将底部布局加入卡片垂直布局】底部位于描述文本下方
    cardLayout->addLayout(footer);

    // 【将卡片添加到技能网格】根据之前计算的row和col定位
    // addWidget(card, row, col)将卡片放置在第row行第col列（0起始索引）
    m_skillGrid->addWidget(card, row, col);
}

/**
 * @brief 应用当前主题样式
 *
 * 【QSS样式表说明】本函数从ThemeManager获取当前主题的颜色值，拼接成一大段CSS风格的样式字符串，
 * 然后通过m_contentWidget->setStyleSheet()应用到整个页面。
 * Qt样式表（QSS）语法与CSS高度相似，支持选择器、属性、伪状态（如:hover）等。
 *
 * 【颜色变量说明】
 * - bg = tm->background() → 主背景色；暗色主题通常为#0f172a（深蓝黑）；改#ffffff则为白色
 * - bg2 = tm->bgSecondary() → 次级背景色；暗色主题通常为#1e293b（深蓝灰）；用于上传区背景、卡片背景
 * - bg3 = tm->bgTertiary() → 第三级背景色；暗色主题通常为#334155（中蓝灰）；用于悬停状态、徽章背景
 * - bdr = tm->border() → 边框色；暗色主题通常为#334155或#475569；用于边框、分隔线、开关关闭状态
 * - pri = tm->primary() → 主色调蓝色；通常为#3B82F6；用于按钮背景、悬停边框、链接文字
 * - txtPri = tm->textPrimary() → 主文字色；暗色主题通常为#f1f5f9（浅灰白）；用于主要文字
 * - txtSec = tm->textSecondary() → 次要文字色；暗色主题通常为#94a3b8（中灰）；用于悬停状态色
 * - txtTer = tm->textTertiary() → 第三级文字色；暗色主题通常为#64748b（深灰）；用于徽章文字
 *
 * 【样式选择器说明】
 * - QWidget#skillContent { ... } → 仅匹配对象名为"skillContent"的QWidget（即内容面板）
 * - QLineEdit#pathInput { ... } → 仅匹配对象名为"pathInput"的QLineEdit（路径输入框）
 * - QPushButton#primaryButton:hover { ... } → 仅匹配鼠标悬停时的"primaryButton"按钮
 * - QLabel { color: %7; } → 匹配所有QLabel，设置文字颜色为主文字色
 */
void SkillImportPage::applyTheme()
{
    // 【获取ThemeManager单例】ThemeManager::instance()返回全局唯一的主题管理器实例
    ThemeManager* tm = ThemeManager::instance();

    // 【获取主背景色】name()将QColor转换为十六进制字符串（如"#0f172a"）
    QString bg = tm->background().name();

    // 【获取次级背景色】用于上传区背景、卡片背景等次要容器
    QString bg2 = tm->bgSecondary().name();

    // 【获取第三级背景色】用于悬停状态、徽章背景、版本标签背景等
    QString bg3 = tm->bgTertiary().name();

    // 【获取边框色】用于输入框边框、卡片边框、分隔线、开关关闭状态等
    QString bdr = tm->border().name();

    // 【获取主色调】蓝色，用于主按钮背景、悬停边框、链接文字、开关开启状态等
    QString pri = tm->primary().name();

    // 【获取主文字色】浅色，用于主要文字内容
    QString txtPri = tm->textPrimary().name();

    // 【获取次要文字色】中灰色，用于主按钮悬停状态色
    QString txtSec = tm->textSecondary().name();

    // 【获取第三级文字色】深灰色，用于徽章文字、次要信息
    QString txtTer = tm->textTertiary().name();

    // 【构建并应用QSS样式表】
    // QString().arg()按顺序将%1~%9替换为对应的颜色字符串
    m_contentWidget->setStyleSheet(QString(
        // 【内容面板背景】skillContent的背景色为主背景色(bg)
        // 视觉效果：整个页面的底层背景色；暗色主题下为深蓝黑色，亮色主题下为白色或浅灰
        "QWidget#skillContent { background-color: %1; }"

        // 【上传区默认样式】uploadZone的背景为次级背景色(bg2)，2px虚线边框（颜色为边框色bdr），16px圆角
        // 视觉效果：一个带虚线边框的圆角矩形区域，提示用户此处可拖拽上传；暗色主题下为深蓝灰底色+虚线边框
        "QWidget#uploadZone { background-color: %2; border: 2px dashed %4; border-radius: 16px; }"

        // 【上传区悬停样式】uploadZone:hover的边框变为主色调蓝色(pri)，背景变为第三级背景色(bg3)
        // 视觉效果：鼠标悬停时上传区边框变蓝、背景略亮，提示用户此处可交互
        "QWidget#uploadZone:hover { border-color: %6; background-color: %3; }"

        // 【路径输入框默认样式】pathInput的背景为次级背景色(bg2)，文字为主文字色(txtPri)，1px实线边框（边框色bdr），8px圆角，8px上下12px左右内边距，13px字体
        // 视觉效果：一个带圆角的输入框，与页面风格一致；暗色主题下为深蓝灰底色+灰色边框
        "QLineEdit#pathInput { background-color: %5; color: %7; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; font-size: 13px; }"

        // 【路径输入框焦点样式】pathInput:focus的边框变为主色调蓝色(pri)
        // 视觉效果：当用户点击输入框开始输入时，边框变蓝，提示当前焦点在此
        "QLineEdit#pathInput:focus { border-color: %6; }"

        // 【主按钮默认样式】primaryButton的背景为主色调蓝色(pri)，文字白色，8px圆角，8px上下20px左右内边距，500粗细
        // 视觉效果：醒目的蓝色填充按钮，作为页面主要操作入口；白色文字在蓝色背景上对比度高
        "QPushButton#primaryButton { background-color: %6; color: white; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"

        // 【主按钮悬停样式】primaryButton:hover的背景变为次要文字色(txtSec)
        // 视觉效果：鼠标悬停时按钮颜色略变（通常变亮或变暗），反馈用户操作
        "QPushButton#primaryButton:hover { background-color: %8; }"

        // 【轮廓按钮默认样式】outlineButton的背景透明，文字和边框为主色调蓝色(pri)，8px圆角，8px上下20px左右内边距
        // 视觉效果：透明底色+蓝色边框的按钮，比主按钮更次要；适合"选择文件"等辅助操作
        "QPushButton#outlineButton { background-color: transparent; color: %6; border: 1px solid %6; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"

        // 【轮廓按钮悬停样式】outlineButton:hover的背景变为主色调蓝色(pri)，文字变白色
        // 视觉效果：鼠标悬停时按钮从"线框"变为"填充"，提供明显的交互反馈
        "QPushButton#outlineButton:hover { background-color: %6; color: white; }"

        // 【幽灵按钮默认样式】ghostButton的背景透明，文字为第三级文字色(txtTer)，1px实线边框（边框色bdr），6px圆角，4px上下12px左右内边距，12px字体，500粗细
        // 视觉效果：非常低调的按钮，几乎融入背景；适合"配置"等不重要的操作
        "QPushButton#ghostButton { background-color: transparent; color: %9; border: 1px solid %4; border-radius: 6px; padding: 4px 12px; font-size: 12px; font-weight: 500; }"

        // 【幽灵按钮悬停样式】ghostButton:hover的背景变为第三级背景色(bg3)，边框变为主色调蓝色(pri)，文字变为主文字色(txtPri)
        // 视觉效果：悬停时按钮略微浮现，边框变蓝，提示可点击
        "QPushButton#ghostButton:hover { background-color: %3; border-color: %6; color: %7; }"

        // 【链接按钮默认样式】linkButton的背景透明，文字为主色调蓝色(pri)，无边框，13px字体，500粗细
        // 视觉效果：看起来像是普通蓝色文字链接，而非按钮；适合"从仓库获取更多技能"等外链操作
        "QPushButton#linkButton { background-color: transparent; color: %6; border: none; font-size: 13px; font-weight: 500; }"

        // 【链接按钮悬停样式】linkButton:hover显示下划线text-decoration: underline
        // 视觉效果：悬停时出现下划线，符合网页链接的交互习惯
        "QPushButton#linkButton:hover { text-decoration: underline; }"

        // 【开关开启样式】toggleOn的背景为主色调蓝色(pri)，12px圆角（药丸形状）
        // 视觉效果：蓝色小药丸，表示技能已启用
        "QPushButton#toggleOn { background-color: %6; border-radius: 12px; }"

        // 【开关关闭样式】toggleOff的背景为边框色(bdr)，12px圆角（药丸形状）
        // 视觉效果：灰色小药丸，表示技能已禁用
        "QPushButton#toggleOff { background-color: %4; border-radius: 12px; }"

        // 【技能卡片默认样式】skillCard的背景为次级背景色(bg2)，1px实线边框（边框色bdr），12px圆角
        // 视觉效果：带边框的圆角矩形卡片，承载单个技能的信息
        "QWidget#skillCard { background-color: %5; border: 1px solid %4; border-radius: 12px; }"

        // 【技能卡片悬停样式】skillCard:hover的边框变为主色调蓝色(pri)
        // 视觉效果：鼠标悬停时卡片边框变蓝，提示用户此处可交互（点击配置或切换开关）
        "QWidget#skillCard:hover { border-color: %6; }"

        // 【版本徽章样式】versionBadge的背景为第三级背景色(bg3)，文字为第三级文字色(txtTer)
        // 视觉效果：胶囊形状的小标签，显示版本号，颜色低调不抢眼
        "QLabel#versionBadge { background-color: %3; color: %9; }"

        // 【类型标签样式】typeBadge的背景为第三级背景色(bg3)，文字为第三级文字色(txtTer)
        // 视觉效果：与版本徽章类似的小标签，显示技能类型
        "QLabel#typeBadge { background-color: %3; color: %9; }"

        // 【分隔线样式】dividerLine的颜色为边框色(bdr)
        // 视觉效果：一条细横线，颜色与边框一致，低调分隔上下内容区域
        "QFrame#dividerLine { color: %4; }"

        // 【所有QLabel默认文字色】QLabel的color为主文字色(txtPri)
        // 视觉效果：页面中所有标签（未单独设置样式的）使用主文字色，确保一致性
        "QLabel { color: %7; }"
    // 【参数替换】arg()按顺序将%1~%9替换为对应的颜色字符串
    ).arg(bg, bg2, bg3, bdr, bg, pri, txtPri, txtSec, txtTer));
}

/**
 * @brief 上传按钮点击槽函数
 *
 * 【信号连接】在setupUploadZone()中通过 connect(selectBtn, &QPushButton::clicked, this, &SkillImportPage::onUploadClicked) 建立关联
 * 【内部逻辑】
 * 1. 调用QFileDialog::getOpenFileName()弹出文件选择对话框
 * 2. 对话框标题为"选择技能文件"，无默认目录，文件过滤器为"Skill Files (*.zip *.json *.yaml *.yml);;All Files (*)"
 * 3. 若用户选择了文件（path非空），将路径写入m_pathEdit输入框并发射skillImported信号
 *
 * 【文件过滤器说明】
 * "Skill Files (*.zip *.json *.yaml *.yml)" → 只显示zip/json/yaml/yml文件
 * ";;" → 分隔不同过滤器选项
 * "All Files (*)" → 显示所有文件（备用选项）
 */
void SkillImportPage::onUploadClicked()
{
    // 【弹出文件选择对话框】
    // 参数1：this → 对话框的父窗口为当前SkillImportPage，对话框会居中显示
    // 参数2："选择技能文件" → 对话框窗口标题
    // 参数3：QString() → 默认打开的目录路径（空字符串表示使用系统默认目录）
    // 参数4："Skill Files (*.zip *.json *.yaml *.yml);;All Files (*)" → 文件类型过滤器
    QString path = QFileDialog::getOpenFileName(this, "选择技能文件",
                                                QString(),
                                                "Skill Files (*.zip *.json *.yaml *.yml);;All Files (*)");

    // 【检查用户是否选择了文件】path.isEmpty()为true表示用户点击了"取消"或未选择文件
    if (!path.isEmpty()) {
        // 【将路径写入输入框】m_pathEdit->setText(path)在路径输入框中显示用户选择的文件路径
        m_pathEdit->setText(path);

        // 【发射技能导入信号】通知外部主窗口或技能管理器处理导入逻辑
        emit skillImported(path);
    }
}

/**
 * @brief 导入按钮点击槽函数
 *
 * 【信号连接】在setupUI()中通过 connect(m_importBtn, &QPushButton::clicked, this, &SkillImportPage::onImportClicked) 建立关联
 * 【内部逻辑】
 * 1. 读取m_pathEdit输入框中的文本
 * 2. 调用trimmed()去除首尾空白字符（空格、换行、制表符等）
 * 3. 若处理后的路径非空，发射skillImported信号
 *
 * 【使用场景】用户手动输入技能配置文件路径后，点击"导入"按钮触发此函数。
 */
void SkillImportPage::onImportClicked()
{
    // 【读取并清理输入文本】text()获取输入框当前文本，trimmed()去除首尾空白
    // trimmed()的作用：用户可能不小心在路径前后输入了空格，清理后避免路径无效
    QString path = m_pathEdit->text().trimmed();

    // 【检查路径是否非空】若用户未输入任何内容或只输入了空白，则不执行导入
    if (!path.isEmpty()) {
        // 【发射技能导入信号】通知外部处理导入逻辑
        emit skillImported(path);
    }
}

/**
 * @brief 主题变化响应槽函数
 *
 * 【触发方式】当用户切换暗色/亮色主题时，ThemeManager发射themeChanged信号，
 * 本页面通过connect()连接该信号到此槽函数，从而重新应用颜色样式。
 *
 * 【执行动作】直接调用applyTheme()，从ThemeManager获取新的颜色值并重新构建QSS样式表，
 * 然后通过m_contentWidget->setStyleSheet()应用到整个页面，实现无闪烁刷新。
 */
void SkillImportPage::onThemeChanged()
{
    applyTheme();
}
