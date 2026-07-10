/**
 * @file tasklistpanel.cpp
 * @brief 任务列表面板实现 —— 左侧任务计划与进度展示
 *
 * @details
 * TaskListPanel 负责将 Planner 解析的 TaskPlan 渲染为用户可见的 UI：
 * - 顶部标题"任务列表"
 * - 目标描述（plan.goal）
 * - 整体进度条（completed/total）
 * - 步骤列表（带状态图标、颜色编码、Agent→工具映射）
 *
 * 数据流：
 *   Planner → TaskPlan → setPlan() → 渲染条目
 *   TaskScheduler → 信号 → MainWindow → updateStepStatus() → 局部刷新
 */

#include "tasklistpanel.h"  // 引入自己的头文件

#include <QVariant>        // 引入 QVariant，用于在 QListWidgetItem 中存储自定义数据
#include <QStringBuilder>  // 引入 QStringBuilder，提升大量字符串拼接性能

/**
 * @brief 构造函数 - 初始化面板 UI 和成员变量
 * @param parent 父 QWidget 指针，默认为 nullptr（独立窗口）
 */
TaskListPanel::TaskListPanel(QWidget *parent)
    : QWidget(parent)        // 【初始化】调用基类 QWidget 构造函数
    , m_taskList(nullptr)    // 【初始化】列表指针置空
    , m_titleLabel(nullptr)  // 【初始化】标题标签指针置空
    , m_goalLabel(nullptr)   // 【初始化】目标标签指针置空
    , m_progressBar(nullptr) // 【初始化】进度条指针置空
    , m_mainLayout(nullptr)  // 【初始化】主布局指针置空
{
    setupUI();  // 创建并布局所有子控件
    clearPlan(); // 初始化为"暂无任务"空状态
}

/**
 * @brief 析构函数
 *
 * Qt 父子对象机制自动释放所有子控件，无需手动 delete。
 */
TaskListPanel::~TaskListPanel()
{
    // 子控件由 Qt 对象树自动释放
}

/**
 * @brief 设置当前显示的计划
 * @param plan 由 Planner 解析生成的 TaskPlan 对象
 */
void TaskListPanel::setPlan(const TaskPlan& plan)
{
    m_currentPlan = plan;  // 保存计划副本，避免外部修改影响显示

    // 清空现有列表条目
    if (m_taskList) {  // 安全检查
        m_taskList->clear();  // 清空列表
    }

    // 更新目标标签
    if (m_goalLabel) {  // 安全检查
        m_goalLabel->setText(plan.goal.isEmpty() ? "（无目标）" : plan.goal);  // 显示目标，缺省时显示占位文本
        // 切换为激活状态样式（高亮显示）
        m_goalLabel->setStyleSheet(
            "QLabel {"
            "  color: #cbd5e1;"
            "  font-size: 12px;"
            "  padding: 8px 12px;"
            "  background: #312e81;"  // 深靛蓝背景，表示有任务
            "  border-radius: 6px;"
            "  border: 1px solid #4f46e5;"
            "}"
        );
    }

    // 重置进度条
    if (m_progressBar) {  // 安全检查
        m_progressBar->setValue(0);  // 重置为 0
    }

    int completedCount = 0;  // 统计已完成步骤数
    const int total = plan.steps.size();  // 总步骤数

    // 为每个步骤创建列表项
    for (int i = 0; i < plan.steps.size(); ++i) {
        const TaskStep& step = plan.steps[i];  // 获取步骤引用

        // 构造富文本：状态图标 + 步骤描述 + Agent→工具映射
        const QString icon = statusIcon(step.status);  // 状态图标
        const QString color = statusColor(step.status); // 状态颜色
        const QString agentTool = step.agent.isEmpty()
            ? QString()
            : QString(" · <span style='color:#94a3b8;'>%1 → %2</span>")
                  .arg(step.agent, step.tool);  // Agent→工具映射

        const QString html = QString(
            "<div style='padding:2px 0;'>"
            "  <span style='color:%1; font-size:14px;'>%2</span>"
            "  <span style='color:#f1f5f9; font-weight:500;'>第%3步：%4</span>"
            "  %5"
            "</div>"
        ).arg(color, icon)
         .arg(i + 1)  // 步骤编号（从1开始）
         .arg(step.description.toHtmlEscaped())  // 转义 HTML 特殊字符
         .arg(agentTool);

        QListWidgetItem* item = new QListWidgetItem(html);  // 创建列表项
        item->setData(Qt::UserRole, step.stepId);  // 存储 stepId 到 UserRole，用于点击事件识别
        item->setSizeHint(QSize(0, 38));  // 设置条目高度
        m_taskList->addItem(item);  // 添加到列表

        if (step.status == StepStatus::Completed) {  // 统计已完成
            completedCount++;
        }
    }

    // 更新进度条
    if (total > 0 && m_progressBar) {  // 避免除零
        m_progressBar->setValue(static_cast<int>(100.0 * completedCount / total));  // 设置百分比
    }
}

