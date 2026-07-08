/**
 * @file tasklistpanel.cpp
 * @brief 任务列表面板实现
 *
 * @details
 * TaskListPanel 的实现基于标准的 Qt Widgets：
 * - QLabel: 展示标题和目标描述
 * - QProgressBar: 展示执行进度
 * - QListWidget: 展示步骤列表（使用 HTML 富文本渲染）
 *
 * 核心设计要点：
 * 1. 每个步骤使用 QListWidgetItem 的 setText() 方法设置 HTML 富文本
 *    这样可以实现状态图标、颜色编码等视觉效果
 * 2. 步骤编号存储在 item 的 Qt::UserRole 数据中，用于点击事件识别
 * 3. updateStepStatus() 通过 stepId 匹配更新，而非索引，确保健壮性
 * 4. 进度条百分比 = completedSteps / totalSteps * 100
 */

#include "tasklistpanel.h"
#include <QDateTime>

/**
 * @brief 构造函数
 * @param parent 父 QWidget
 *
 * @implementation
 * 调用 setupUI() 完成所有子控件的创建和布局。
 */
TaskListPanel::TaskListPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

/**
 * @brief 析构函数
 *
 * @note 所有子控件由 Qt 父对象机制自动析构，无需手动 delete。
 */
TaskListPanel::~TaskListPanel()
{
}

/**
 * @brief 创建和布局所有 UI 子控件
 *
 * @implementation
 * 控件层次结构（垂直布局）：
 * m_mainLayout (QVBoxLayout)
 *   ├── m_titleLabel    — "任务列表" 标题（16px, 粗体, 白色）
 *   ├── m_goalLabel     — 计划目标描述（默认"暂无任务"）
 *   ├── m_progressBar   — 进度条（0-100，渐变紫色填充）
 *   └── m_taskList      — 步骤列表（自定义 QSS 样式）
 *
 * 样式设计：
 * - 整体深色主题，与主窗口协调
 * - 标题底部有 8px 内边距
 * - 目标标签使用 #1e293b 背景色 + 圆角边框
 * - 进度条使用 qlineargradient 实现 indigo → violet 渐变
 * - 列表项有悬停和选中效果
 */
void TaskListPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(10);

    // 标题标签
    m_titleLabel = new QLabel("任务列表", this);
    m_titleLabel->setObjectName("taskListTitle");
    m_titleLabel->setStyleSheet(
        "color: #f1f5f9;"
        "font-size: 16px;"
        "font-weight: 600;"
        "padding-bottom: 8px;"
    );
    m_mainLayout->addWidget(m_titleLabel);

    // 目标描述标签（初始状态："暂无任务"）
    m_goalLabel = new QLabel("暂无任务", this);
    m_goalLabel->setObjectName("taskGoal");
    m_goalLabel->setWordWrap(true);  // 允许自动换行
    m_goalLabel->setStyleSheet(
        "color: #94a3b8;"
        "font-size: 13px;"
        "padding: 8px 12px;"
        "background-color: #1e293b;"
        "border-radius: 6px;"
    );
    m_mainLayout->addWidget(m_goalLabel);

    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setObjectName("taskProgressBar");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("进度: %v%");
    m_progressBar->setTextVisible(true);
    // 进度条样式：使用 qlineargradient 实现 indigo → violet 渐变填充
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "   border: none;"
        "   border-radius: 6px;"
        "   background-color: #1e293b;"
        "   text-align: center;"
        "   color: #f1f5f9;"
        "   font-size: 12px;"
        "   height: 20px;"
        "}"
        "QProgressBar::chunk {"
        "   border-radius: 6px;"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "       stop:0 #6366f1, stop:1 #8b5cf6);"
        "}"
    );
    m_mainLayout->addWidget(m_progressBar);

    // 步骤列表
    m_taskList = new QListWidget(this);
    m_taskList->setObjectName("taskList");
    m_taskList->setSpacing(4);
    m_taskList->setWordWrap(true);
    m_taskList->setStyleSheet(
        "QListWidget {"
        "   background-color: #0f172a;"
        "   color: #f1f5f9;"
        "   border: 1px solid #334155;"
        "   border-radius: 8px;"
        "   padding: 6px;"
        "}"
        "QListWidget::item {"
        "   padding: 12px 14px;"
        "   margin: 3px 0px;"
        "   border-radius: 8px;"
        "   border: 1px solid transparent;"
        "   min-height: 48px;"
        "}"
        "QListWidget::item:hover {"
        "   background-color: #1e293b;"
        "   border-color: #475569;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #1e293b;"
        "   border-color: #6366f1;"
        "   color: #f1f5f9;"
        "}"
    );
    m_mainLayout->addWidget(m_taskList);

    // 连接点击事件
    connect(m_taskList, &QListWidget::itemClicked, this, &TaskListPanel::onItemClicked);
}

/**
 * @brief 设置当前显示的计划
 * @param plan 解析后的 TaskPlan
 *
 * @implementation
 * 1. 保存 plan 副本
 * 2. 清空现有列表项
 * 3. 更新目标标签：显示计划目标，更新背景样式为激活状态
 * 4. 遍历 plan.steps，为每个步骤创建 QListWidgetItem
 *    每个 item 使用 HTML 富文本，包含：
 *    - 状态图标（通过 statusIcon() 获取）
 *    - 步骤编号和描述（彩色，通过 statusColor() 获取）
 *    - Agent → 工具映射（灰色小字）
 *    - stepId 存储在 Qt::UserRole 中
 * 5. 计算并设置进度条初始值
 */
