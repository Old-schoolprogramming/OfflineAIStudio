#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>

class SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPanel(QWidget *parent = nullptr);
    ~SettingsPanel();

    QString apiUrl() const;
    QString modelName() const;
    int maxTokens() const;
    double temperature() const;
    QString apiKey() const;

    void setApiUrl(const QString& url);
    void setModelName(const QString& name);
    void setMaxTokens(int tokens);
    void setTemperature(double temp);
    void setApiKey(const QString& apiKey);

signals:
    void settingsChanged();

private slots:
    void onSaveClicked();

private:
    QLineEdit* m_apiUrlEdit;
    QLineEdit* m_modelNameEdit;
    QLineEdit* m_apiKeyEdit;
    QSpinBox* m_maxTokensSpin;
    QDoubleSpinBox* m_temperatureSpin;
    QPushButton* m_saveButton;
};

#endif
