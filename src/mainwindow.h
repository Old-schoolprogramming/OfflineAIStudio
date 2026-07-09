/**
 * @file mainwindow.h
 * @brief 主窗口 —— 应用程序顶层容器与导航中枢
 *
 * MainWindow继承自QMainWindow，作为整个应用的容器框架：
 * - 左侧浮动导航栏：提供对话/模型配置/技能导入三个页面的切换入口
 * - 顶部标题栏：显示当前页面标题、搜索框、主题状态
 * - 主内容区：通过QStackedWidget管理三个页面的堆叠切换
 *
 * 同时负责核心组件（LlmClient、Orchestrator、Agent）的生命周期管理与信号路由。
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QFile>

#include "core/llmclient.h"
#include "core/orchestrator.h"
#include "core/task.h"
#include "agents/fileagent.h"
#include "agents/computeragent.h"
#include "agents/searchagent.h"
#include "agents/codeagent.h"
#include "ui/chatpage.h"
#include "ui/modelconfigpage.h"
#include "ui/skillimportpage.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Orchestrator信号处理槽 —— 任务执行生命周期
    void onMessageSent(const QString& message);       ///< 用户发送消息，触发Orchestrator处理
    void onStopClicked();                              ///< 用户点击停止，中断当前执行
    void onPlanGenerated(const TaskPlan& plan);        ///< 计划生成完毕，更新UI状态
    void onStepStarted(int stepId);                    ///< 步骤开始执行
    void onStepOutput(int stepId, const QString& output); ///< 步骤产生输出日志
    void onStepCompleted(int stepId, const QVariantMap& result); ///< 步骤成功完成
    void onStepFailed(int stepId, const QString& error); ///< 步骤执行失败
    void onPlanCompleted(const TaskPlan& plan);        ///< 整个计划执行完毕
    void onOrchestratorMessage(const QString& message);///< 收到AI普通文本消息
    void onOrchestratorError(const QString& error);    ///< 发生错误
    void onResponseChunk(const QString& chunk);        ///< 收到流式响应片段
    void onResponseCompleted();                        ///< 流式响应完成
    void onLogMessage(const QString& logType, const QString& message); ///< 日志消息（LLM/Agent等）

    // 模型配置页面信号处理
    void onSettingsChanged();                          ///< 模型设置变更，同步到LlmClient
    void onTestConnection();                           ///< 测试模型连接
    void onRestoreDefaults();                          ///< 恢复默认设置

    // 导航与UI交互
    void onNavButtonClicked(int pageIndex);            ///< 导航按钮点击切换页面
    void onThemeToggleClicked();                       ///< 主题切换按钮点击
    void onThemeChanged();                             ///< 主题变更时更新所有样式
    void onNewChatClicked();                           ///< 新建对话
    void onSearchTextChanged(const QString& text);     ///< 搜索文本变化
    void onConversationSelected(int index);           ///< 对话历史选择

    void saveConfig();                                ///< 保存配置到文件
    void loadConfig();                                ///< 从文件加载配置

private:
    void setupUI();           ///< 构建整体布局（侧边栏+主区域）
    void setupSidebar();      ///< 构建左侧浮动导航栏
    void setupMainArea();     ///< 构建主内容区域
    void setupHeader();       ///< 构建顶部标题栏
    void setupPages();        ///< 构建三个子页面并添加到QStackedWidget
    void setupCoreComponents(); ///< 初始化核心组件（LLM、Orchestrator、Agent）
    void applyStyles();       ///< 应用动态主题样式表
    void updateNavButtons();  ///< 更新导航按钮的active状态和图标颜色

    // 侧边栏组件
    QWidget* m_sidebar;           ///< 左侧导航栏容器
    QWidget* m_logoWidget;        ///< Logo图标区域
    QPushButton* m_navMain;       ///< 主界面导航按钮（对话）
    QPushButton* m_navModel;      ///< 模型配置导航按钮
    QPushButton* m_navSkill;      ///< 技能导入导航按钮
    QPushButton* m_themeToggle;   ///< 主题切换按钮

    // 主区域组件
    QWidget* m_mainArea;          ///< 主内容区容器
    QWidget* m_header;            ///< 顶部标题栏
    QLabel* m_headerTitle;        ///< 页面标题标签
    QLineEdit* m_searchInput;     ///< 搜索输入框
    QLabel* m_themeLabel;         ///< 当前主题模式标签
    QStackedWidget* m_stack;      ///< 页面堆叠管理器

    // 三个子页面
    ChatPage* m_chatPage;         ///< 对话页面
    ModelConfigPage* m_modelPage; ///< 模型配置页面
    SkillImportPage* m_skillPage; ///< 技能导入页面

    // 核心组件
    LlmClient* m_llmClient;           ///< LLM客户端（负责网络请求）
    Orchestrator* m_orchestrator;     ///< 总控协调器（规划+调度）
    FileAgent* m_fileAgent;           ///< 文件操作Agent
    ComputerAgent* m_computerAgent;   ///< 系统命令Agent
    SearchAgent* m_searchAgent;       ///< 网络搜索Agent
    CodeAgent* m_codeAgent;           ///< 代码开发Agent
    EnvironmentDetector* m_envDetector; ///< 环境检测器

    int m_currentPage;                ///< 当前活动页面索引（0=对话,1=模型,2=技能）
    QNetworkAccessManager* m_testManager; ///< 测试连接用的网络管理器
};

#endif
