#include "modelconfigpage.h"
#include "thememanager.h"
#include "iconhelper.h"
#include <QButtonGroup>
#include <QRadioButton>
#include <QFormLayout>
#include <QFrame>
#include <QFileDialog>

ModelConfigPage::ModelConfigPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    applyTheme();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &ModelConfigPage::onThemeChanged);
}

ModelConfigPage::~ModelConfigPage()
{
}

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

    // Bottom action bar
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

    QLabel* statusDot = new QLabel(statusWidget);
    statusDot->setFixedSize(10, 10);
    statusDot->setObjectName("statusDot");
    statusLayout->addWidget(statusDot);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);
    m_statusLabel = new QLabel("模型已连接", statusWidget);
    m_statusLabel->setObjectName("statusTitle");
    m_statusLabel->setStyleSheet("font-size: 14px; font-weight: 500;");
    textLayout->addWidget(m_statusLabel);

    m_statusDetail = new QLabel("Qwen-7B · 本地部署", statusWidget);
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

    // Endpoint
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
    m_apiUrlEdit = new QLineEdit("http://localhost:8080/v1", m_modelSettingsGroup);
    m_apiUrlEdit->setObjectName("formInput");
    endpointInputLayout->addWidget(m_apiUrlEdit, 1);

    QLabel* connectedBadge = new QLabel("已连接", m_modelSettingsGroup);
    connectedBadge->setObjectName("successBadge");
    connectedBadge->setStyleSheet(
        "background: rgba(34, 197, 94, 0.1); color: #22C55E; "
        "padding: 4px 12px; border-radius: 999px; font-size: 12px; font-weight: 500;"
    );
    endpointInputLayout->addWidget(connectedBadge);
    endpointLayout->addLayout(endpointInputLayout);
    groupLayout->addLayout(endpointLayout);

    // Model Name
    QVBoxLayout* modelLayout = new QVBoxLayout();
    modelLayout->setSpacing(6);
    QLabel* modelLabel = new QLabel("模型名称", m_modelSettingsGroup);
    modelLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    modelLayout->addWidget(modelLabel);

    m_modelCombo = new QComboBox(m_modelSettingsGroup);
    m_modelCombo->setObjectName("formSelect");
    m_modelCombo->addItems({"Qwen-7B-Instruct", "Qwen-14B-Chat", "DeepSeek-Coder-7B", "Llama-3-8B"});
    m_modelCombo->setFixedWidth(360);
    modelLayout->addWidget(m_modelCombo);
    groupLayout->addLayout(modelLayout);

    // Backend
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

    // Row 0: Temperature & Top P
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

    // Row 1: Max Tokens & Context Length
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

    // Row 2: GPU Memory & Parallel Threads
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

    m_models = {
        {"Qwen-7B-Instruct", "4.2 GB", "通用对话", "运行中", true},
        {"DeepSeek-Coder-7B", "3.8 GB", "代码生成", "已停止", false},
        {"Llama-3-8B", "4.7 GB", "通用对话", "已安装", false}
    };

    for (int i = 0; i < m_models.size(); ++i) {
        QWidget* card = new QWidget(m_installedModelsGroup);
        card->setObjectName("modelCard");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(6);

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

QString ModelConfigPage::apiUrl() const
{
    return m_apiUrlEdit->text();
}

QString ModelConfigPage::modelName() const
{
    return m_modelCombo->currentText();
}

int ModelConfigPage::maxTokens() const
{
    return m_maxTokensSpin->value();
}

double ModelConfigPage::temperature() const
{
    return m_temperatureSpin->value();
}

double ModelConfigPage::topP() const
{
    return m_topPSpin->value();
}

int ModelConfigPage::contextLength() const
{
    return m_contextLengthSpin->value();
}

int ModelConfigPage::gpuMemoryLimit() const
{
    return m_gpuMemorySpin->value();
}

int ModelConfigPage::parallelThreads() const
{
    return m_parallelThreadsSpin->value();
}

QString ModelConfigPage::backend() const
{
    int id = m_backendGroup->checkedId();
    return id == 0 ? "CPU" : (id == 1 ? "CUDA" : "Metal");
}

void ModelConfigPage::setApiUrl(const QString& url)
{
    m_apiUrlEdit->setText(url);
}

void ModelConfigPage::setModelName(const QString& name)
{
    int idx = m_modelCombo->findText(name);
    if (idx >= 0) m_modelCombo->setCurrentIndex(idx);
}

void ModelConfigPage::setMaxTokens(int tokens)
{
    m_maxTokensSpin->setValue(tokens);
}

void ModelConfigPage::setTemperature(double temp)
{
    m_temperatureSpin->setValue(temp);
}

void ModelConfigPage::onSaveClicked()
{
    emit settingsChanged();
}

void ModelConfigPage::onTestConnectionClicked()
{
    emit testConnectionClicked();
}

void ModelConfigPage::onRestoreDefaultsClicked()
{
    m_apiUrlEdit->setText("http://localhost:8080/v1");
    m_modelCombo->setCurrentIndex(0);
    m_temperatureSpin->setValue(0.7);
    m_topPSpin->setValue(0.9);
    m_maxTokensSpin->setValue(4096);
    m_contextLengthSpin->setValue(8192);
    m_gpuMemorySpin->setValue(8);
    m_parallelThreadsSpin->setValue(4);
    emit restoreDefaultsClicked();
}

void ModelConfigPage::onBackendSelected(QAbstractButton* button)
{
    Q_UNUSED(button)
}

void ModelConfigPage::onThemeChanged()
{
    applyTheme();
}
