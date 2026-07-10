/**
 * @file modelconfigpage.cpp
 * @brief 模型配置页面实现
 *
 * 【整体功能】
 * 本文件实现了 ModelConfigPage 类的所有成员函数，负责构建完整的模型配置 UI 界面，
 * 并处理用户的各种交互操作（点击按钮、修改数值、选择后端等）。
 *
 * 【页面结构从上到下】
 * 1. 连接状态卡片 — 显示当前与模型服务器的连接状态
 * 2. 模型设置 — API 端点、模型名称、已安装模型下拉框、推理后端选择
 * 3. 高级参数 — Temperature/TopP/MaxTokens 等 6 项参数调节
 * 4. 底部操作栏 — "恢复默认"按钮（左）和"保存配置"按钮（右）
 * 5. 已安装模型网格 — 以卡片形式展示服务器上的模型列表
 *
 * 【关键技术】
 * - Qt 布局系统（QVBoxLayout/QHBoxLayout/QGridLayout）自动管理控件位置和大小
 * - QSS 样式表（类似 CSS）控制颜色、边框、圆角等视觉效果
 * - 信号槽机制实现按钮点击与业务逻辑的解耦
 */

#include "modelconfigpage.h"    // 【引入头文件】本类自身的声明
#include "thememanager.h"       // 【引入主题管理器】获取暗色/亮色主题的颜色值
#include "iconhelper.h"         // 【引入图标辅助类】绘制各种矢量图标（本文件中未直接使用，但头文件已包含备用）
#include <QButtonGroup>         // 【按钮组】管理一组互斥的单选按钮
#include <QRadioButton>         // 【单选按钮】推理后端选择（CPU/CUDA/Metal）
#include <QFormLayout>          // 【表单布局】标签-输入框成对排列（本文件未直接使用，但保留了头文件引入）
#include <QFrame>               // 【框架控件】用于创建分隔线等视觉元素
#include <QFileDialog>          // 【文件对话框】打开文件选择窗口（本文件未直接使用，保留备用）

/**
 * @brief 构造函数
 * @param parent 父 QWidget 指针
 *
 * 【初始化流程】
 * 1. 调用 setupUI() 创建和排列所有界面控件
 * 2. 调用 applyTheme() 应用当前主题的颜色样式
 * 3. 连接 ThemeManager 的 themeChanged 信号到 onThemeChanged 槽函数，
 *    这样当用户切换暗色/亮色主题时，本页面会自动刷新颜色。
 *
 * 【调试提示】如果页面打开后看不到内容，检查 setupUI() 是否被调用；
 * 如果颜色显示异常，检查 applyTheme() 中的 QSS 字符串是否完整。
 */
ModelConfigPage::ModelConfigPage(QWidget *parent)
    : QWidget(parent)   // 【初始化列表】调用父类 QWidget 的构造函数，传入父指针
{
    setupUI();          // 【创建界面】构建所有控件和布局
    applyTheme();       // 【应用主题】设置颜色、边框、圆角等视觉样式

    // 【信号槽连接】将 ThemeManager 的主题变化信号连接到这个页面的刷新函数
    // 当用户点击主题切换按钮时，ThemeManager 会发射 themeChanged 信号，
    // 然后自动调用本页面的 onThemeChanged() 槽函数重新应用颜色。
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ModelConfigPage::onThemeChanged);
}

/**
 * @brief 析构函数
 *
 * 【Qt 自动内存管理】本类所有子控件（QLabel、QLineEdit 等）在创建时都指定了 this 作为父对象，
 * 因此当本 ModelConfigPage 对象被销毁时，Qt 会自动删除所有子控件，无需手动 delete。
 */
ModelConfigPage::~ModelConfigPage()
{
}

/**
 * @brief 设置模型配置页面整体 UI 布局
 *
 * 【布局结构详解】
 * 整个页面采用"滚动区域"设计，确保内容超出窗口高度时可以上下滚动：
 *
 * ModelConfigPage (this)
 * └── mainLayout (QVBoxLayout, 无边距无间距)
 *     └── m_scrollArea (QScrollArea, 占据全部空间)
 *         └── m_contentWidget (QWidget, 实际内容容器)
 *             └── contentLayout (QVBoxLayout, 左右边距32像素)
 *                 ├── m_connectionCard (连接状态卡片)
 *                 ├── 28px 间距
 *                 ├── m_modelSettingsGroup (模型设置)
 *                 ├── 28px 间距
 *                 ├── m_advancedGroup (高级参数)
 *                 ├── 28px 间距
 *                 ├── actionLayout (底部操作栏)
 *                 ├── 24px 间距
 *                 ├── m_installedModelsGroup (已安装模型网格)
 *                 └── 底部弹性空间 (addStretch, 将内容向上推)
 *
 * 【关键参数说明】
 * - setContentsMargins(0,0,0,0)：布局四周不留空白；改大则页面边缘出现空白
 * - setSpacing(0)：布局内控件之间默认无间距；改大则控件之间空隙变大
 * - setContentsMargins(32,32,32,32)：内容区左右上下各留32像素边距；改大则内容区变窄
 */
