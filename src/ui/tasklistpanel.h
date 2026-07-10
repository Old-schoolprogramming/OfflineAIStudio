/**
 * @file tasklistpanel.h
 * @brief 任务列表面板头文件 —— 左侧边栏的核心 UI 组件
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

#ifndef TASKLISTPANEL_H  // 【条件编译保护】防止头文件被重复包含
#define TASKLISTPANEL_H  // 【定义保护宏】标记此文件已被包含过一次

#include <QWidget>       // 【引入Qt基础控件类】QWidget是所有UI控件的基类
#include <QListWidget>   // 【引入列表控件类】QListWidget展示可选项列表，支持图标、富文本、点击事件；用于显示步骤列表
#include <QVBoxLayout>   // 【引入垂直布局类】QVBoxLayout将子控件沿垂直方向排列；用于标题、目标、进度条、列表的上下布局
#include <QLabel>        // 【引入标签类】QLabel显示静态文本；用于"任务列表"标题和计划目标描述
#include <QProgressBar>  // 【引入进度条类】QProgressBar以图形化方式展示任务完成百分比；范围0-100

#include "core/task.h"   // 【引入任务核心头文件】包含TaskPlan、StepStatus等类型定义；是TaskListPanel数据模型的基础

/**
 * @brief 任务列表面板类 - 展示Multi-Agent执行计划的步骤列表和进度
 *
 * @note TaskListPanel 内部保存 m_currentPlan 的副本，
 *       通过 setPlan() 初始化，通过 updateStepStatus() 更新状态。
 *
 * 设计特点：
 * 1. 使用QListWidget展示步骤，每个步骤使用HTML富文本渲染状态图标和颜色
 * 2. 步骤编号存储在QListWidgetItem的Qt::UserRole数据中，用于点击事件识别
 * 3. updateStepStatus()通过stepId匹配更新，而非索引，确保即使列表顺序变化也能正确更新
 * 4. 进度条百分比 = completedSteps / totalSteps * 100，实时反映整体执行进度
 */
class TaskListPanel : public QWidget  // 【类声明】TaskListPanel继承QWidget，成为独立的任务列表面板
{
    Q_OBJECT  // 【Qt元对象宏】启用信号与槽、运行时类型信息等Qt核心机制

public:  // 【公有接口区】外部可访问的构造函数、计划设置和状态更新方法

    /**
     * @brief 构造函数
     * @param parent 父 QWidget；决定面板的嵌入位置和生命周期
     *
     * 调用 setupUI() 创建和布局所有子控件（标题标签、目标标签、进度条、步骤列表），
     * 并应用自定义QSS样式表（深色主题、渐变色进度条、列表项悬停效果）。
     */
    explicit TaskListPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * Qt父子对象机制自动释放所有子控件，无需手动delete。
     */
    ~TaskListPanel();

    /**
     * @brief 设置当前显示的计划
     * @param plan 由 Planner 解析生成的 TaskPlan 对象，包含目标文本和步骤列表
     *
     * @details
     * 此方法是 TaskListPanel 的初始化入口。调用后执行以下操作：
     * 1. 将plan副本保存到 m_currentPlan（避免外部plan被修改后影响面板显示）
     * 2. 清空 QListWidget 的现有条目
     * 3. 更新目标标签（m_goalLabel）显示计划目标文本，并切换为激活状态样式
     * 4. 遍历plan.steps，为每个步骤创建 QListWidgetItem：
     *    - 使用HTML富文本渲染，包含状态图标、步骤描述、Agent→工具映射
     *    - 将stepId存储在Qt::UserRole中，供点击事件识别使用
     * 5. 根据已完成的步骤数计算并设置进度条初始值
     *
     * 使用场景：Planner生成新计划后，MainWindow调用此方法将计划展示给用户
     */
    void setPlan(const TaskPlan& plan);

    /**
     * @brief 更新指定步骤的状态
     * @param stepId 要更新的步骤编号（与TaskPlan中步骤的stepId匹配）
     * @param status 新的状态值（Pending/Running/Completed/Failed/Skipped）
     *
     * @details
     * 在 m_currentPlan 中遍历查找 stepId 匹配的步骤：
     * 1. 更新该步骤的内部状态字段
     * 2. 重新生成对应 QListWidgetItem 的显示文本（新的状态图标和颜色）
     * 3. 重新计算已完成步骤数，更新进度条的值
     *
     * 如果找不到匹配的 stepId，此方法静默返回（不做任何操作）。
     * 使用场景：TaskScheduler执行过程中，Orchestrator通知某个步骤状态变化时调用
     */
    void updateStepStatus(int stepId, StepStatus status);

