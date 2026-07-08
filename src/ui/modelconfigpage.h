/**
 * @file modelconfigpage.h
 * @brief 模型配置页面头文件
 *
 * 提供模型配置界面的声明，包含连接状态卡片、模型设置（API端点、模型名称、推理后端）、
 * 高级参数（Temperature、TopP、MaxTokens等）以及已安装模型网格展示。
 * 通过信号与外部交互，支持设置变更、连接测试和恢复默认配置等操作。
 */

#ifndef MODELCONFIGPAGE_H
#define MODELCONFIGPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>

/**
 * @brief 模型配置页面类
 *
 * 继承自QWidget的模型配置页面，提供完整的模型参数设置界面。
 * 包含连接状态展示、API配置、模型选择、推理后端切换、高级参数调节及已安装模型列表等功能模块。
 */
class ModelConfigPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针，默认为nullptr
     */
    explicit ModelConfigPage(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ModelConfigPage();

    /**
     * @brief 获取当前配置的API地址
     * @return API URL字符串
     */
    QString apiUrl() const;

    /**
     * @brief 获取当前选中的模型名称
     * @return 模型名称字符串
     */
    QString modelName() const;

    /**
     * @brief 获取最大生成Token数
     * @return MaxTokens数值
     */
    int maxTokens() const;

    /**
     * @brief 获取Temperature采样温度
     * @return Temperature数值
     */
    double temperature() const;

    /**
     * @brief 获取TopP核采样参数
     * @return TopP数值
     */
    double topP() const;

    /**
     * @brief 获取上下文长度
     * @return 上下文长度数值
     */
    int contextLength() const;

    /**
     * @brief 获取GPU显存限制
     * @return GPU显存限制数值（单位：MB）
     */
    int gpuMemoryLimit() const;

    /**
     * @brief 获取并行线程数
     * @return 并行线程数
     */
    int parallelThreads() const;

    /**
     * @brief 获取当前选中的推理后端
     * @return 后端名称字符串
     */
    QString backend() const;

    /**
     * @brief 设置API地址
     * @param url API URL字符串
     */
    void setApiUrl(const QString& url);

    /**
     * @brief 设置模型名称
     * @param name 模型名称字符串
     */
    void setModelName(const QString& name);

    /**
     * @brief 设置最大生成Token数
     * @param tokens MaxTokens数值
     */
    void setMaxTokens(int tokens);

    /**
     * @brief 设置Temperature采样温度
     * @param temp Temperature数值
     */
    void setTemperature(double temp);

signals:
    /**
     * @brief 设置发生变更时发出的信号
     */
    void settingsChanged();

    /**
     * @brief 测试连接按钮被点击时发出的信号
     */
    void testConnectionClicked();

    /**
     * @brief 恢复默认设置按钮被点击时发出的信号
     */
    void restoreDefaultsClicked();

private slots:
    void onSaveClicked();                           ///< 保存按钮点击槽函数
    void onTestConnectionClicked();                 ///< 测试连接按钮点击槽函数
    void onRestoreDefaultsClicked();                ///< 恢复默认按钮点击槽函数
    void onBackendSelected(QAbstractButton* button);///< 推理后端选择变更槽函数
    void onThemeChanged();                          ///< 主题变更响应槽函数

private:
    void setupUI();             ///< 初始化整体界面布局
    void setupConnectionCard(); ///< 构建连接状态卡片
    void setupModelSettings();  ///< 构建模型设置区域
    void setupAdvancedSettings();///< 构建高级参数设置区域
    void setupModelGrid();      ///< 构建已安装模型网格
    void applyTheme();          ///< 应用当前主题样式

    QScrollArea* m_scrollArea;          ///< 滚动区域容器
    QWidget* m_contentWidget;           ///< 滚动内容主窗口
    QWidget* m_connectionCard;          ///< 连接状态卡片
    QWidget* m_modelSettingsGroup;      ///< 模型设置分组
    QWidget* m_advancedGroup;           ///< 高级参数分组
    QWidget* m_installedModelsGroup;    ///< 已安装模型分组

    // Connection status
    QLabel* m_statusLabel;              ///< 连接状态标签
    QLabel* m_statusDetail;             ///< 连接状态详情标签
    QPushButton* m_testConnectionBtn;   ///< 测试连接按钮

    // Model settings
    QLineEdit* m_apiUrlEdit;            ///< API地址输入框
    QComboBox* m_modelCombo;            ///< 模型选择下拉框
    QButtonGroup* m_backendGroup;       ///< 推理后端按钮组

    // Advanced settings
    QDoubleSpinBox* m_temperatureSpin;  ///< Temperature参数调节框
    QDoubleSpinBox* m_topPSpin;         ///< TopP参数调节框
    QSpinBox* m_maxTokensSpin;          ///< MaxTokens参数调节框
    QSpinBox* m_contextLengthSpin;      ///< 上下文长度调节框
    QSpinBox* m_gpuMemorySpin;          ///< GPU显存限制调节框
    QSpinBox* m_parallelThreadsSpin;    ///< 并行线程数调节框

    // Buttons
    QPushButton* m_saveBtn;             ///< 保存按钮
    QPushButton* m_restoreBtn;          ///< 恢复默认按钮

    struct ModelInfo {
        QString name;       ///< 模型名称
        QString size;       ///< 模型大小
        QString type;       ///< 模型类型
        QString status;     ///< 模型状态
        bool isRunning;     ///< 是否运行中
    };
    QList<ModelInfo> m_models; ///< 已安装模型列表
};

#endif
