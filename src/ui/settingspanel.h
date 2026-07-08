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

/**
 * @brief 设置面板组件 - LLM相关参数配置界面
 *
 * 这个组件提供了一个表单界面，让用户可以配置大模型的连接参数：
 * - API地址：本地模型服务的HTTP接口地址
 * - 模型名称：使用的模型标识
 * - 最大Token数：每次生成文本的最大长度
 * - 温度：控制生成的创造性程度（0-1）
 *
 * 修改后点击保存按钮会触发settingsChanged信号，
 * 通知主程序更新LLM客户端的配置。
 */
class SettingsPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit SettingsPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SettingsPanel();

    /**
     * @brief 获取API地址
     * @return API URL字符串
     */
    QString apiUrl() const;

    /**
     * @brief 获取模型名称
     * @return 模型名称
     */
    QString modelName() const;

    /**
     * @brief 获取最大Token数
     * @return Token数量
     */
    int maxTokens() const;

    /**
     * @brief 获取温度值
     * @return 温度（0.0-1.0）
     */
    double temperature() const;

    /**
     * @brief 设置API地址
     * @param url API URL
     */
    void setApiUrl(const QString& url);

    /**
     * @brief 设置模型名称
     * @param name 模型名称
     */
    void setModelName(const QString& name);

    /**
     * @brief 设置最大Token数
     * @param tokens Token数量
     */
    void setMaxTokens(int tokens);

    /**
     * @brief 设置温度值
     * @param temp 温度值
     */
    void setTemperature(double temp);

signals:
    /**
     * @brief 设置已更改信号
     *
     * 用户点击保存按钮后触发，通知主程序更新配置。
     */
    void settingsChanged();

private slots:
    /**
     * @brief 保存按钮点击的槽函数
     */
    void onSaveClicked();

private:
    QLineEdit* m_apiUrlEdit;        ///< API地址输入框
    QLineEdit* m_modelNameEdit;     ///< 模型名称输入框
    QSpinBox* m_maxTokensSpin;      ///< 最大Token数数字选择框
    QDoubleSpinBox* m_temperatureSpin; ///< 温度值数字选择框
    QPushButton* m_saveButton;      ///< 保存设置按钮
};

#endif
