#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTabWidget>
#include <QAction>

#include "core/llmclient.h"
#include "core/orchestrator.h"
#include "agents/fileagent.h"
#include "agents/computeragent.h"
#include "ui/chatwidget.h"
#include "ui/agentselector.h"
#include "ui/settingspanel.h"

/**
 * @brief 主窗口类 - 应用程序的主界面
 *
 * 主窗口是整个应用的入口和组织者，它负责：
 * - 创建并布局所有UI组件（聊天区、侧边栏、菜单栏等）
 * - 初始化核心业务组件（LLM客户端、各Agent、调度器）
 * - 连接信号槽，协调各组件之间的交互
 * - 应用暗色高级主题样式
 *
 * 整体布局：
 * ┌─────────────────────────────────────┐
 * │              菜单栏                 │
 * ├──────────┬──────────────────────────┤
 * │ 侧边栏   │                          │
 * │ ┌──────┐ │      聊天窗口           │
 * │ │Agent │ │                          │
 * │ ├──────┤ │                          │
 * │ │设置  │ │                          │
 * │ └──────┘ │                          │
 * ├──────────┴──────────────────────────┤
 * │              状态栏                 │
 * └─────────────────────────────────────┘
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数 - 初始化整个应用程序
     * @param parent 父窗口（通常为nullptr）
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief 用户发送消息的槽函数
     * @param message 消息内容
     */
    void onMessageSent(const QString& message);

    /**
     * @brief LLM返回消息的槽函数
     * @param message AI回复的消息
     */
    void onLlmMessageReceived(const QString& message);

    /**
     * @brief Agent被选中/调用的槽函数
     * @param agentName Agent或工具名称
     */
    void onAgentSelected(const QString& agentName);

    /**
     * @brief 设置已更改的槽函数
     */
    void onSettingsChanged();

    /**
     * @brief 关于菜单点击的槽函数
     */
    void onAboutAction();

    /**
     * @brief 清空聊天菜单点击的槽函数
     */
    void onClearChatAction();

    /**
     * @brief 显示/隐藏侧边栏菜单点击的槽函数
     */
    void onToggleSidebarAction();

private:
    /**
     * @brief 设置用户界面
     *
     * 创建所有UI组件并进行布局。
     */
    void setupUI();

    /**
     * @brief 设置菜单栏
     *
     * 创建菜单栏和各菜单项，连接到对应的槽函数。
     */
    void setupMenuBar();

    /**
     * @brief 初始化核心业务组件
     *
     * 创建LLM客户端、各Agent和调度器，并建立它们之间的连接。
     */
    void setupCoreComponents();

    /**
     * @brief 应用样式表
     *
     * 应用暗色高级主题的QSS样式表，美化界面外观。
     */
    void applyStyles();

    ChatWidget* m_chatWidget;        ///< 聊天窗口组件
    AgentSelector* m_agentSelector;  ///< Agent选择器组件
    SettingsPanel* m_settingsPanel;  ///< 设置面板组件
    QTabWidget* m_sidebarTabs;       ///< 侧边栏标签页
    QSplitter* m_mainSplitter;       ///< 主分割器（左右布局）

    LlmClient* m_llmClient;          ///< LLM客户端
    Orchestrator* m_orchestrator;    ///< 总控调度Agent
    FileAgent* m_fileAgent;          ///< 文件操作Agent
    ComputerAgent* m_computerAgent;  ///< 系统命令Agent

    QAction* m_toggleSidebarAction;  ///< 切换侧边栏显示的动作
    bool m_sidebarVisible;           ///< 侧边栏是否可见
};

#endif
