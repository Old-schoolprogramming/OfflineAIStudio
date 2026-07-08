/**
 * @file settingspanel.cpp
 * @brief LLM 设置面板实现
 *
 * @details
 * SettingsPanel 提供 LLM（大语言模型）相关参数的图形化配置界面，
 * 包含 API 地址、API Key、模型名称、最大 Token 数和温度系数。
 *
 * 核心设计要点：
 * 1. 使用 QFormLayout 排列标签-输入控件对，整齐对齐
 * 2. API Key 输入框使用 Password 回显模式，保护敏感信息
 * 3. 所有控件提供默认值（OpenAI 兼容接口配置）
 * 4. 提供 getter/setter 方法，便于外部读取和初始化
 * 5. 点击保存按钮发射 settingsChanged 信号，通知外部持久化
 */

#include "settingspanel.h"

/**
 * @brief 构造函数
 * @param parent 父 QWidget
 *
 * @implementation
 * 布局结构：
 * mainLayout (QVBoxLayout)
 *   ├── titleLabel — "LLM 设置"标题
 *   ├── formLayout (QFormLayout)
 *   │   ├── API地址: m_apiUrlEdit (默认 https://api.openai.com/v1/chat/completions)
 *   │   ├── API Key: m_apiKeyEdit (Password 模式，留空提示)
 *   │   ├── 模型名称: m_modelNameEdit (默认 gpt-4o-mini)
 *   │   ├── 最大Token数: m_maxTokensSpin (范围 128~8192，默认 2048)
 *   │   └── 温度: m_temperatureSpin (范围 0.0~1.0，步进 0.1，默认 0.7)
 *   └── m_saveButton — "保存设置"按钮
 *
 * 信号连接：保存按钮点击 → onSaveClicked()
 */
SettingsPanel::SettingsPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel("LLM 设置", this);
    titleLabel->setObjectName("settingsTitle");
    mainLayout->addWidget(titleLabel);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(6);

    m_apiUrlEdit = new QLineEdit("https://api.openai.com/v1/chat/completions", this);
    m_apiUrlEdit->setPlaceholderText("API地址");
    formLayout->addRow("API地址:", m_apiUrlEdit);

    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setPlaceholderText("API Key (留空则不使用)");
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    formLayout->addRow("API Key:", m_apiKeyEdit);

    m_modelNameEdit = new QLineEdit("gpt-4o-mini", this);
    m_modelNameEdit->setPlaceholderText("模型名称");
    formLayout->addRow("模型名称:", m_modelNameEdit);

    m_maxTokensSpin = new QSpinBox(this);
    m_maxTokensSpin->setRange(128, 8192);
    m_maxTokensSpin->setValue(2048);
    formLayout->addRow("最大Token数:", m_maxTokensSpin);

    m_temperatureSpin = new QDoubleSpinBox(this);
    m_temperatureSpin->setRange(0.0, 1.0);
    m_temperatureSpin->setSingleStep(0.1);
    m_temperatureSpin->setValue(0.7);
    formLayout->addRow("温度:", m_temperatureSpin);

    mainLayout->addLayout(formLayout);

    m_saveButton = new QPushButton("保存设置", this);
    m_saveButton->setObjectName("saveButton");
    mainLayout->addWidget(m_saveButton);

    connect(m_saveButton, &QPushButton::clicked, this, &SettingsPanel::onSaveClicked);
}

/**
 * @brief 析构函数
 */
SettingsPanel::~SettingsPanel()
{
}

/**
 * @brief 获取 API 地址
 * @return 当前输入的 API URL 字符串
 */
QString SettingsPanel::apiUrl() const
{
    return m_apiUrlEdit->text();
}

/**
 * @brief 获取模型名称
 * @return 当前输入的模型名称字符串
 */
QString SettingsPanel::modelName() const
{
    return m_modelNameEdit->text();
}

/**
 * @brief 获取 API Key
 * @return 当前输入的 API Key 字符串
 *
 * @note 若留空则返回空字符串，表示不使用 API Key
 */
QString SettingsPanel::apiKey() const
{
    return m_apiKeyEdit->text();
}

/**
 * @brief 获取最大 Token 数
 * @return 当前设置的最大 Token 数（范围 128~8192）
 */
int SettingsPanel::maxTokens() const
{
    return m_maxTokensSpin->value();
}

/**
 * @brief 获取温度系数
 * @return 当前设置的温度值（范围 0.0~1.0）
 *
 * @note 温度值控制生成文本的随机性，越低越确定性，越高越创造性
 */
double SettingsPanel::temperature() const
{
    return m_temperatureSpin->value();
}

/**
 * @brief 设置 API 地址
 * @param url API URL 字符串
 */
void SettingsPanel::setApiUrl(const QString& url)
{
    m_apiUrlEdit->setText(url);
}

/**
 * @brief 设置模型名称
 * @param name 模型名称字符串
 */
void SettingsPanel::setModelName(const QString& name)
{
    m_modelNameEdit->setText(name);
}

/**
 * @brief 设置 API Key
 * @param apiKey API Key 字符串
 */
void SettingsPanel::setApiKey(const QString& apiKey)
{
    m_apiKeyEdit->setText(apiKey);
}

/**
 * @brief 设置最大 Token 数
 * @param tokens Token 数量（需在 128~8192 范围内）
 */
void SettingsPanel::setMaxTokens(int tokens)
{
    m_maxTokensSpin->setValue(tokens);
}

/**
 * @brief 设置温度系数
 * @param temp 温度值（需在 0.0~1.0 范围内）
 */
void SettingsPanel::setTemperature(double temp)
{
    m_temperatureSpin->setValue(temp);
}

/**
 * @brief 保存按钮点击事件处理
 *
 * @implementation
 * 发射 settingsChanged() 信号，通知外部（如主窗口或配置管理器）
 * 当前设置已变更，需要进行持久化保存。
 */
void SettingsPanel::onSaveClicked()
{
    emit settingsChanged();
}
