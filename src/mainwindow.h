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
#ifndef MAINWINDOW_H  // 【条件编译】防止头文件被重复包含，如果未定义MAINWINDOW_H则继续
#define MAINWINDOW_H  // 【宏定义】定义MAINWINDOW_H宏，标记该头文件已被包含

#include <QMainWindow>        // 【引入Qt主窗口类】应用程序顶级窗口基类
#include <QHBoxLayout>        // 【引入水平布局类】用于将子控件水平排列
#include <QVBoxLayout>        // 【引入垂直布局类】用于将子控件垂直排列
#include <QStackedWidget>     // 【引入堆叠控件类】管理多个页面的叠加显示，每次只显示一个
#include <QPushButton>        // 【引入按钮类】可点击的按钮控件
#include <QLabel>             // 【引入标签类】用于显示文本或图片
#include <QLineEdit>          // 【引入单行输入框类】用户可输入单行文本
#include <QWidget>            // 【引入控件基类】所有可视控件的基类
#include <QNetworkAccessManager> // 【引入网络访问管理器】发起HTTP网络请求
#include <QNetworkReply>      // 【引入网络回复类】接收HTTP响应数据
#include <QCoreApplication>   // 【引入核心应用类】获取应用程序目录等全局信息
#include <QFile>              // 【引入文件类】读写本地文件

#include "core/llmclient.h"        // 【引入LLM客户端头文件】负责与大语言模型API通信
#include "core/orchestrator.h"     // 【引入总控协调器头文件】负责任务规划与执行调度
#include "core/task.h"             // 【引入任务头文件】定义任务相关数据结构
#include "agents/fileagent.h"      // 【引入文件Agent头文件】处理文件读写操作
#include "agents/computeragent.h"  // 【引入系统命令Agent头文件】执行系统命令
#include "agents/searchagent.h"    // 【引入搜索Agent头文件】网络搜索功能
#include "agents/codeagent.h"      // 【引入代码Agent头文件】代码开发与编译
#include "ui/chatpage.h"           // 【引入对话页面头文件】聊天交互界面
#include "ui/modelconfigpage.h"    // 【引入模型配置页面头文件】AI模型参数设置
#include "ui/skillimportpage.h"    // 【引入技能导入页面头文件】导入外部技能

// 【类声明】MainWindow继承自QMainWindow，作为应用程序的主窗口
class MainWindow : public QMainWindow
{
    Q_OBJECT  // 【Qt宏】启用Qt的元对象系统，支持信号槽、属性等特性

public:
    // 【构造函数】explicit防止隐式转换，parent指定父窗口（默认为nullptr表示无父窗口）
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();  // 【析构函数】释放主窗口占用的资源

private slots:
    // ========== Orchestrator信号处理槽 —— 任务执行生命周期 ==========
    // 【槽函数】用户发送消息时触发，将消息交给Orchestrator处理
    void onMessageSent(const QString& message);
    // 【槽函数】用户点击停止按钮，中断当前正在执行的任务
    void onStopClicked();
    // 【槽函数】计划生成完毕，更新UI状态以显示新计划
    void onPlanGenerated(const TaskPlan& plan);
    // 【槽函数】某个步骤开始执行，更新该步骤的UI状态为运行中
    void onStepStarted(int stepId);
    // 【槽函数】步骤产生输出日志，将日志追加到对应步骤的显示区域
    void onStepOutput(int stepId, const QString& output);
    // 【槽函数】步骤成功完成，更新UI状态并显示结果摘要
    void onStepCompleted(int stepId, const QVariantMap& result);
    // 【槽函数】步骤执行失败，更新UI状态并显示错误信息
    void onStepFailed(int stepId, const QString& error);
    // 【槽函数】整个计划执行完毕，显示任务完成总结
    void onPlanCompleted(const TaskPlan& plan);
    // 【槽函数】收到AI普通文本消息，将其显示在对话页面
    void onOrchestratorMessage(const QString& message);
    // 【槽函数】发生错误，在对话页面显示红色错误提示
    void onOrchestratorError(const QString& error);
    // 【槽函数】收到流式响应片段，实时追加到当前消息
    void onResponseChunk(const QString& chunk);
    // 【槽函数】流式响应完成，结束当前消息的显示
    void onResponseCompleted();
    // 【槽函数】收到日志消息（来自LLM/Agent等），在日志区域显示
    void onLogMessage(const QString& logType, const QString& message);

    // ========== 模型配置页面信号处理 ==========
    // 【槽函数】模型设置变更，将新设置同步到LlmClient并保存配置
    void onSettingsChanged();
    // 【槽函数】测试模型连接，向API发送请求验证连通性
    void onTestConnection();
    // 【槽函数】恢复默认设置，将模型配置重置为初始值
    void onRestoreDefaults();