/**
 * @brief 更新指定步骤的状态
 * @param stepId 步骤编号
 * @param status 新状态
 */
void TaskListPanel::updateStepStatus(int stepId, StepStatus status)
{
    // 在 m_currentPlan 中查找匹配的步骤
    int foundIndex = -1;  // 找到的步骤索引
    for (int i = 0; i < m_currentPlan.steps.size(); ++i) {
        if (m_currentPlan.steps[i].stepId == stepId) {  // 匹配 stepId
            m_currentPlan.steps[i].status = status;     // 更新内部状态
            foundIndex = i;  // 记录索引
            break;  // 跳出循环
        }
    }

    if (foundIndex < 0) {  // 未找到匹配的步骤
        return;  // 静默返回
    }

    // 更新对应列表项的显示
    if (!m_taskList) return;  // 安全检查
    for (int row = 0; row < m_taskList->count(); ++row) {
        QListWidgetItem* item = m_taskList->item(row);  // 获取列表项
        if (item->data(Qt::UserRole).toInt() == stepId) {  // 找到匹配 stepId 的项
            const TaskStep& step = m_currentPlan.steps[foundIndex];  // 获取更新后的步骤

            // 重新构造富文本
            const QString icon = statusIcon(step.status);
            const QString color = statusColor(step.status);
            const QString agentTool = step.agent.isEmpty()
                ? QString()
                : QString(" · <span style='color:#94a3b8;'>%1 → %2</span>")
                      .arg(step.agent, step.tool);

            const QString html = QString(
                "<div style='padding:2px 0;'>"
                "  <span style='color:%1; font-size:14px;'>%2</span>"
                "  <span style='color:#f1f5f9; font-weight:500;'>第%3步：%4</span>"
                "  %5"
                "</div>"
            ).arg(color, icon)
             .arg(foundIndex + 1)
             .arg(step.description.toHtmlEscaped())
             .arg(agentTool);

            item->setText(html);  // 更新列表项文本
            break;  // 跳出循环
        }
    }

    // 重新计算并更新进度条
    int completedCount = 0;  // 统计已完成
    const int total = m_currentPlan.steps.size();  // 总数
    for (const TaskStep& s : m_currentPlan.steps) {
        if (s.status == StepStatus::Completed) {  // 累计已完成
            completedCount++;
        }
    }
    if (total > 0 && m_progressBar) {  // 避免除零
        m_progressBar->setValue(static_cast<int>(100.0 * completedCount / total));
    }
}

/**
 * @brief 清空当前计划显示
 */
void TaskListPanel::clearPlan()
{
    if (m_taskList) {  // 安全检查
        m_taskList->clear();  // 清空列表条目
    }

    // 重置目标标签为空状态
    if (m_goalLabel) {  // 安全检查
        m_goalLabel->setText("暂无任务");  // 显示占位文本
        m_goalLabel->setStyleSheet(  // 恢复默认样式
            "QLabel {"
            "  color: #94a3b8;"
            "  font-size: 12px;"
            "  padding: 6px 10px;"
            "  background: #1e293b;"
            "  border-radius: 4px;"
            "}"
        );
    }

    // 重置进度条
    if (m_progressBar) {  // 安全检查
        m_progressBar->setValue(0);  // 重置为 0%
    }

    m_currentPlan = TaskPlan();  // 重置为空的 TaskPlan 对象
}

/**
 * @brief QListWidget 条目点击事件处理槽函数
 * @param item 被点击的列表项指针
 */
void TaskListPanel::onItemClicked(QListWidgetItem* item)
{
    if (!item) {  // 异常情况：item 为空
        return;   // 直接返回
    }
    const int stepId = item->data(Qt::UserRole).toInt();  // 提取 stepId
    emit stepClicked(stepId);  // 发射 stepClicked 信号
}

/**
 * @brief 创建和布局所有 UI 子控件
 */