void ModelConfigPage::setupUI()
{
    // 【主布局】垂直布局，铺满整个页面
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);  // 四周边距设为0，让滚动区域完全贴合页面边界
    mainLayout->setSpacing(0);                   // 布局内控件间距设为0

    // 【滚动区域】当页面内容超出窗口高度时，自动显示垂直滚动条
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);              // 内容控件可随滚动区域大小自动调整宽度
    m_scrollArea->setFrameShape(QFrame::NoFrame);        // 不显示边框；改为 QFrame::Box 则出现矩形边框
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);  // 永远隐藏水平滚动条

    // 【内容面板】滚动区域内部的真实内容容器
    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setObjectName("configContent");     // 设置对象名，便于 QSS 中通过 #configContent 精准定位
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(32, 32, 32, 32);   // 内容区四周留白32像素；改大则内容区变窄，改0则贴边
    contentLayout->setSpacing(0);                        // 默认无间距，通过 addSpacing() 手动控制各区域间距

    // 【创建各功能区域】依次构建连接卡片、模型设置、高级参数、模型网格
    setupConnectionCard();
    setupModelSettings();
    setupAdvancedSettings();
    setupModelGrid();

    // 【按顺序添加各区域到内容布局】
    contentLayout->addWidget(m_connectionCard);   // 添加连接状态卡片
    contentLayout->addSpacing(28);                // 卡片与下方区域之间留28像素空隙；改大则间距变大
    contentLayout->addWidget(m_modelSettingsGroup); // 添加模型设置区域
    contentLayout->addSpacing(28);                // 28像素间距
    contentLayout->addWidget(m_advancedGroup);    // 添加高级参数区域
    contentLayout->addSpacing(28);                // 28像素间距

    // 【底部操作栏】水平排列：左对齐"恢复默认"按钮 + 弹性空间 + 右对齐"保存配置"按钮
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 20, 0, 36);  // 上20、下36像素内边距；改大则按钮与上下方距离增大
    actionLayout->setSpacing(12);                    // 按钮之间12像素间距

    // 【恢复默认按钮】灰色幽灵风格，点击后重置所有输入为默认值
    m_restoreBtn = new QPushButton("恢复默认", m_contentWidget);
    m_restoreBtn->setObjectName("ghostButton");      // 通过 QSS 中的 #ghostButton 设置透明背景+灰色文字样式
    m_restoreBtn->setCursor(Qt::PointingHandCursor); // 鼠标悬停时显示手型指针，提示可点击
    // 【信号槽连接】按钮点击 → 调用 onRestoreDefaultsClicked() 槽函数
    connect(m_restoreBtn, &QPushButton::clicked, this, &ModelConfigPage::onRestoreDefaultsClicked);
    actionLayout->addWidget(m_restoreBtn);           // 将按钮添加到左侧

    actionLayout->addStretch();                      // 【弹性空间】将左右按钮向两侧推开，实现左+右分布

    // 【保存配置按钮】蓝色主按钮，点击后通知外部保存当前设置
    m_saveBtn = new QPushButton("保存配置", m_contentWidget);
    m_saveBtn->setObjectName("primaryButton");       // QSS 中通过 #primaryButton 设置为蓝色背景+白色文字
    m_saveBtn->setCursor(Qt::PointingHandCursor);    // 鼠标悬停变手型
    // 【信号槽连接】按钮点击 → 调用 onSaveClicked() 槽函数
    connect(m_saveBtn, &QPushButton::clicked, this, &ModelConfigPage::onSaveClicked);
    actionLayout->addWidget(m_saveBtn);              // 将按钮添加到右侧

    contentLayout->addLayout(actionLayout);          // 将操作栏添加到内容布局
    contentLayout->addSpacing(24);                   // 操作栏与模型网格之间24像素间距
    contentLayout->addWidget(m_installedModelsGroup); // 添加已安装模型网格
    contentLayout->addStretch();                     // 【底部弹性空间】将以上所有内容向上顶，底部留白

    // 【将内容面板放入滚动区域】
    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);             // 滚动区域铺满主布局
}

/**
 * @brief 设置连接状态卡片
 *
 * 【视觉效果详解】
 * 这是一个横向排列的圆角矩形卡片，整体宽度填满内容区。
 * - 左侧：绿色/灰色圆点（10×10像素）+ 两行文字（状态标题+详情）
 * - 右侧："测试连接"按钮
 *
 * 【QSS样式影响】在 applyTheme() 中通过 #connectionCard 设置：
 * - background-color：卡片背景色
 * - border: 1px solid xxx：1像素边框
 * - border-radius: 12px：圆角半径12像素；改0则变成直角，改20则更圆润
 */
void ModelConfigPage::setupConnectionCard()
{
    m_connectionCard = new QWidget(m_contentWidget);
    m_connectionCard->setObjectName("connectionCard");   // QSS定位用；样式在 applyTheme() 中统一设置
    QHBoxLayout* cardLayout = new QHBoxLayout(m_connectionCard);
    cardLayout->setContentsMargins(20, 16, 20, 16);      // 卡片内部边距：左右20、上下16像素；改大则内容与边缘距离增大
    cardLayout->setSpacing(12);                          // 子控件之间12像素间距

    // 【状态显示区】包含圆点和文字，水平排列
    QWidget* statusWidget = new QWidget(m_connectionCard);
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(0, 0, 0, 0);        // 无内边距
    statusLayout->setSpacing(12);                        // 圆点与文字之间12像素

    // 【状态指示圆点】10×10像素的 QLabel，通过 QSS 设置为圆形背景色
    m_statusDot = new QLabel(statusWidget);
    m_statusDot->setFixedSize(10, 10);                   // 固定宽高10像素；改大则圆点变大
    m_statusDot->setObjectName("statusDot");             // QSS中通过 #statusDot 设置圆形和颜色
    statusLayout->addWidget(m_statusDot);

    // 【文字区域】垂直排列标题和详情两行
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);                           // 标题与详情之间2像素间距；改大则两行文字距离增大
    m_statusLabel = new QLabel("未连接", statusWidget);   // 默认显示"未连接"；连接成功后变为"模型已连接"
    m_statusLabel->setObjectName("statusTitle");
    m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500;");  // 14px中等粗细字体；改大则字变大，改400则不粗
    textLayout->addWidget(m_statusLabel);

    m_statusDetail = new QLabel("请配置模型端点并测试连接", statusWidget); // 默认提示文字
    m_statusDetail->setObjectName("statusDetail");
    m_statusDetail->setStyleSheet("font-size: 12px;");   // 12px小字；改14px则变大
    textLayout->addWidget(m_statusDetail);
    statusLayout->addLayout(textLayout);

    cardLayout->addWidget(statusWidget);
    cardLayout->addStretch();                            // 【弹性空间】将状态区推到左侧，按钮留在右侧

    // 【测试连接按钮】点击后通知外部进行网络连接测试
    m_testConnectionBtn = new QPushButton("测试连接", m_connectionCard);
    m_testConnectionBtn->setObjectName("secondaryButton"); // QSS中设置为浅色背景+深色边框样式
    m_testConnectionBtn->setCursor(Qt::PointingHandCursor);
    // 【信号槽连接】按钮点击 → 调用 onTestConnectionClicked() 槽函数
    connect(m_testConnectionBtn, &QPushButton::clicked, this, &ModelConfigPage::onTestConnectionClicked);
    cardLayout->addWidget(m_testConnectionBtn);
}

/**
 * @brief 设置模型基础设置区域
 *
 * 【包含控件】
 * 1. "模型设置"标题（15px 粗体，白色/黑色）
 * 2. 模型端点输入框 — 带灰色/绿色"未连接/已连接"徽章
 * 3. 模型名称手动输入框
 * 4. 已安装模型下拉框 — 初始禁用，测试连接成功后启用并填充
 * 5. 推理后端单选按钮组 — CPU / CUDA(GPU) / Metal(Mac)
 *
 * 【调试提示】
 * - 若输入框文字颜色看不清，检查 applyTheme() 中 #formInput 的 color 和 background-color 设置
 * - 若徽章位置不对，检查 endpointInputLayout 的 addWidget(m_apiUrlEdit, 1) 拉伸因子
 */
