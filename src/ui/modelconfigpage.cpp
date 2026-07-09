/**
 * @file modelconfigpage.cpp
 * @brief 模型配置页面实现
 *
 * @details
 * ModelConfigPage提供本地大语言模型的连接配置、参数调节及已安装模型管理功能。
 * 页面采用滚动布局，包含连接状态卡片、模型设置、高级参数网格、操作按钮及模型卡片网格。
 */

#include "modelconfigpage.h"
#include "thememanager.h"
#include "iconhelper.h"
#include <QButtonGroup>
#include <QRadioButton>
#include <QFormLayout>
#include <QFrame>
#include <QFileDialog>

/**
 * @brief 构造函数
 * @param parent 父QWidget
 *
 * 初始化UI、应用主题并连接ThemeManager主题变化信号。
 */
ModelConfigPage::ModelConfigPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyTheme();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ModelConfigPage::onThemeChanged);
}

/**
 * @brief 析构函数
 */
ModelConfigPage::~ModelConfigPage()
{
}

/**
 * @brief 设置模型配置页面整体UI布局
 *
 * 采用QScrollArea承载垂直内容布局，依次添加：
 * 连接状态卡片、模型设置组、高级参数组、底部操作栏、已安装模型网格。
 * 确保内容超出可视区域时可滚动查看。
 */
void ModelConfigPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentWidget->setObjectName("configContent");
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(32, 32, 32, 32);
    contentLayout->setSpacing(0);

    setupConnectionCard();
    setupModelSettings();
    setupAdvancedSettings();
    setupModelGrid();

    contentLayout->addWidget(m_connectionCard);
    contentLayout->addSpacing(28);
    contentLayout->addWidget(m_modelSettingsGroup);
    contentLayout->addSpacing(28);
    contentLayout->addWidget(m_advancedGroup);
    contentLayout->addSpacing(28);

    // 底部操作栏：恢复默认 | 保存配置
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setContentsMargins(0, 20, 0, 36);
    actionLayout->setSpacing(12);

    m_restoreBtn = new QPushButton("恢复默认", m_contentWidget);
    m_restoreBtn->setObjectName("ghostButton");
    m_restoreBtn->setCursor(Qt::PointingHandCursor);
    connect(m_restoreBtn, &QPushButton::clicked, this, &ModelConfigPage::onRestoreDefaultsClicked);
    actionLayout->addWidget(m_restoreBtn);

    actionLayout->addStretch();

    m_saveBtn = new QPushButton("保存配置", m_contentWidget);
    m_saveBtn->setObjectName("primaryButton");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, &ModelConfigPage::onSaveClicked);
    actionLayout->addWidget(m_saveBtn);

    contentLayout->addLayout(actionLayout);
    contentLayout->addSpacing(24);
    contentLayout->addWidget(m_installedModelsGroup);
    contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);
}

/**
 * @brief 设置连接状态卡片
 *
 * 构建水平布局卡片，左侧展示连接状态指示点与描述文本（模型名称+部署方式），
 * 右侧放置"测试连接"按钮。
 */
void ModelConfigPage::setupConnectionCard()
{
    m_connectionCard = new QWidget(m_contentWidget);
    m_connectionCard->setObjectName("connectionCard");
    QHBoxLayout* cardLayout = new QHBoxLayout(m_connectionCard);
    cardLayout->setContentsMargins(20, 16, 20, 16);
    cardLayout->setSpacing(12);

    QWidget* statusWidget = new QWidget(m_connectionCard);
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(12);

    // 状态指示圆点
    m_statusDot = new QLabel(statusWidget);
    m_statusDot->setFixedSize(10, 10);
    m_statusDot->setObjectName("statusDot");
    statusLayout->addWidget(m_statusDot);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    m_statusLabel = new QLabel("未连接", statusWidget);
    m_statusLabel->setObjectName("statusTitle");
    m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500;");
    textLayout->addWidget(m_statusLabel);

    m_statusDetail = new QLabel("请配置模型端点并测试连接", statusWidget);
    m_statusDetail->setObjectName("statusDetail");
    m_statusDetail->setStyleSheet("font-size: 12px;");
    textLayout->addWidget(m_statusDetail);
    statusLayout->addLayout(textLayout);

    cardLayout->addWidget(statusWidget);
    cardLayout->addStretch();

    m_testConnectionBtn = new QPushButton("测试连接", m_connectionCard);
    m_testConnectionBtn->setObjectName("secondaryButton");
    m_testConnectionBtn->setCursor(Qt::PointingHandCursor);
    connect(m_testConnectionBtn, &QPushButton::clicked, this, &ModelConfigPage::onTestConnectionClicked);
    cardLayout->addWidget(m_testConnectionBtn);
}