void TaskListPanel::setupUI()
{
    // ========== 创建主垂直布局 ==========
    m_mainLayout = new QVBoxLayout(this);  // 创建垂直布局，父对象为本组件
    m_mainLayout->setContentsMargins(12, 12, 12, 12);  // 设置四周内边距12像素
    m_mainLayout->setSpacing(10);  // 设置控件间距10像素
    this->setLayout(m_mainLayout);  // 应用布局到本组件

    // ========== 创建标题标签 ==========
    m_titleLabel = new QLabel("任务列表", this);  // 创建标题标签
    m_titleLabel->setStyleSheet(  // 设置标题样式
        "QLabel {"
        "  color: #f1f5f9;"        // 极浅灰白色
        "  font-size: 16px;"       // 16px 字号
        "  font-weight: 600;"      // 半加粗
        "  padding-bottom: 8px;"   // 底部8px间隙
        "}"
    );
    m_mainLayout->addWidget(m_titleLabel);  // 添加到主布局

    // ========== 创建目标描述标签 ==========
    m_goalLabel = new QLabel("暂无任务", this);  // 创建目标标签
    m_goalLabel->setWordWrap(true);  // 启用自动换行
    m_goalLabel->setStyleSheet(  // 设置默认样式
        "QLabel {"
        "  color: #94a3b8;"
        "  font-size: 12px;"
        "  padding: 6px 10px;"
        "  background: #1e293b;"
        "  border-radius: 4px;"
        "}"
    );
    m_mainLayout->addWidget(m_goalLabel);  // 添加到主布局

    // ========== 创建进度条 ==========
    m_progressBar = new QProgressBar(this);  // 创建进度条
    m_progressBar->setRange(0, 100);  // 范围 0-100（百分比）
    m_progressBar->setValue(0);  // 初始值 0
    m_progressBar->setTextVisible(false);  // 隐藏百分比文字
    m_progressBar->setFixedHeight(6);  // 固定高度 6px（细长）
    m_progressBar->setStyleSheet(  // 设置自定义 QSS 样式
        "QProgressBar {"
        "  background: #1e293b;"  // 背景色
        "  border: none;"
        "  border-radius: 3px;"  // 圆角
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #6366f1, stop:1 #8b5cf6);"  // 靛蓝到紫色渐变
        "  border-radius: 3px;"
        "}"
    );
    m_mainLayout->addWidget(m_progressBar);  // 添加到主布局

    // ========== 创建步骤列表 ==========
    m_taskList = new QListWidget(this);  // 创建 QListWidget
    m_taskList->setStyleSheet(  // 设置自定义 QSS 样式
        "QListWidget {"
        "  background: transparent;"   // 背景透明
        "  border: none;"              // 无边框
        "  outline: 0;"                // 无焦点虚线框
        "}"
        "QListWidget::item {"
        "  padding: 6px 8px;"          // 每项内边距
        "  border-radius: 4px;"        // 圆角
        "  margin: 1px 0;"             // 项之间小间距
        "}"
        "QListWidget::item:hover {"    // 悬停效果
        "  background: #1e293b;"
        "}"
        "QListWidget::item:selected {" // 选中效果
        "  background: #312e81;"
        "  color: #f1f5f9;"
        "}"
    );
    connect(m_taskList, &QListWidget::itemClicked,  // 绑定 itemClicked 信号
            this, &TaskListPanel::onItemClicked);   // 到本类的 onItemClicked 槽
    m_mainLayout->addWidget(m_taskList, 1);  // 添加到主布局，stretch=1（占据剩余空间）
}

/**
 * @brief 获取状态对应的图标字符
 * @param status 步骤状态
 * @return Unicode 图标字符串
 */
QString TaskListPanel::statusIcon(StepStatus status) const
{
    switch (status) {
        case StepStatus::Pending:   return "○";  // 空心圆：等待
        case StepStatus::Running:   return "◐";  // 半圆：执行中
        case StepStatus::Completed: return "✓";  // 对勾：成功
        case StepStatus::Failed:    return "✗";  // 叉号：失败
        default:                    return "?";  // 未知
    }
}

/**
 * @brief 获取状态对应的颜色代码
 * @param status 步骤状态
 * @return CSS 颜色字符串
 */
QString TaskListPanel::statusColor(StepStatus status) const
{
    switch (status) {
        case StepStatus::Pending:   return "#94a3b8";  // 灰色：等待
        case StepStatus::Running:   return "#6366f1";  // 靛蓝：执行中
        case StepStatus::Completed: return "#22c55e";  // 绿色：成功
        case StepStatus::Failed:    return "#ef4444";  // 红色：失败
        default:                    return "#cbd5e1";  // 浅灰：未知
    }
}