void TaskListPanel::setPlan(const TaskPlan& plan)
{
    m_currentPlan = plan;
    m_taskList->clear();

    // 更新目标标签为激活状态样式
    m_goalLabel->setText("目标: " + plan.goal);
    m_goalLabel->setStyleSheet(
        "color: #e2e8f0;"
        "font-size: 13px;"
        "padding: 10px 12px;"
        "background-color: #1e293b;"
        "border: 1px solid #334155;"
        "border-radius: 8px;"
    );

    // 为每个步骤创建列表项
    for (const auto& step : plan.steps) {
        QListWidgetItem* item = new QListWidgetItem(m_taskList);
        item->setData(Qt::UserRole, step.stepId);
        item->setSizeHint(QSize(0, 56));

        QString icon = statusIcon(step.status);
        QString text = QString("%1  步骤 %2: %3\n      %4 → %5")
            .arg(icon)
            .arg(step.stepId)
            .arg(step.description)
            .arg(step.agent)
            .arg(step.tool);

        item->setText(text);

        QFont font;
        font.setPointSize(13);
        item->setFont(font);

        QColor color(statusColor(step.status));
        item->setForeground(color);

        m_taskList->addItem(item);
    }

    // 计算初始进度
    int progress = plan.totalSteps() > 0
        ? (plan.completedSteps() * 100 / plan.totalSteps())
        : 0;
    m_progressBar->setValue(progress);
}

/**
 * @brief 更新指定步骤的状态
 * @param stepId 要更新的步骤编号
 * @param status 新状态
 *
 * @implementation
 * 遍历 m_currentPlan.steps 查找匹配的 stepId：
 * 1. 更新步骤的内部状态
 * 2. 重新生成对应 QListWidgetItem 的 HTML 文本（新的图标和颜色）
 * 3. 重新计算并更新进度条
 *
 * 如果找不到匹配项，静默返回。
 */
void TaskListPanel::updateStepStatus(int stepId, StepStatus status)
{
    for (int i = 0; i < m_currentPlan.steps.size(); ++i) {
        if (m_currentPlan.steps[i].stepId == stepId) {
            // 更新内部状态
            m_currentPlan.steps[i].status = status;

            // 更新 UI 列表项
            QListWidgetItem* item = m_taskList->item(i);
            if (item) {
                const auto& step = m_currentPlan.steps[i];
                QString icon = statusIcon(status);
                QString text = QString("%1  步骤 %2: %3\n      %4 → %5")
                    .arg(icon)
                    .arg(step.stepId)
                    .arg(step.description)
                    .arg(step.agent)
                    .arg(step.tool);

                item->setText(text);
                QColor color(statusColor(status));
                item->setForeground(color);
            }

            break;
        }
    }

    // 重新计算进度
    int progress = m_currentPlan.totalSteps() > 0
        ? (m_currentPlan.completedSteps() * 100 / m_currentPlan.totalSteps())
        : 0;
    m_progressBar->setValue(progress);
}

/**
 * @brief 清空当前计划显示
 *
 * @implementation
 * 将 TaskListPanel 恢复到初始空状态：
 * 1. 清空 QListWidget
 * 2. 重置目标标签为"暂无任务"和默认样式
 * 3. 重置进度条为 0
 * 4. 清空 m_currentPlan
 */
void TaskListPanel::clearPlan()
{
    m_taskList->clear();
    m_goalLabel->setText("暂无任务");
    m_goalLabel->setStyleSheet(
        "color: #94a3b8;"
        "font-size: 13px;"
        "padding: 8px 12px;"
        "background-color: #1e293b;"
        "border-radius: 6px;"
    );
    m_progressBar->setValue(0);
    m_currentPlan = TaskPlan();
}

/**
 * @brief 获取状态对应的图标字符
 * @param status 步骤状态
 * @return Unicode 图标字符
 *
 * 图标映射：
 * - Pending:   ○ (空心圆，等待中)
 * - Running:   ◐ (半圆，执行中)
 * - Completed: ✓ (对勾，成功)
 * - Failed:    ✗ (叉号，失败)
 * - Skipped:   ⊘ (禁止符，跳过)
 */
QString TaskListPanel::statusIcon(StepStatus status) const
{
    switch (status) {
        case StepStatus::Pending:   return "○";
        case StepStatus::Running:   return "◐";
        case StepStatus::Completed: return "✓";
        case StepStatus::Failed:    return "✗";
        case StepStatus::Skipped:   return "⊘";
    }
    return "○";
}

/**
 * @brief 获取状态对应的颜色代码
 * @param status 步骤状态
 * @return CSS 颜色字符串
 *
 * 颜色映射：
 * - Pending:   #94a3b8 (slate-400，灰色，表示等待)
 * - Running:   #6366f1 (indigo-500，紫色，表示执行中)
 * - Completed: #22c55e (green-500，绿色，表示成功)
 * - Failed:    #ef4444 (red-500，红色，表示失败)
 * - Skipped:   #64748b (slate-500，灰色，表示跳过)
 */
QString TaskListPanel::statusColor(StepStatus status) const
{
    switch (status) {
        case StepStatus::Pending:   return "#94a3b8";
        case StepStatus::Running:   return "#6366f1";
        case StepStatus::Completed: return "#22c55e";
        case StepStatus::Failed:    return "#ef4444";
        case StepStatus::Skipped:   return "#64748b";
    }
    return "#94a3b8";
}

/**
 * @brief 列表项点击事件处理
 * @param item 被点击的 QListWidgetItem
 *
 * @implementation
 * 从 item 的 Qt::UserRole 数据中提取 stepId，发射 stepClicked 信号。
 * 如果 item 为空，不做任何操作。
 */
void TaskListPanel::onItemClicked(QListWidgetItem* item)
{
    if (item) {
        int stepId = item->data(Qt::UserRole).toInt();
        emit stepClicked(stepId);
    }
}