/**
 * @brief 设置模型基础设置区域
 *
 * 包含三个配置项：
 * - 模型端点（API URL输入框+已连接徽章）
 * - 模型名称（下拉选择框，预置常用本地模型）
 * - 推理后端（CPU/CUDA/Metal单选按钮组）
 */
void ModelConfigPage::setupModelSettings()
{
    m_modelSettingsGroup = new QWidget(m_contentWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_modelSettingsGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(20);

    QLabel* title = new QLabel("模型设置", m_modelSettingsGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");
    groupLayout->addWidget(title);

    // 模型端点输入
    QVBoxLayout* endpointLayout = new QVBoxLayout();
    endpointLayout->setSpacing(6);
    QLabel* endpointLabel = new QLabel("模型端点 (Endpoint)", m_modelSettingsGroup);
    endpointLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    endpointLayout->addWidget(endpointLabel);

    QLabel* endpointHint = new QLabel("本地大模型的 API 服务地址", m_modelSettingsGroup);
    endpointHint->setStyleSheet("font-size: 12px;");
    endpointLayout->addWidget(endpointHint);

    QHBoxLayout* endpointInputLayout = new QHBoxLayout();
    endpointInputLayout->setSpacing(8);
    m_apiUrlEdit = new QLineEdit(m_modelSettingsGroup);
    m_apiUrlEdit->setObjectName("formInput");
    m_apiUrlEdit->setPlaceholderText("例如: http://localhost:8080/v1");
    endpointInputLayout->addWidget(m_apiUrlEdit, 1);

    QLabel* connectedBadge = new QLabel("未连接", m_modelSettingsGroup);
    connectedBadge->setObjectName("successBadge");
    connectedBadge->setStyleSheet(
        "background: rgba(148, 163, 184, 0.1); color: #94A3B8; "
        "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
    );
    m_connectedBadge = connectedBadge;
    endpointInputLayout->addWidget(connectedBadge);
    endpointLayout->addLayout(endpointInputLayout);
    groupLayout->addLayout(endpointLayout);

    // 模型名称手动输入
    QVBoxLayout* modelLayout = new QVBoxLayout();
    modelLayout->setSpacing(6);
    QLabel* modelLabel = new QLabel("模型名称", m_modelSettingsGroup);
    modelLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    modelLayout->addWidget(modelLabel);

    QLabel* modelHint = new QLabel("请手动输入模型名称，测试连接通过后可从已安装模型中选择", m_modelSettingsGroup);
    modelHint->setStyleSheet("font-size: 12px;");
    modelLayout->addWidget(modelHint);

    m_modelNameEdit = new QLineEdit(m_modelSettingsGroup);
    m_modelNameEdit->setObjectName("formInput");
    m_modelNameEdit->setPlaceholderText("例如: Qwen-7B-Instruct");
    m_modelNameEdit->setFixedWidth(360);
    modelLayout->addWidget(m_modelNameEdit);
    groupLayout->addLayout(modelLayout);

    // 已安装的模型下拉框（测试连接后填充）
    QVBoxLayout* installedLayout = new QVBoxLayout();
    installedLayout->setSpacing(6);
    QLabel* installedLabel = new QLabel("已安装的模型", m_modelSettingsGroup);
    installedLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    installedLayout->addWidget(installedLabel);

    QLabel* installedHint = new QLabel("测试连接成功后将从服务器获取已安装模型列表", m_modelSettingsGroup);
    installedHint->setStyleSheet("font-size: 12px;");
    installedLayout->addWidget(installedHint);

    m_modelCombo = new QComboBox(m_modelSettingsGroup);
    m_modelCombo->setObjectName("formSelect");
    m_modelCombo->addItem("-- 请先测试连接 --");
    m_modelCombo->setFixedWidth(360);
    m_modelCombo->setEnabled(false);
    installedLayout->addWidget(m_modelCombo);
    groupLayout->addLayout(installedLayout);

    // 推理后端选择
    QVBoxLayout* backendLayout = new QVBoxLayout();
    backendLayout->setSpacing(8);
    QLabel* backendLabel = new QLabel("推理后端", m_modelSettingsGroup);
    backendLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    backendLayout->addWidget(backendLabel);

    m_backendGroup = new QButtonGroup(m_modelSettingsGroup);
    QHBoxLayout* backendBtnLayout = new QHBoxLayout();
    backendBtnLayout->setSpacing(6);

    QRadioButton* cpuBtn = new QRadioButton("CPU", m_modelSettingsGroup);
    QRadioButton* cudaBtn = new QRadioButton("CUDA (GPU)", m_modelSettingsGroup);
    QRadioButton* metalBtn = new QRadioButton("Metal (Mac)", m_modelSettingsGroup);
    cudaBtn->setChecked(true);

    m_backendGroup->addButton(cpuBtn, 0);
    m_backendGroup->addButton(cudaBtn, 1);
    m_backendGroup->addButton(metalBtn, 2);

    for (QAbstractButton* btn : m_backendGroup->buttons()) {
        btn->setObjectName("pillButton");
        btn->setCursor(Qt::PointingHandCursor);
        backendBtnLayout->addWidget(btn);
    }
    backendBtnLayout->addStretch();
    backendLayout->addLayout(backendBtnLayout);
    groupLayout->addLayout(backendLayout);
}

/**
 * @brief 设置高级参数区域
 *
 * 在QGridLayout卡片中排列六组可调参数：
 * Temperature、Top P、Max Tokens、Context Length、GPU显存限制、并行线程数。
 */
void ModelConfigPage::setupAdvancedSettings()
{
    m_advancedGroup = new QWidget(m_contentWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_advancedGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(12);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(8);
    QLabel* title = new QLabel("高级参数", m_advancedGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    groupLayout->addLayout(titleLayout);

    QWidget* card = new QWidget(m_advancedGroup);
    card->setObjectName("advancedCard");
    QGridLayout* grid = new QGridLayout(card);
    grid->setContentsMargins(20, 20, 20, 20);
    grid->setSpacing(16);

    // Row 0-1: Temperature & Top P
    QLabel* tempLabel = new QLabel("Temperature", card);
    tempLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(tempLabel, 0, 0);

    m_temperatureSpin = new QDoubleSpinBox(card);
    m_temperatureSpin->setRange(0, 2);
    m_temperatureSpin->setSingleStep(0.1);
    m_temperatureSpin->setValue(0.7);
    m_temperatureSpin->setFixedWidth(120);
    grid->addWidget(m_temperatureSpin, 1, 0);

    QLabel* topPLabel = new QLabel("Top P", card);
    topPLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(topPLabel, 0, 1);

    m_topPSpin = new QDoubleSpinBox(card);
    m_topPSpin->setRange(0, 1);
    m_topPSpin->setSingleStep(0.1);
    m_topPSpin->setValue(0.9);
    m_topPSpin->setFixedWidth(120);
    grid->addWidget(m_topPSpin, 1, 1);

    // Row 2-3: Max Tokens & Context Length
    QLabel* maxTokensLabel = new QLabel("Max Tokens", card);
    maxTokensLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(maxTokensLabel, 2, 0);

    m_maxTokensSpin = new QSpinBox(card);
    m_maxTokensSpin->setRange(1, 32768);
    m_maxTokensSpin->setValue(4096);
    m_maxTokensSpin->setFixedWidth(160);
    grid->addWidget(m_maxTokensSpin, 3, 0);

    QLabel* ctxLabel = new QLabel("Context Length", card);
    ctxLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(ctxLabel, 2, 1);

    m_contextLengthSpin = new QSpinBox(card);
    m_contextLengthSpin->setRange(1, 32768);
    m_contextLengthSpin->setValue(8192);
    m_contextLengthSpin->setFixedWidth(160);
    grid->addWidget(m_contextLengthSpin, 3, 1);

    // Row 4-5: GPU Memory & Parallel Threads
    QLabel* gpuLabel = new QLabel("GPU 显存限制", card);
    gpuLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(gpuLabel, 4, 0);

    QHBoxLayout* gpuLayout = new QHBoxLayout();
    m_gpuMemorySpin = new QSpinBox(card);
    m_gpuMemorySpin->setRange(1, 128);
    m_gpuMemorySpin->setValue(8);
    m_gpuMemorySpin->setFixedWidth(100);
    gpuLayout->addWidget(m_gpuMemorySpin);
    gpuLayout->addWidget(new QLabel("GB", card));
    gpuLayout->addStretch();
    grid->addLayout(gpuLayout, 5, 0);

    QLabel* threadLabel = new QLabel("并行线程数", card);
    threadLabel->setStyleSheet("font-size: 12px; font-weight: 500;");
    grid->addWidget(threadLabel, 4, 1);

    m_parallelThreadsSpin = new QSpinBox(card);
    m_parallelThreadsSpin->setRange(1, 64);
    m_parallelThreadsSpin->setValue(4);
    m_parallelThreadsSpin->setFixedWidth(120);
    grid->addWidget(m_parallelThreadsSpin, 5, 1);

    groupLayout->addWidget(card);
}

/**
 * @brief 设置已安装模型网格展示区域
 *
 * 预置若干示例模型数据，并为每个模型创建信息卡片，
 * 卡片中包含模型名称、大小、类型、运行状态徽章及更多操作按钮。
 */
void ModelConfigPage::setupModelGrid()
{
    m_installedModelsGroup = new QWidget(m_contentWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(m_installedModelsGroup);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(16);

    QLabel* title = new QLabel("已安装的模型", m_installedModelsGroup);
    title->setObjectName("sectionTitle");
    title->setStyleSheet("font-size: 15px; font-weight: 600;");
    groupLayout->addWidget(title);

    QGridLayout* grid = new QGridLayout();
    grid->setSpacing(12);

    // 初始时没有已安装模型，测试连接成功后动态填充
    m_models.clear();

    for (int i = 0; i < m_models.size(); ++i) {
        QWidget* card = new QWidget(m_installedModelsGroup);
        card->setObjectName("modelCard");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(6);

        // 卡片头部：名称 + 更多按钮
        QHBoxLayout* header = new QHBoxLayout();
        QLabel* name = new QLabel(m_models[i].name, card);
        name->setStyleSheet("font-size: 14px; font-weight: 600;");
        header->addWidget(name);
        header->addStretch();

        QPushButton* moreBtn = new QPushButton("\u22EE", card);
        moreBtn->setObjectName("iconButton");
        moreBtn->setFixedSize(24, 24);
        moreBtn->setCursor(Qt::PointingHandCursor);
        header->addWidget(moreBtn);
        cardLayout->addLayout(header);

        // 辅助lambda：添加标签-值行，状态行使用彩色徽章
        auto addRow = [&](const QString& label, const QString& value, bool isStatus = false) {
            QHBoxLayout* row = new QHBoxLayout();
            QLabel* l = new QLabel(label, card);
            l->setStyleSheet("font-size: 12px;");
            row->addWidget(l);
            row->addStretch();
            if (isStatus) {
                QLabel* badge = new QLabel(value, card);
                QString color = value == "运行中" ? "#22C55E" : (value == "已停止" ? "#94A3B8" : "#3B82F6");
                QString bg = value == "运行中" ? "rgba(34, 197, 94, 0.1)" : (value == "已停止" ? "rgba(148, 163, 184, 0.1)" : "rgba(59, 130, 246, 0.1)");
                badge->setStyleSheet(QString("background: %1; color: %2; padding: 2px 8px; border-radius: 999px; font-size: 11px; font-weight: 500;").arg(bg, color));
                row->addWidget(badge);
            } else {
                QLabel* v = new QLabel(value, card);
                v->setStyleSheet("font-size: 12px; font-weight: 500;");
                row->addWidget(v);
            }
            cardLayout->addLayout(row);
        };

        addRow("大小", m_models[i].size);
        addRow("类型", m_models[i].type);
        addRow("状态", m_models[i].status, true);

        grid->addWidget(card, 0, i);
    }

    groupLayout->addLayout(grid);
}

/**
 * @brief 应用当前主题样式
 *
 * 从ThemeManager获取颜色令牌，为内容区、连接卡片、高级卡片、模型卡片、
 * 各类输入框与按钮构建并设置QSS样式表。
 */
void ModelConfigPage::applyTheme()
{
    ThemeManager* tm = ThemeManager::instance();
    QString bg = tm->background().name();
    QString bg2 = tm->bgSecondary().name();
    QString bg3 = tm->bgTertiary().name();
    QString surf = tm->surface().name();
    QString bdr = tm->border().name();
    QString pri = tm->primary().name();
    QString txtPri = tm->textPrimary().name();
    QString txtSec = tm->textSecondary().name();
    QString txtTer = tm->textTertiary().name();
    QString sucBg = tm->stateSuccessBg().name();
    QString suc = tm->stateSuccess().name();

    m_contentWidget->setStyleSheet(QString(
        "QWidget#configContent { background-color: %1; }"
        "QWidget#connectionCard { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        "QLabel#statusTitle { color: %8; }"
        "QLabel#statusDetail { color: %10; }"
        "QLabel#statusDot { background-color: %9; border-radius: 5px; }"
        "QWidget#advancedCard { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        "QWidget#modelCard { background-color: %5; border: 1px solid %4; border-radius: 8px; }"
        "QWidget#modelCard:hover { border-color: %6; }"
        "QLineEdit#formInput { background-color: %5; color: %8; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; font-size: 13px; }"
        "QLineEdit#formInput:focus { border-color: %6; }"
        "QComboBox#formSelect { background-color: %5; color: %8; border: 1px solid %4; border-radius: 8px; padding: 8px 12px; font-size: 13px; }"
        "QComboBox#formSelect:focus { border-color: %6; }"
        "QPushButton#primaryButton { background-color: %6; color: white; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"
        "QPushButton#primaryButton:hover { background-color: %7; }"
        "QPushButton#secondaryButton { background-color: %5; color: %8; border: 1px solid %4; border-radius: 8px; padding: 8px 20px; font-weight: 500; }"
        "QPushButton#secondaryButton:hover { background-color: %3; }"
        "QPushButton#ghostButton { background-color: transparent; color: %10; border: none; border-radius: 8px; padding: 8px 16px; font-weight: 500; }"
        "QPushButton#ghostButton:hover { background-color: %3; color: %8; }"
        "QPushButton#iconButton { background-color: transparent; color: %10; border: none; border-radius: 4px; }"
        "QPushButton#iconButton:hover { background-color: %3; }"
        "QRadioButton#pillButton { color: %10; }"
        "QRadioButton#pillButton:checked { color: %6; }"
        "QLabel#sectionTitle { color: %8; }"
        "QLabel { color: %8; }"
    ).arg(bg, bg2, bg3, bdr, surf, pri, txtSec, txtPri, suc, txtTer, sucBg));
}

/**
 * @brief 获取当前API地址
 * @return API URL字符串
 */
QString ModelConfigPage::apiUrl() const
{
    return m_apiUrlEdit->text();
}

/**
 * @brief 获取当前模型名称
 * @return 优先返回已安装模型下拉框当前选中项，否则返回手动输入框内容
 */
QString ModelConfigPage::modelName() const
{
    if (m_modelCombo->isEnabled() && m_modelCombo->currentIndex() > 0) {
        return m_modelCombo->currentText();
    }
    return m_modelNameEdit->text().trimmed();
}

/**
 * @brief 动态填充已安装模型列表
 * @param modelNames 从API获取到的模型名称列表
 *
 * 测试连接通过后调用，将服务器返回的已安装模型名称填入下拉框。
 * 保留第一个提示项 "-- 请先测试连接 --"，其后追加实际模型列表。
 */
void ModelConfigPage::populateInstalledModels(const QStringList& modelNames)
{
    // 更新模型下拉框
    m_modelCombo->clear();
    m_modelCombo->addItem("-- 选择已安装模型 --");
    for (const QString& name : modelNames) {
        m_modelCombo->addItem(name);
    }
    m_modelCombo->setEnabled(true);

    // 自动选中第一个模型并填入模型名称输入框
    if (!modelNames.isEmpty()) {
        m_modelCombo->setCurrentIndex(1);  // 跳过提示项
        m_modelNameEdit->setText(modelNames.first());
    }
}

/**
 * @brief 更新连接状态显示
 *
 * 连接成功时徽章变绿显示"已连接"，状态标签显示模型详情；
 * 连接失败或未连接时显示灰色"未连接"并附加错误信息。
 */
void ModelConfigPage::updateConnectionStatus(bool connected, const QString& detail)
{
    if (connected) {
        m_statusLabel->setText("模型已连接");
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500; color: #22C55E;");
        m_statusDetail->setText(detail + " · 本地部署");

        m_connectedBadge->setText("已连接");
        m_connectedBadge->setStyleSheet(
            "background: rgba(34, 197, 94, 0.1); color: #22C55E; "
            "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
        );

        m_statusDot->setStyleSheet(
            "background: #22C55E; border-radius: 5px;"
        );
    } else {
        m_statusLabel->setText(detail.isEmpty() ? "未连接" : detail);
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500; color: #94A3B8;");
        m_statusDetail->setText("请配置模型端点并测试连接");

        m_connectedBadge->setText("未连接");
        m_connectedBadge->setStyleSheet(
            "background: rgba(148, 163, 184, 0.1); color: #94A3B8; "
            "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
        );

        m_statusDot->setStyleSheet(
            "background: #94A3B8; border-radius: 5px;"
        );
    }
}

/**
 * @brief 获取Max Tokens设置值
 * @return 最大生成Token数
 */
int ModelConfigPage::maxTokens() const
{
    return m_maxTokensSpin->value();
}

/**
 * @brief 获取Temperature设置值
 * @return 温度参数值
 */
double ModelConfigPage::temperature() const
{
    return m_temperatureSpin->value();
}

/**
 * @brief 获取Top P设置值
 * @return Top P参数值
 */
double ModelConfigPage::topP() const
{
    return m_topPSpin->value();
}

/**
 * @brief 获取Context Length设置值
 * @return 上下文长度
 */
int ModelConfigPage::contextLength() const
{
    return m_contextLengthSpin->value();
}

/**
 * @brief 获取GPU显存限制设置值
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
 */
QString ModelConfigPage::backend() const
{
    int id = m_backendGroup->checkedId();
    return id == 0 ? "CPU" : (id == 1 ? "CUDA" : "Metal");
}

void ModelConfigPage::setTestButtonEnabled(bool enabled)
{
    m_testConnectionBtn->setEnabled(enabled);
}

/**
 * @brief 设置API地址
 * @param url 目标API URL
 */
void ModelConfigPage::setApiUrl(const QString& url)
{
    m_apiUrlEdit->setText(url);
}

/**
 * @brief 设置模型名称
 * @param name 目标模型名称
 *
 * 优先在已安装模型下拉框中查找匹配项；否则写入手动输入框。
 */
void ModelConfigPage::setModelName(const QString& name)
{
    int idx = m_modelCombo->findText(name);
    if (idx > 0) {
        m_modelCombo->setCurrentIndex(idx);
    } else {
        m_modelNameEdit->setText(name);
    }
}

/**
 * @brief 设置Max Tokens
 * @param tokens 最大生成Token数
 */
void ModelConfigPage::setMaxTokens(int tokens)
{
    m_maxTokensSpin->setValue(tokens);
}

/**
 * @brief 设置Temperature
 * @param temp 温度参数值
 */
void ModelConfigPage::setTemperature(double temp)
{
    m_temperatureSpin->setValue(temp);
}

/**
 * @brief 保存配置按钮点击槽函数
 *
 * 发射settingsChanged信号，由主窗口接收并同步到LlmClient。
 */
void ModelConfigPage::onSaveClicked()
{
    emit settingsChanged();
}

/**
 * @brief 测试连接按钮点击槽函数
 *
 * 发射testConnectionClicked信号，由主窗口弹出测试提示。
 */
void ModelConfigPage::onTestConnectionClicked()
{
    emit testConnectionClicked();
}

/**
 * @brief 恢复默认设置按钮点击槽函数
 *
 * 将所有控件恢复为预设默认值，并发射restoreDefaultsClicked信号。
 */
void ModelConfigPage::onRestoreDefaultsClicked()
{
    m_apiUrlEdit->clear();
    m_modelNameEdit->clear();
    m_modelCombo->clear();
    m_modelCombo->addItem("-- 请先测试连接 --");
    m_modelCombo->setEnabled(false);
    m_temperatureSpin->setValue(0.7);
    m_topPSpin->setValue(0.9);
    m_maxTokensSpin->setValue(4096);
    m_contextLengthSpin->setValue(8192);
    m_gpuMemorySpin->setValue(8);
    m_parallelThreadsSpin->setValue(4);
    updateConnectionStatus(false);
    emit restoreDefaultsClicked();
}

/**
 * @brief 后端选择变化槽函数（占位）
 * @param button 被选中的单选按钮
 *
 * 当前未实现额外逻辑，保留接口以便后续扩展。
 */
void ModelConfigPage::onBackendSelected(QAbstractButton* button)
{
    Q_UNUSED(button)
}

/**
 * @brief 主题变化响应槽函数
 *
 * 重新应用主题样式表。
 */
void ModelConfigPage::onThemeChanged()
{
    applyTheme();
}