    /**
     * @brief 清空当前计划显示，恢复到初始空状态
     *
     * @details
     * 将 TaskListPanel 恢复到初始状态：
     * 1. 清空 QListWidget 的所有条目
     * 2. 重置目标标签为"暂无任务"，并恢复默认样式（非激活状态）
     * 3. 重置进度条值为 0%
     * 4. 清空 m_currentPlan（替换为一个空的TaskPlan对象）
     *
     * 使用场景：任务执行完毕、用户点击"新建对话"或重置任务时调用
     */
    void clearPlan();

signals:  // 【信号声明区】用户交互通知

    /**
     * @brief 用户点击了某个步骤
     * @param stepId 被点击的步骤编号（从QListWidgetItem的Qt::UserRole中提取）
     *
     * @note 当前 UI 设计中未连接此信号，保留供未来扩展
     *       （如点击步骤后，自动滚动 OutputPanel 到对应日志位置，
     *        或在右侧面板显示该步骤的详细参数和输出）。
     */
    void stepClicked(int stepId);

private slots:  // 【私有槽函数区】处理内部控件的事件

    /**
     * @brief QListWidget 条目点击事件处理槽函数
     * @param item 被点击的 QListWidgetItem 指针
     *
     * @implementation
     * 从 item 的 Qt::UserRole 角色数据中提取 stepId（之前setPlan时存入），
     * 然后发射 stepClicked(stepId) 信号通知外部。
     * 如果 item 为nullptr（异常情况），不做任何操作。
     */
    void onItemClicked(QListWidgetItem* item);

private:  // 【私有成员区】内部UI控件和辅助方法

    /**
     * @brief 创建和布局所有 UI 子控件
     *
     * @implementation
     * 创建以下子控件并添加到垂直布局中：
     * 1. m_titleLabel — "任务列表"标题，16px白色加粗文字，底部8px内边距
     * 2. m_goalLabel  — 计划目标描述标签；默认显示"暂无任务"，使用#1e293b背景+圆角边框
     * 3. m_progressBar — 进度条，范围0-100，渐变紫色填充（indigo→violet）
     * 4. m_taskList    — 步骤列表（QListWidget），深色背景，带悬停和选中效果
     *
     * 同时为所有控件设置自定义QSS样式表，确保深色主题一致性。
     */
    void setupUI();

    /**
     * @brief 获取状态对应的图标字符（Unicode符号）
     * @param status 步骤状态枚举值
     * @return 图标字符字符串（○/◐/✓/✗/⊘）
     *
     * 图标映射关系：
     * - Pending（等待）:   ○ 空心圆，表示尚未开始执行
     * - Running（执行中）: ◐ 半圆，表示正在执行中
     * - Completed（成功）: ✓ 对勾，表示执行成功完成
     * - Failed（失败）:    ✗ 叉号，表示执行失败
     * - Skipped（跳过）:   ⊘ 禁止符，表示该步骤被跳过未执行
     */
    QString statusIcon(StepStatus status) const;

    /**
     * @brief 获取状态对应的颜色代码（CSS颜色字符串）
     * @param status 步骤状态枚举值
     * @return CSS颜色字符串（如 #94a3b8、#6366f1）
     *
     * 颜色映射关系：
     * - Pending:   #94a3b8 (slate-400，中性灰色，低调表示等待)
     * - Running:   #6366f1 (indigo-500，靛蓝色，醒目表示执行中)
     * - Completed: #22c55e (green-500，绿色，明确表示成功)
     * - Failed:    #ef4444 (red-500，红色，警示表示失败)
     * - Skipped:   #64748b (slate-500，深灰色，表示跳过)
     */
    QString statusColor(StepStatus status) const;

    QListWidget*    m_taskList;    // 【步骤列表控件指针】QListWidget实例；展示所有步骤条目，每个条目使用HTML富文本渲染；支持点击选择和悬停效果
    QLabel*         m_titleLabel;  // 【标题标签指针】显示"任务列表"文字；16px字号、白色、font-weight:600（半加粗），底部有8px内边距
    QLabel*         m_goalLabel;   // 【目标描述标签指针】显示计划目标文本；默认显示"暂无任务"；支持自动换行（wordWrap=true）；使用#1e293b背景+圆角边框
    QProgressBar*   m_progressBar; // 【进度条指针】QProgressBar实例；范围0-100，显示任务完成百分比；使用qlineargradient实现indigo→violet渐变填充；高度20px
    QVBoxLayout*    m_mainLayout;  // 【主布局指针】QVBoxLayout实例，垂直排列标题、目标、进度条、列表；边距12px，间距10px
    TaskPlan        m_currentPlan; // 【当前计划副本】保存当前显示的TaskPlan数据；包含目标文本和步骤列表；通过setPlan更新，通过updateStepStatus修改步骤状态
};

#endif // TASKLISTPANEL_H  // 【结束条件编译】对应开头的#ifndef
