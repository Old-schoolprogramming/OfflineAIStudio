/**
 * @file tasklistpanel.h
 * @brief 任务列表面板 —— 左侧边栏的核心 UI 组件
 *
 * @details
 * TaskListPanel 是用户与 Multi-Agent 执行系统交互的主要可视化入口。
 * 它展示由 Planner 解析生成的 TaskPlan，并在 TaskScheduler 执行过程中
 * 实时更新每个步骤的状态、进度条和统计信息。
 *
 * 功能特性：
 * - 展示任务目标（plan.goal）
 * - 展示进度条（基于 completedSteps / totalSteps）
 * - 步骤列表：每个步骤显示状态图标、描述、Agent→工具映射
 * - 状态实时更新：Pending → Running → Completed / Failed
 * - 颜色编码：灰色(等待)、紫色(执行中)、绿色(成功)、红色(失败)
 * - 点击步骤可查看详情（通过 stepClicked 信号）
 *
 * 与 TaskScheduler 的交互：
 *   TaskScheduler 发射 stepStarted / stepCompleted / stepFailed 信号
 *   → Orchestrator 透传 → MainWindow 接收
 *   → MainWindow 调用 updateStepStatus() 更新对应步骤状态
 *
 * 样式设计：
 * - 深色主题，与主窗口协调
 * - QListWidget 自定义样式（悬停、选中效果）
 * - 进度条使用渐变色（indigo → violet）
 */

#ifndef TASKLISTPANEL_H
#define TASKLISTPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>

#include "core/task.h"

/**
 * @brief 任务列表面板
 *
 * @note TaskListPanel 内部保存 m_currentPlan 的副本，
 *       通过 setPlan() 初始化，通过 updateStepStatus() 更新状态。
 */
class TaskListPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父 QWidget
     *
     * 调用 setupUI() 创建和布局所有子控件，应用自定义样式表。
     */
    explicit TaskListPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TaskListPanel();

    /**
     * @brief 设置当前显示的计划
     * @param plan 由 Planner 解析的 TaskPlan
     *
     * @details
     * 此方法是 TaskListPanel 的初始化入口。调用后：
     * 1. 保存 plan 副本到 m_currentPlan
     * 2. 清空并重建 QListWidget 的条目
     * 3. 更新目标标签（m_goalLabel）
     * 4. 设置进度条初始值
     *
     * 每个 QListWidgetItem 使用富文本（HTML）渲染，包含状态图标、
     * 步骤描述、Agent→工具映射信息。
     */
    void setPlan(const TaskPlan& plan);

    /**
     * @brief 更新指定步骤的状态
     * @param stepId 要更新的步骤编号
     * @param status 新的状态值
     *
     * @details
     * 在 m_currentPlan 中查找 stepId 匹配的步骤：
     * 1. 更新步骤的内部状态
     * 2. 更新对应 QListWidgetItem 的显示文本（重新生成 HTML）
     * 3. 重新计算并更新进度条的值
     *
     * 如果找不到匹配的 stepId，此方法静默返回。
     */
    void updateStepStatus(int stepId, StepStatus status);

    /**
     * @brief 清空当前计划显示
     *
     * @details
     * 将 TaskListPanel 恢复到初始状态：
     * 1. 清空 QListWidget
     * 2. 重置目标标签为"暂无任务"
     * 3. 重置进度条为 0%
     * 4. 清空 m_currentPlan
     */
    void clearPlan();

signals:
    /**
     * @brief 用户点击了某个步骤
     * @param stepId 被点击的步骤编号
     *
     * @note 当前 UI 设计中未连接此信号，保留供未来扩展
     *       （如点击步骤后滚动 OutputPanel 到对应日志位置）。
     */
    void stepClicked(int stepId);

private slots:
    /**
     * @brief QListWidget 条目点击事件处理
     * @param item 被点击的 QListWidgetItem
     *
     * @implementation
     * 从 item 的 Qt::UserRole 数据中提取 stepId，发射 stepClicked 信号。
     */
    void onItemClicked(QListWidgetItem* item);

private:
    /**
     * @brief 创建和布局所有 UI 子控件
     *
     * @implementation
     * 创建以下子控件并添加到垂直布局中：
     * 1. m_titleLabel — "任务列表"标题
     * 2. m_goalLabel  — 计划目标描述（或"暂无任务"）
     * 3. m_progressBar — 进度条，范围 0-100
     * 4. m_taskList    — 步骤列表（QListWidget）
     *
     * 同时设置所有控件的自定义样式表（QSS）。
     */
    void setupUI();

    /**
     * @brief 获取状态对应的图标字符
     * @param status 步骤状态
     * @return 图标字符（○/◐/✓/✗/⊘）
     */
    QString statusIcon(StepStatus status) const;

    /**
     * @brief 获取状态对应的颜色代码
     * @param status 步骤状态
     * @return CSS 颜色字符串（如 #94a3b8、#6366f1）
     */
    QString statusColor(StepStatus status) const;

    QListWidget*    m_taskList;    ///< 步骤列表控件
    QLabel*         m_titleLabel;  ///< "任务列表"标题标签
    QLabel*         m_goalLabel;   ///< 计划目标描述标签
    QProgressBar*   m_progressBar; ///< 进度条
    QVBoxLayout*    m_mainLayout;  ///< 主垂直布局
    TaskPlan        m_currentPlan; ///< 当前显示的计划副本
};

#endif // TASKLISTPANEL_H