void ModelConfigPage::setupModelSettings()
{
    m_modelSettingsGroup = new QWidget(m_contentWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_modelSettingsGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);   // 无内边距
    groupLayout->setSpacing(20);                    // 每个配置项之间20像素间距；改大则项间距增大

    // 【区域标题】
    QLabel* title = new QLabel("模型设置", m_modelSettingsGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");  // 15px加粗；改18px则更大更醒目
    groupLayout->addWidget(title);

    // ===== 模型端点输入 =====
    QVBoxLayout* endpointLayout = new QVBoxLayout();
    endpointLayout->setSpacing(6);                  // 标签、提示文字、输入行之间6像素间距

    QLabel* endpointLabel = new QLabel("模型端点 (Endpoint)", m_modelSettingsGroup);
    endpointLabel->setStyleSheet("font-size: 13px; font-weight: 500;");  // 13px中等粗细；作为配置项标题
    endpointLayout->addWidget(endpointLabel);

    QLabel* endpointHint = new QLabel("本地大模型的 API 服务地址", m_modelSettingsGroup);
    endpointHint->setStyleSheet("font-size: 12px;");  // 12px灰色小字提示；颜色在 applyTheme() 的 QLabel 规则中设置
    endpointLayout->addWidget(endpointHint);

    // 【输入行】水平布局：输入框（占满剩余宽度）+ 状态徽章
    QHBoxLayout* endpointInputLayout = new QHBoxLayout();
    endpointInputLayout->setSpacing(8);             // 输入框与徽章之间8像素间距
    m_apiUrlEdit = new QLineEdit(m_modelSettingsGroup);
    m_apiUrlEdit->setObjectName("formInput");       // QSS中 #formInput 控制外观
    m_apiUrlEdit->setPlaceholderText("例如: http://localhost:8080/v1"); // 输入框为空时显示的灰色提示文字
    // 【拉伸因子1】表示输入框会占据所有剩余水平空间；改0则输入框保持默认宽度
    endpointInputLayout->addWidget(m_apiUrlEdit, 1);

    // 【连接状态徽章】胶囊形状标签，显示在输入框右侧
    QLabel* connectedBadge = new QLabel("未连接", m_modelSettingsGroup);
    connectedBadge->setObjectName("successBadge");   // 虽然叫 successBadge，但初始状态是灰色"未连接"
    // 【内联QSS】直接设置徽章样式：浅灰背景+灰字，圆角999px形成胶囊形状
    connectedBadge->setStyleSheet(
        "background: rgba(148, 163, 184, 0.1); color: #94A3B8; "
        "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
        // 样式说明：
        // - background: rgba(148,163,184,0.1) → 浅灰蓝色半透明背景；改 rgba(255,0,0,0.1) 则变淡红色
        // - color: #94A3B8 → 文字灰蓝色；改 #22C55E 则变绿色（连接成功后）
        // - padding: 4px 12px → 上下4、左右12像素内边距；改大则徽章变高变宽
        // - border-radius: 999px → 超大圆角形成胶囊/药丸形状；改4px则变成圆角矩形
        // - font-size: 12px → 12像素字体；改14px则字更大
        // - font-weight: 500 → 中等粗细；改700则更粗
    );
    m_connectedBadge = connectedBadge;              // 保存指针到成员变量，供后续 updateConnectionStatus() 修改文字和颜色
    endpointInputLayout->addWidget(connectedBadge);
    endpointLayout->addLayout(endpointInputLayout);
    groupLayout->addLayout(endpointLayout);

    // ===== 模型名称手动输入 =====
    QVBoxLayout* modelLayout = new QVBoxLayout();
    modelLayout->setSpacing(6);

    QLabel* modelLabel = new QLabel("模型名称", m_modelSettingsGroup);
    modelLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    modelLayout->addWidget(modelLabel);

    QLabel* modelHint = new QLabel("请手动输入模型名称，测试连接通过后可从已安装模型中选择", m_modelSettingsGroup);
    modelHint->setStyleSheet("font-size: 12px;");
    modelLayout->addWidget(modelHint);

    m_modelNameEdit = new QLineEdit(m_modelSettingsGroup);
    m_modelNameEdit->setObjectName("formInput");    // 与 API 地址输入框使用相同样式
    m_modelNameEdit->setPlaceholderText("例如: Qwen-7B-Instruct");  // 灰色提示文字
    m_modelNameEdit->setFixedWidth(360);            // 【固定宽度】360像素；改大则输入框变宽，改小则变窄
    modelLayout->addWidget(m_modelNameEdit);
    groupLayout->addLayout(modelLayout);

    // ===== 已安装的模型下拉框（测试连接后填充） =====
    QVBoxLayout* installedLayout = new QVBoxLayout();
    installedLayout->setSpacing(6);

    QLabel* installedLabel = new QLabel("已安装的模型", m_modelSettingsGroup);
    installedLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    installedLayout->addWidget(installedLabel);

    QLabel* installedHint = new QLabel("测试连接成功后将从服务器获取已安装模型列表", m_modelSettingsGroup);
    installedHint->setStyleSheet("font-size: 12px;");
    installedLayout->addWidget(installedHint);

    m_modelCombo = new QComboBox(m_modelSettingsGroup);
    m_modelCombo->setObjectName("formSelect");      // QSS中 #formSelect 控制下拉框外观
    m_modelCombo->addItem("-- 请先测试连接 --");     // 默认提示项；测试连接成功后会被清除并填入真实模型
    m_modelCombo->setFixedWidth(360);               // 固定宽度360像素；与上方模型名称输入框对齐
    m_modelCombo->setEnabled(false);                // 【禁用状态】初始不可用，按钮和文字呈灰色；连接成功后调用 setEnabled(true)
    installedLayout->addWidget(m_modelCombo);
    groupLayout->addLayout(installedLayout);

    // ===== 推理后端选择 =====
    QVBoxLayout* backendLayout = new QVBoxLayout();
    backendLayout->setSpacing(8);

    QLabel* backendLabel = new QLabel("推理后端", m_modelSettingsGroup);
    backendLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    backendLayout->addWidget(backendLabel);

    // 【按钮组】管理三个单选按钮，确保同一时间只能选中一个
    m_backendGroup = new QButtonGroup(m_modelSettingsGroup);
    QHBoxLayout* backendBtnLayout = new QHBoxLayout();
    backendBtnLayout->setSpacing(6);                // 单选按钮之间6像素间距

    // 【三个单选按钮】分别对应 CPU、CUDA(GPU)、Metal(Mac) 三种推理方式
    QRadioButton* cpuBtn = new QRadioButton("CPU", m_modelSettingsGroup);
    QRadioButton* cudaBtn = new QRadioButton("CUDA (GPU)", m_modelSettingsGroup);
    QRadioButton* metalBtn = new QRadioButton("Metal (Mac)", m_modelSettingsGroup);
    cudaBtn->setChecked(true);                      // 【默认选中】CUDA(GPU)；程序启动时默认使用 GPU 加速

    // 【将按钮加入按钮组并分配ID】ID用于后续通过 checkedId() 判断选中了哪个
    m_backendGroup->addButton(cpuBtn, 0);           // ID 0 = CPU
    m_backendGroup->addButton(cudaBtn, 1);          // ID 1 = CUDA
    m_backendGroup->addButton(metalBtn, 2);         // ID 2 = Metal

    // 【统一样式设置】三个按钮都设置对象名和鼠标手型
    for (QAbstractButton* btn : m_backendGroup->buttons()) {
        btn->setObjectName("pillButton");           // QSS中 #pillButton 控制单选按钮文字颜色
        btn->setCursor(Qt::PointingHandCursor);     // 鼠标悬停变手型
        backendBtnLayout->addWidget(btn);
    }
    backendBtnLayout->addStretch();                 // 【弹性空间】将按钮推到左侧，右侧留白
    backendLayout->addLayout(backendBtnLayout);
    groupLayout->addLayout(backendLayout);
}

/**
 * @brief 设置高级参数区域
 *
 * 【视觉效果】在一个圆角卡片（#advancedCard）内，使用 2 列网格布局排列 6 组参数。
 * 每组参数包含：上方是参数名称标签（如"Temperature"），下方是对应的数值调节框。
 *
 * 【网格布局说明】
 * QGridLayout 按 (行, 列) 放置控件：
 * - 第0行：Temperature标签(0,0) | TopP标签(0,1)
 * - 第1行：Temperature调节框(1,0) | TopP调节框(1,1)
 * - 第2行：MaxTokens标签(2,0) | ContextLength标签(2,1)
 * - ...以此类推
 *
 * 【调试提示】
 * - 若卡片背景色不对，检查 applyTheme() 中 #advancedCard 的 background-color
 * - 若调节框太窄，修改 setFixedWidth(120) 为更大值如 160
 */
void ModelConfigPage::setupAdvancedSettings()
{
    m_advancedGroup = new QWidget(m_contentWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_advancedGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);    // 无内边距
    groupLayout->setSpacing(12);                     // 标题与卡片之间12像素间距

    // 【标题行】水平布局，标题在左，弹性空间在右（后续可扩展添加"?"帮助按钮）
    QHBoxLayout* titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(8);
    QLabel* title = new QLabel("高级参数", m_advancedGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");  // 15px加粗标题
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    groupLayout->addLayout(titleLayout);

    // 【参数卡片】圆角矩形背景容器，内部用网格排列参数
    QWidget* card = new QWidget(m_advancedGroup);
    card->setObjectName("advancedCard");            // QSS中 #advancedCard 设置背景色、边框、圆角
    QGridLayout* grid = new QGridLayout(card);
    grid->setContentsMargins(20, 20, 20, 20);       // 卡片内部四边各留20像素；改大则参数离边缘更远
    grid->setSpacing(16);                           // 网格单元之间16像素间距；改大则参数之间更松散

    // --- 第0-1行：Temperature & Top P ---
    QLabel* tempLabel = new QLabel("Temperature", card);
    tempLabel->setStyleSheet("font-size: 12px; font-weight: 500;"); // 12px参数名标签
    grid->addWidget(tempLabel, 0, 0);              // 放在第0行第0列

    m_temperatureSpin = new QDoubleSpinBox(card);   // 【浮点调节框】带上下箭头，可键盘输入
    m_temperatureSpin->setRange(0, 2);              // 最小值0，最大值2；改大如5则允许更高温度
    m_temperatureSpin->setSingleStep(0.1);          // 点击上下箭头每次增减0.1；改0.05则更精细
    m_temperatureSpin->setValue(0.7);               // 默认值0.7；常用值，平衡确定性和创造性
    m_temperatureSpin->setFixedWidth(120);          // 固定宽度120像素；改大则框变宽
    grid->addWidget(m_temperatureSpin, 1, 0);       // 放在第1行第0列

    QLabel* topPLabel = new QLabel("Top P", card);
    topPLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(topPLabel, 0, 1);               // 第0行第1列

    m_topPSpin = new QDoubleSpinBox(card);
    m_topPSpin->setRange(0, 1);                     // TopP范围0~1（概率值）
    m_topPSpin->setSingleStep(0.1);
    m_topPSpin->setValue(0.9);                      // 默认值0.9
    m_topPSpin->setFixedWidth(120);
    grid->addWidget(m_topPSpin, 1, 1);              // 第1行第1列

    // --- 第2-3行：Max Tokens & Context Length ---
    QLabel* maxTokensLabel = new QLabel("Max Tokens", card);
    maxTokensLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(maxTokensLabel, 2, 0);          // 第2行第0列

    m_maxTokensSpin = new QSpinBox(card);           // 【整数调节框】
    m_maxTokensSpin->setRange(1, 32768);            // 范围1~32768；改大可支持更长输出
    m_maxTokensSpin->setValue(4096);                // 默认4096；多数本地模型支持的最大输出长度
    m_maxTokensSpin->setFixedWidth(160);            // 比Temperature稍宽，因为数字可能更大
    grid->addWidget(m_maxTokensSpin, 3, 0);         // 第3行第0列

    QLabel* ctxLabel = new QLabel("Context Length", card);
    ctxLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(ctxLabel, 2, 1);                // 第2行第1列

    m_contextLengthSpin = new QSpinBox(card);
    m_contextLengthSpin->setRange(1, 32768);
    m_contextLengthSpin->setValue(8192);            // 默认8192；表示模型能记住8192个token的对话历史
    m_contextLengthSpin->setFixedWidth(160);
    grid->addWidget(m_contextLengthSpin, 3, 1);     // 第3行第1列

    // --- 第4-5行：GPU Memory & Parallel Threads ---
    QLabel* gpuLabel = new QLabel("GPU 显存限制", card);
    gpuLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(gpuLabel, 4, 0);                // 第4行第0列

    // 【GPU显存】带"GB"单位标签
    QHBoxLayout* gpuLayout = new QHBoxLayout();
    m_gpuMemorySpin = new QSpinBox(card);
    m_gpuMemorySpin->setRange(1, 128);              // 范围1~128 GB；根据实际显卡显存调整上限
    m_gpuMemorySpin->setValue(8);                   // 默认8GB；适合大多数中端显卡
    m_gpuMemorySpin->setFixedWidth(100);
    gpuLayout->addWidget(m_gpuMemorySpin);
    gpuLayout->addWidget(new QLabel("GB", card));   // 在单位数值右侧显示"GB"
    gpuLayout->addStretch();                        // 弹性空间，将内容推到左侧
    grid->addLayout(gpuLayout, 5, 0);               // 第5行第0列

    QLabel* threadLabel = new QLabel("并行线程数", card);
    threadLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(threadLabel, 4, 1);             // 第4行第1列

    m_parallelThreadsSpin = new QSpinBox(card);
    m_parallelThreadsSpin->setRange(1, 64);         // 范围1~64线程
    m_parallelThreadsSpin->setValue(4);             // 默认4线程；适合4核8线程CPU
    m_parallelThreadsSpin->setFixedWidth(120);
    grid->addWidget(m_parallelThreadsSpin, 5, 1);   // 第5行第1列

    groupLayout->addWidget(card);                   // 将参数卡片添加到高级参数区域
}

/**
 * @brief 设置已安装模型网格展示区域
 *
 * 【功能说明】创建"已安装的模型"标题和网格布局容器。
 * 注意：初始时 m_models 列表为空，因此首次调用时不会生成任何卡片。
 * 当测试连接成功后，populateInstalledModels() 会重新填充列表并调用 updateModelGrid() 生成卡片。
 *
 * 【网格参数】
 * - grid->setSpacing(12)：卡片之间12像素间距；改大则卡片之间空隙更大
 */
void ModelConfigPage::setupModelGrid()
{
    m_installedModelsGroup = new QWidget(m_contentWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_installedModelsGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(16);                    // 标题与网格之间16像素间距

    QLabel* title = new QLabel("已安装的模型", m_installedModelsGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");
    groupLayout->addWidget(title);

    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(12);                           // 卡片行列间距12像素

    // 初始时没有已安装模型，测试连接成功后动态填充
    m_models.clear();                               // 清空模型列表

    // 初始状态 m_models 为空，此循环不会执行任何内容
    for (int i = 0; i < m_models.size(); ++i) {
        // ...（此处代码在初始时不会执行，实际卡片生成在 updateModelGrid() 中）
    }

    groupLayout->addLayout(grid);
}

/**
 * @brief 应用当前主题样式
 *
 * 【QSS样式表详解】
 * QSS（Qt StyleSheet）是一种类似 CSS 的样式语言，用于控制 Qt 控件的外观。
 * 本函数从 ThemeManager 获取当前主题的颜色值，拼接成一长串 QSS 字符串，
 * 然后通过 m_contentWidget->setStyleSheet() 应用到整个内容区。
 *
 * 【颜色变量说明】（以下都是 QString 类型的颜色十六进制值，如 "#0F172A"）
 * - bg: 主背景色（暗色=#0F172A，亮色=#FFFFFF）
 * - bg2: 次级背景色（暗色=#1E293B，亮色=#F8FAFC）
 * - bg3: 第三级背景色（暗色=#334155，亮色=#F1F5F9）
 * - surf: 表面/卡片背景色（暗色=#1E293B，亮色=#FFFFFF）
 * - bdr: 边框色（暗色=#334155，亮色=#E2E8F0）
 * - pri: 主色调/品牌色（暗色=#3B82F6蓝色，亮色=#2563EB）
 * - txtPri: 主要文字色（暗色=#F1F5F9浅灰白，亮色=#0F172A深灰）
 * - txtSec: 次要文字色（暗色=#94A3B8，亮色=#475569）
 * - txtTer: 第三级文字色（暗色=#64748B，亮色=#94A3B8）
 * - suc: 成功色绿色（=#22C55E）
 * - txtTer/sucBg: 成功背景色（=#052E16深绿）
 *
 * 【各控件样式效果】
 * - #configContent：内容区背景色；改 bg 值可改变整个页面底色
 * - #connectionCard：连接状态卡片背景+边框+圆角；改 border-radius 可调整圆角大小
 * - #statusDot：状态圆点；border-radius:5px 配合 10×10 尺寸形成正圆
 * - #formInput：输入框样式；border:1px solid 控制边框粗细和颜色
 * - #primaryButton：主按钮蓝色背景；padding 控制按钮内部空间大小
 * - #ghostButton：幽灵按钮透明背景；hover 时显示浅色背景
 */
void ModelConfigPage::applyTheme()
{
    ThemeManager* tm = ThemeManager::instance();    // 【获取单例】主题管理器全局唯一实例
    QString bg = tm->background().name();           // 主背景色，如 "#0F172A"
    QString bg2 = tm->bgSecondary().name();         // 次级背景色，如 "#1E293B"
    QString bg3 = tm->bgTertiary().name();          // 第三级背景色，如 "#334155"
    QString surf = tm->surface().name();            // 表面色，如 "#1E293B"
    QString bdr = tm->border().name();              // 边框色，如 "#334155"
    QString pri = tm->primary().name();             // 主色调，如 "#3B82F6"（蓝色）
    QString txtPri = tm->textPrimary().name();      // 主文字色，如 "#F1F5F9"
    QString txtSec = tm->textSecondary().name();    // 次要文字色，如 "#94A3B8"
    QString txtTer = tm->textTertiary().name();     // 第三级文字色，如 "#64748B"
    QString sucBg = tm->stateSuccessBg().name();    // 成功背景色，如 "#052E16"
    QString suc = tm->stateSuccess().name();        // 成功色，如 "#22C55E"

    // 【设置内容区样式表】
    // QString::arg() 按顺序替换 %1, %2, %3... 占位符为实际颜色值
    m_contentWidget->setStyleSheet(QString(
        // 内容区背景
        "QWidget#configContent { background-color: %1; }"
        // 连接状态卡片：背景色+边框+12px大圆角
        "QWidget#connectionCard { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        // 状态标题文字颜色
        "QLabel#statusTitle { color: %8; }"
        // 状态详情文字颜色（使用第三级文字色，较淡）
        "QLabel#statusDetail { color: %10; }"
        // 状态圆点：圆形（10×10 + border-radius:5px）+ 成功色背景
        "QLabel#statusDot { background-color: %9; border-radius: 5px; }"
        // 高级参数卡片：与连接卡片类似样式
        "QWidget#advancedCard { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        // 模型卡片：表面色背景+8px圆角+边框
        "QWidget#modelCard { background-color: %5; border: 1px solid %4; border-radius: 8px; }"
        // 模型卡片悬停效果：边框变为主色调蓝色，提示用户可点击
        "QWidget#modelCard:hover { border-color: %6; }"
        // 表单输入框：背景色+文字色+边框+8px圆角+内边距
        "QLineEdit#formInput { background-color: %5; color: %8; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; font-size: 13px; }"
        // 输入框获得焦点时：边框变蓝色，提示当前正在编辑
        "QLineEdit#formInput:focus { border-color: %6; }"
        // 下拉选择框：样式与输入框类似
        "QComboBox#formSelect { background-color: %5; color: %8; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; font-size: 13px; }"
        "QComboBox#formSelect:focus { border-color: %6; }"
        // 主按钮：蓝色背景+白色文字+8px圆角；用于"保存配置"等核心操作
        "QPushButton#primaryButton { background-color: %6; color: white; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"
        // 主按钮悬停：背景变为次要文字色（稍浅的蓝色）
        "QPushButton#primaryButton:hover { background-color: %7; }"
        // 次要按钮：表面色背景+边框；用于"测试连接"
        "QPushButton#secondaryButton { background-color: %5; color: %8; border: 1px solid %4; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"
        "QPushButton#secondaryButton:hover { background-color: %3; }"
        // 幽灵按钮：透明背景+灰色文字；用于"恢复默认"
        "QPushButton#ghostButton { background-color: transparent; color: %10; border: none; border-radius: 8px; padding: 8px 16px; font-weight: 500; }"
        "QPushButton#ghostButton:hover { background-color: %3; color: %8; }"
        // 图标按钮：24×24小按钮，用于模型卡片右上角"更多"操作
        "QPushButton#iconButton { background-color: transparent; color: %10; border: none; border-radius: 4px; }"
        "QPushButton#iconButton:hover { background-color: %3; }"
        // 药丸按钮（单选按钮）：未选中时灰色，选中时蓝色
        "QRadioButton#pillButton { color: %10; }"
        "QRadioButton#pillButton:checked { color: %6; }"
        // 区域标题颜色
        "QLabel#sectionTitle { color: %8; }"
        // 所有 QLabel 默认文字颜色
        "QLabel { color: %8; }"
    ).arg(bg, bg2, bg3, bdr, surf, pri, txtSec, txtPri, suc, txtTer, sucBg));
    // arg() 参数顺序：%1=bg, %2=bg2, %3=bg3, %4=bdr, %5=surf, %6=pri, %7=txtSec, %8=txtPri, %9=suc, %10=txtTer, %11=sucBg
}

/**
 * @brief 获取当前 API 地址
 * @return API URL 字符串
 */
QString ModelConfigPage::apiUrl() const
{
    return m_apiUrlEdit->text();    // 返回用户输入框中的文本内容
}

/**
 * @brief 获取当前模型名称
 * @return 模型名称字符串
 *
 * 【获取逻辑】优先使用下拉选择框（如果已启用且有选中项），否则使用手动输入框。
 * 这样允许用户从列表选择，也可以自由输入列表中没有的模型。
 */
QString ModelConfigPage::modelName() const
{
    // 如果下拉框已启用（测试连接成功后 setEnabled(true)）且当前选中的不是第0个提示项
    if (m_modelCombo->isEnabled() && m_modelCombo->currentIndex() > 0) {
        return m_modelCombo->currentText();     // 返回下拉框当前选中的模型名称
    }
    return m_modelNameEdit->text().trimmed();   // 返回手动输入框内容，并去除首尾空白字符
}

/**
 * @brief 动态填充已安装模型列表
 * @param modelNames 从 API 获取到的模型名称列表
 *
 * 【调用场景】主窗口收到网络连接测试成功的结果后，将服务器返回的模型名称列表传入此处。
 * 【执行动作】
 * 1. 清空下拉框原有内容
 * 2. 添加提示项"-- 选择已安装模型 --"
 * 3. 逐个添加模型名称
 * 4. 启用下拉框（之前是禁用的灰色状态）
 * 5. 自动选中第一个真实模型，并同步到手动输入框
 * 6. 调用 updateModelGrid() 刷新下方卡片网格
 */
void ModelConfigPage::populateInstalledModels(const QStringList& modelNames)
{
    m_modelCombo->clear();                          // 清空原有下拉项
    m_modelCombo->addItem("-- 选择已安装模型 --");   // 添加默认提示项（索引0）
    for (const QString& name : modelNames) {
        m_modelCombo->addItem(name);                // 逐个添加模型名称到下拉框
    }
    m_modelCombo->setEnabled(true);                 // 【启用】按钮从灰色变为可用状态

    // 如果列表非空，自动选中第一个真实模型（索引1，因为索引0是提示文字）
    if (!modelNames.isEmpty()) {
        m_modelCombo->setCurrentIndex(1);           // 自动选择第一个模型
        m_modelNameEdit->setText(modelNames.first()); // 将第一个模型名称同步到手动输入框
    }

    updateModelGrid(modelNames);                    // 刷新下方模型卡片网格显示
}

/**
 * @brief 更新已安装模型网格
 * @param modelNames 模型名称列表
 *
 * 【视觉效果】根据传入的模型名称生成卡片网格，每行最多3张卡片。
 * 每张卡片显示：模型名称（14px加粗）、大小、类型、状态徽章（彩色胶囊标签）。
 *
 * 【实现细节】
 * - 先清空旧布局（删除所有旧卡片控件）
 * - 重新创建 QVBoxLayout + QGridLayout
 * - 为每个模型创建一张卡片，使用 m_models[i] 的数据
 * - 安装事件过滤器，使卡片可点击
 */
void ModelConfigPage::updateModelGrid(const QStringList& modelNames)
{
    // 【填充模型数据】根据传入的名称列表构建内部 ModelInfo 结构
    m_models.clear();
    for (const QString& name : modelNames) {
        ModelInfo info;
        info.name = name;
        info.size = "未知";     // 默认大小未知（服务器未提供时可扩展）
        // 【智能判断模型类型】根据名称关键词推断用途
        info.type = name.contains("Coder") ? "代码生成" : (name.contains("Math") ? "数学推理" : "通用对话");
        info.status = "已安装"; // 默认状态
        info.isRunning = (name == m_modelNameEdit->text()); // 如果名称与当前输入框一致，标记为运行中
        m_models.append(info);
    }

    // 【清理旧布局】先删除 m_installedModelsGroup 中原有的所有子控件和布局
    QLayout* oldLayout = m_installedModelsGroup->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {  // 逐个取出布局项
            delete item->widget();                          // 删除布局项中的控件（如旧卡片）
            delete item;                                    // 删除布局项本身
        }
        delete oldLayout;                                   // 删除旧布局对象
    }

    // 【重建布局】
    QVBoxLayout* groupLayout = new QVBoxLayout(m_installedModelsGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(16);

    QLabel* title = new QLabel("已安装的模型", m_installedModelsGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");
    groupLayout->addWidget(title);

    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(12);

    // 【为每个模型创建卡片】
    for (int i = 0; i < m_models.size(); ++i) {
        QWidget* card = new QWidget(m_installedModelsGroup);
        card->setObjectName("modelCard");               // QSS中 #modelCard 控制卡片外观
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16); // 卡片内部四边16像素内边距；改大则内容与边缘距离增大
        cardLayout->setSpacing(6);                      // 卡片内各元素之间6像素间距

        // 【卡片头部】水平布局：名称（左）+ 更多按钮（右）
        QHBoxLayout* header = new QHBoxLayout();
        QLabel* name = new QLabel(m_models[i].name, card);
        name->setStyleSheet("font-size: 14px; font-weight: 600;");  // 14px加粗模型名
        header->addWidget(name);
        header->addStretch();                           // 弹性空间将名称推到左侧

        // 【更多按钮】竖三点符号（⋮），24×24像素小按钮
        QPushButton* moreBtn = new QPushButton("\u22EE", card);  // \u22EE 是 Unicode 竖三点字符
        moreBtn->setObjectName("iconButton");
        moreBtn->setFixedSize(24, 24);                  // 固定24×24像素；改大则按钮变大
        moreBtn->setCursor(Qt::PointingHandCursor);
        header->addWidget(moreBtn);
        cardLayout->addLayout(header);

        // 【辅助lambda】添加标签-值行；isStatus=true 时使用彩色徽章样式
        auto addRow = [&](const QString& label, const QString& value, bool isStatus = false) {
            QHBoxLayout* row = new QHBoxLayout();
            QLabel* l = new QLabel(label, card);
            l->setStyleSheet("font-size: 12px;");       // 标签如"大小"、"类型"使用12px字
            row->addWidget(l);
            row->addStretch();                          // 将标签推到左侧，值/徽章推到右侧
            if (isStatus) {
                // 【状态徽章】根据状态值选择不同颜色
                QLabel* badge = new QLabel(value, card);
                // 三元条件：运行中=绿色(#22C55E)，已停止=灰色(#94A3B8)，其他=蓝色(#3B82F6)
                QString color = value == "运行中" ? "#22C55E" : (value == "已停止" ? "#94A3B8" : "#3B82F6");
                QString bg = value == "运行中" ? "rgba(34, 197, 94, 0.1)" : (value == "已停止" ? "rgba(148, 163, 184, 0.1)" : "rgba(59, 130, 246, 0.1)");
                badge->setStyleSheet(QString("background: %1; color: %2; padding: 2px 8px; border-radius: 999px; font-size: 11px; font-weight: 500;").arg(bg, color));
                // 徽章样式说明：
                // - padding: 2px 8px → 上下2、左右8像素内边距；改大则徽章变高变宽
                // - border-radius: 999px → 胶囊形状；改小则变成圆角矩形
                row->addWidget(badge);
            } else {
                QLabel* v = new QLabel(value, card);
                v->setStyleSheet("font-size: 12px; font-weight: 500;");  // 值使用稍粗字体
                row->addWidget(v);
            }
            cardLayout->addLayout(row);
        };

        // 添加大小、类型、状态三行
        addRow("大小", m_models[i].size);
        addRow("类型", m_models[i].type);
        addRow("状态", m_models[i].status, true);       // true 表示使用彩色徽章样式

        // 将卡片放入网格：行号=i/3（每行3列），列号=i%3
        grid->addWidget(card, i / 3, i % 3);

        // 【安装事件过滤器】使卡片可响应鼠标点击
        card->setProperty("modelIndex", i);             // 将模型索引存储到控件属性中，点击时识别
        card->installEventFilter(this);                 // 让本页面接收卡片的事件（如鼠标点击）
        card->setCursor(Qt::PointingHandCursor);        // 鼠标悬停变手型，提示可点击
    }

    groupLayout->addLayout(grid);
}

/**
 * @brief 获取已安装模型名称列表
 * @return 模型名称字符串列表
 */
QStringList ModelConfigPage::installedModelNames() const
{
    QStringList names;
    for (const auto& model : m_models) {
        names.append(model.name);
    }
    return names;
}

/**
 * @brief 更新连接状态显示
 * @param connected true=连接成功, false=未连接
 * @param detail 模型详情文本（连接成功时显示）
 *
 * 【视觉效果详解】
 * 连接成功时：
 * - 状态标签文字变"模型已连接"，颜色变绿色(#22C55E)
 * - 详情标签显示模型名称 + "· 本地部署"
 * - 徽章变绿色"已连接"，背景为淡绿色半透明
 * - 圆点变绿色
 *
 * 连接失败/未连接时：
 * - 状态标签文字变"未连接"或传入的错误信息，颜色变灰色(#94A3B8)
 * - 详情标签恢复默认提示
 * - 徽章变灰色"未连接"
 * - 圆点变灰色
 */
void ModelConfigPage::updateConnectionStatus(bool connected, const QString& detail)
{
    if (connected) {
        m_statusLabel->setText("模型已连接");
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500; color: #22C55E;");
        // color: #22C55E → 绿色；改 #EF4444 则变红色，改 #3B82F6 则变蓝色
        m_statusDetail->setText(detail + " · 本地部署");

        m_connectedBadge->setText("已连接");
        m_connectedBadge->setStyleSheet(
            "background: rgba(34, 197, 94, 0.1); color: #22C55E; "
            "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
            // rgba(34,197,94,0.1) = 淡绿半透明背景；#22C55E = 绿字
        );

        m_statusDot->setStyleSheet(
            "background: #22C55E; border-radius: 5px;"
            // 10×10 像素 + 5px 圆角 = 正圆绿色点
        );
    } else {
        m_statusLabel->setText(detail.isEmpty() ? "未连接" : detail);
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500; color: #94A3B8;");
        // #94A3B8 = 灰蓝色；用于未连接状态的弱提示
        m_statusDetail->setText("请配置模型端点并测试连接");

        m_connectedBadge->setText("未连接");
        m_connectedBadge->setStyleSheet(
            "background: rgba(148, 163, 184, 0.1); color: #94A3B8; "
            "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
        );

        m_statusDot->setStyleSheet(
            "background: #94A3B8; border-radius: 5px;"
            // 灰色圆点
        );
    }
}

/**
 * @brief 获取 Max Tokens 设置值
 * @return 最大生成 Token 数
 */
int ModelConfigPage::maxTokens() const
{
    return m_maxTokensSpin->value();    // 返回调节框当前数值
}

/**
 * @brief 获取 Temperature 设置值
 * @return 温度参数值
 */
double ModelConfigPage::temperature() const
{
    return m_temperatureSpin->value();
}

/**
 * @brief 获取 Top P 设置值
 * @return Top P 参数值
 */
double ModelConfigPage::topP() const
{
    return m_topPSpin->value();
}

/**
 * @brief 获取 Context Length 设置值
 * @return 上下文长度
 */
int ModelConfigPage::contextLength() const
{
    return m_contextLengthSpin->value();
}

/**
 * @brief 获取 GPU 显存限制设置值
 * @return 显存限制（GB）
 */
int ModelConfigPage::gpuMemoryLimit() const
{
    return m_gpuMemorySpin->value();
}

/**
 * @brief 获取并行线程数设置值
 * @return 线程数
 */
int ModelConfigPage::parallelThreads() const
{
    return m_parallelThreadsSpin->value();
}

/**
 * @brief 获取当前选中的推理后端
 * @return 后端名称字符串（"CPU"/"CUDA"/"Metal"）
 *
 * 【逻辑】通过 QButtonGroup 的 checkedId() 获取当前选中单选按钮的ID：
 * - ID 0 → "CPU"
 * - ID 1 → "CUDA"
 * - ID 2 → "Metal"
 */
QString ModelConfigPage::backend() const
{
    int id = m_backendGroup->checkedId();
    return id == 0 ? "CPU" : (id == 1 ? "CUDA" : "Metal");
}

/**
 * @brief 设置测试连接按钮的可用状态
 * @param enabled true=可用, false=禁用
 */
void ModelConfigPage::setTestButtonEnabled(bool enabled)
{
    m_testConnectionBtn->setEnabled(enabled);   // true时按钮正常显示可点击；false时按钮变灰不可点击
}

/**
 * @brief 设置 API 地址
 * @param url API URL 字符串
 */
void ModelConfigPage::setApiUrl(const QString& url)
{
    m_apiUrlEdit->setText(url);     // 将字符串填入 API 地址输入框
}

/**
 * @brief 设置模型名称
 * @param name 模型名称字符串
 *
 * 【逻辑】优先在下拉框中查找匹配项；找不到则填入手动输入框。
 */
void ModelConfigPage::setModelName(const QString& name)
{
    int idx = m_modelCombo->findText(name);     // 在下拉框中搜索匹配文字
    if (idx > 0) {                              // idx>0 表示找到了（idx=0是提示项"-- 选择已安装模型 --"）
        m_modelCombo->setCurrentIndex(idx);     // 设置下拉框选中该项
    } else {
        m_modelNameEdit->setText(name);         // 没找到则填入手动输入框
    }
}

/**
 * @brief 设置 Max Tokens
 * @param tokens 最大生成 Token 数
 */
void ModelConfigPage::setMaxTokens(int tokens)
{
    m_maxTokensSpin->setValue(tokens);  // 设置调节框数值
}

/**
 * @brief 设置 Temperature
 * @param temp 温度参数值
 */
void ModelConfigPage::setTemperature(double temp)
{
    m_temperatureSpin->setValue(temp);
}

/**
 * @brief 保存配置按钮点击槽函数
 *
 * 【信号发射】当用户点击"保存配置"按钮时，此槽函数被调用，
 * 发射 settingsChanged() 信号通知主窗口保存当前设置。
 */
void ModelConfigPage::onSaveClicked()
{
    emit settingsChanged();     // 【emit关键字】发射信号，通知所有连接到此信号的接收者
}

/**
 * @brief 测试连接按钮点击槽函数
 *
 * 【信号发射】通知主窗口需要进行网络连接测试。
 * 主窗口收到此信号后，会读取 apiUrl() 和 modelName()，然后发送 HTTP 请求测试连通性。
 */
void ModelConfigPage::onTestConnectionClicked()
{
    emit testConnectionClicked();
}

/**
 * @brief 恢复默认设置按钮点击槽函数
 *
 * 【重置逻辑】将所有输入控件恢复为程序预设的默认值：
 * - API 地址清空
 * - 模型名称清空
 * - 下拉框重置为禁用状态并显示提示项
 * - 高级参数恢复默认值
 * - 连接状态重置为未连接
 * 然后发射 restoreDefaultsClicked() 信号通知外部。
 */
void ModelConfigPage::onRestoreDefaultsClicked()
{
    m_apiUrlEdit->clear();                          // 清空 API 地址输入框
    m_modelNameEdit->clear();                       // 清空模型名称输入框
    m_modelCombo->clear();                          // 清空下拉框
    m_modelCombo->addItem("-- 请先测试连接 --");   // 恢复默认提示项
    m_modelCombo->setEnabled(false);                // 禁用下拉框（变灰色）
    m_temperatureSpin->setValue(0.7);               // Temperature 恢复默认值 0.7
    m_topPSpin->setValue(0.9);                      // TopP 恢复默认值 0.9
    m_maxTokensSpin->setValue(4096);                // MaxTokens 恢复默认值 4096
    m_contextLengthSpin->setValue(8192);            // 上下文长度恢复默认值 8192
    m_gpuMemorySpin->setValue(8);                   // GPU 显存恢复默认值 8GB
    m_parallelThreadsSpin->setValue(4);             // 并行线程恢复默认值 4
    updateConnectionStatus(false);                  // 重置连接状态为"未连接"
    emit restoreDefaultsClicked();                  // 发射信号通知主窗口
}

/**
 * @brief 后端选择变化槽函数（当前为占位实现）
 * @param button 被选中的单选按钮
 *
 * 【说明】当前未实现额外逻辑，保留接口以便后续扩展（如切换后端时显示对应提示）。
 */
void ModelConfigPage::onBackendSelected(QAbstractButton* button)
{
    Q_UNUSED(button)    // 【Q_UNUSED宏】标记参数未使用，避免编译器警告
}

/**
 * @brief 主题变化响应槽函数
 *
 * 【触发时机】当 ThemeManager 发射 themeChanged 信号时，此槽函数被自动调用。
 * 【执行动作】重新调用 applyTheme()，从 ThemeManager 获取新的颜色值并应用。
 */
void ModelConfigPage::onThemeChanged()
{
    applyTheme();
}

/**
 * @brief 事件过滤器
 * @param obj 发生事件的对象
 * @param event 事件对象
 * @return true=事件已处理, false=继续传递
 *
 * 【用途】拦截模型卡片的鼠标点击事件。
 * 当用户点击某个模型卡片时，自动将该模型的名称填入上方输入框和下拉框。
 * 这样用户可以直接点击卡片选择模型，无需手动输入或从下拉框选择。
 */
bool ModelConfigPage::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {    // 只处理鼠标按下事件
        QWidget *w = qobject_cast<QWidget*>(obj);       // 将事件对象转换为 QWidget 指针
        if (w && w->objectName() == "modelCard") {      // 确认被点击的是模型卡片
            bool ok;
            int index = w->property("modelIndex").toInt(&ok);   // 读取卡片上存储的模型索引
            if (ok && index >= 0 && index < m_models.size()) {
                m_modelNameEdit->setText(m_models[index].name);     // 将模型名称填入手动输入框
                m_modelCombo->setCurrentText(m_models[index].name); // 同步下拉框选中该项
                return true;    // 返回true表示事件已处理，不再继续传递
            }
        }
    }
    return QWidget::eventFilter(obj, event);    // 其他事件交给父类默认处理
}
