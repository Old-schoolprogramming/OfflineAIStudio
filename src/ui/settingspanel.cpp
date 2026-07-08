#include "settingspanel.h"

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

    m_apiUrlEdit = new QLineEdit("http://localhost:8080/v1/chat/completions", this);
    m_apiUrlEdit->setPlaceholderText("API地址");
    formLayout->addRow("API地址:", m_apiUrlEdit);

    m_modelNameEdit = new QLineEdit("model", this);
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

SettingsPanel::~SettingsPanel()
{
}

QString SettingsPanel::apiUrl() const
{
    return m_apiUrlEdit->text();
}

QString SettingsPanel::modelName() const
{
    return m_modelNameEdit->text();
}

int SettingsPanel::maxTokens() const
{
    return m_maxTokensSpin->value();
}

double SettingsPanel::temperature() const
{
    return m_temperatureSpin->value();
}

void SettingsPanel::setApiUrl(const QString& url)
{
    m_apiUrlEdit->setText(url);
}

void SettingsPanel::setModelName(const QString& name)
{
    m_modelNameEdit->setText(name);
}

void SettingsPanel::setMaxTokens(int tokens)
{
    m_maxTokensSpin->setValue(tokens);
}

void SettingsPanel::setTemperature(double temp)
{
    m_temperatureSpin->setValue(temp);
}

void SettingsPanel::onSaveClicked()
{
    emit settingsChanged();
}