    // ========== 导航与UI交互 ==========
    // 【槽函数】导航按钮点击切换页面，pageIndex为目标页面索引
    void onNavButtonClicked(int pageIndex);
    // 【槽函数】主题切换按钮点击，在深色/浅色模式间切换
    void onThemeToggleClicked();
    // 【槽函数】主题变更时重新应用所有样式
    void onThemeChanged();
    // 【槽函数】新建对话，清空当前聊天并创建新会话
    void onNewChatClicked();
    // 【槽函数】搜索文本变化，可触发搜索过滤逻辑
    void onSearchTextChanged(const QString& text);
    // 【槽函数】用户从对话历史中选择某个会话
    void onConversationSelected(int index);
    // 【槽函数】文件选择处理，保存用户选中的文件列表
    void onFilesSelected(const QStringList& files);
    // 【槽函数】删除指定索引的对话
    void onDeleteConversation(int index);

    // 【槽函数】保存配置到本地JSON文件
    void saveConfig();
    // 【槽函数】从本地JSON文件加载配置
    void loadConfig();

private:
    // 【初始化方法】构建整体布局（侧边栏+主区域），由构造函数调用
    void setupUI();
    // 【初始化方法】构建左侧浮动导航栏，包含Logo和三个导航按钮
    void setupSidebar();
    // 【初始化方法】构建主内容区域（标题栏+页面栈）
    void setupMainArea();
    // 【初始化方法】构建顶部标题栏（标题、搜索框、主题标签）
    void setupHeader();
    // 【初始化方法】构建三个子页面并添加到QStackedWidget
    void setupPages();
    // 【初始化方法】初始化核心组件（LLM客户端、Orchestrator、Agent）
    void setupCoreComponents();
    // 【样式方法】根据当前主题生成并应用QSS样式表
    void applyStyles();
    // 【刷新方法】更新导航按钮的active状态和图标颜色
    void updateNavButtons();

    // ========== 侧边栏组件 ==========
    QWidget* m_sidebar;           // 【成员变量】左侧导航栏容器控件
    QWidget* m_logoWidget;        // 【成员变量】Logo图标区域（当前未使用，预留）
    QPushButton* m_navMain;       // 【成员变量】主界面导航按钮（切换到对话页面）
    QPushButton* m_navModel;      // 【成员变量】模型配置导航按钮（切换到模型配置页面）
    QPushButton* m_navSkill;      // 【成员变量】技能导入导航按钮（切换到技能导入页面）
    QPushButton* m_themeToggle;   // 【成员变量】主题切换按钮（深色/浅色）

    // ========== 主区域组件 ==========
    QWidget* m_mainArea;          // 【成员变量】主内容区容器
    QWidget* m_header;            // 【成员变量】顶部标题栏容器
    QLabel* m_headerTitle;        // 【成员变量】页面标题标签（显示"对话"/"模型配置"/"技能导入"）
    QLineEdit* m_searchInput;     // 【成员变量】搜索输入框
    QLabel* m_themeLabel;         // 【成员变量】当前主题模式标签（"深色模式"/"浅色模式"）
    QStackedWidget* m_stack;      // 【成员变量】页面堆叠管理器，管理三个子页面的切换

    // ========== 三个子页面 ==========
    ChatPage* m_chatPage;         // 【成员变量】对话页面实例
    ModelConfigPage* m_modelPage; // 【成员变量】模型配置页面实例
    SkillImportPage* m_skillPage; // 【成员变量】技能导入页面实例

    // ========== 核心组件 ==========
    LlmClient* m_llmClient;           // 【成员变量】LLM客户端，负责与AI模型API的网络通信
    Orchestrator* m_orchestrator;     // 【成员变量】总控协调器，负责任务规划、步骤调度
    FileAgent* m_fileAgent;           // 【成员变量】文件操作Agent，处理文件读写
    ComputerAgent* m_computerAgent;   // 【成员变量】系统命令Agent，执行shell命令
    SearchAgent* m_searchAgent;       // 【成员变量】网络搜索Agent，进行联网搜索
    CodeAgent* m_codeAgent;           // 【成员变量】代码开发Agent，代码生成与编译
    EnvironmentDetector* m_envDetector; // 【成员变量】环境检测器，检测系统可用工具

    // ========== 状态与辅助 ==========
    int m_currentPage;                // 【成员变量】当前活动页面索引（0=对话, 1=模型配置, 2=技能导入）
    QNetworkAccessManager* m_testManager; // 【成员变量】测试连接专用的网络管理器
    QStringList m_selectedFiles;       // 【成员变量】当前选中的文件列表
};

#endif  // 【条件编译结束】MAINWINDOW_H宏定义结束
