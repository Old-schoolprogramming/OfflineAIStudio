/**
 * @file agentselector.cpp
 * @brief Agent 选择器控件实现
 *
 * @details
 * AgentSelector 提供可用 Agent 的列表展示和选择功能，
 * 基于 QListWidget 实现，每项显示 Agent 名称，悬停显示描述。
 *
 * 核心设计要点：
 * 1. Agent 指针存储在 QListWidgetItem 的 Qt::UserRole 数据中，便于后续获取
 * 2. 点击列表项时发射 agentSelected 信号，传递 Agent 名称
 * 3. 支持动态添加和清空 Agent 列表
 * 4. 提供 selectedAgentName() 查询当前选中项
 */

#include "agentselector.h"
#include "agent.h"

/**
 * @brief 构造函数
 * @param parent 父 QWidget
 *
 * @implementation
 * 布局结构：
 * m_mainLayout (QVBoxLayout)
 *   ├── m_titleLabel — "可用 Agent"标题
 *   └── m_agentList — QListWidget（Agent 列表）
 *
 * 信号连接：列表项点击 → onItemClicked()
 */
AgentSelector::AgentSelector(QWidget *parent)
    : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);

    m_titleLabel = new QLabel("可用 Agent", this);
    m_titleLabel->setObjectName("agentTitle");
    m_mainLayout->addWidget(m_titleLabel);

    m_agentList = new QListWidget(this);
    m_agentList->setObjectName("agentList");
    m_mainLayout->addWidget(m_agentList);

    connect(m_agentList, &QListWidget::itemClicked, this, &AgentSelector::onItemClicked);
}

/**
 * @brief 析构函数
 */
AgentSelector::~AgentSelector()
{
}

/**
 * @brief 向列表中添加一个 Agent
 * @param agent Agent 对象指针（非空）
 *
 * @implementation
 * 1. 创建 QListWidgetItem，文本设为 agent->name()
 * 2. 设置工具提示为 agent->description()，鼠标悬停时显示
 * 3. 将 agent 指针存入 item 的 Qt::UserRole 数据，供外部通过 item 反查
 */
void AgentSelector::addAgent(Agent* agent)
{
    QListWidgetItem* item = new QListWidgetItem(agent->name(), m_agentList);
    item->setToolTip(agent->description());
    item->setData(Qt::UserRole, QVariant::fromValue(agent));
}

/**
 * @brief 清空所有 Agent 列表项
 */
void AgentSelector::clearAgents()
{
    m_agentList->clear();
}

/**
 * @brief 获取当前选中的 Agent 名称
 * @return 选中项的文本；若未选中则返回空字符串
 */
QString AgentSelector::selectedAgentName() const
{
    QListWidgetItem* item = m_agentList->currentItem();
    if (item) {
        return item->text();
    }
    return "";
}

/**
 * @brief 列表项点击事件处理
 * @param item 被点击的 QListWidgetItem
 *
 * @implementation
 * 若 item 非空，发射 agentSelected(item->text()) 信号，
 * 通知外部当前选中的 Agent 名称已变更。
 */
void AgentSelector::onItemClicked(QListWidgetItem* item)
{
    if (item) {
        emit agentSelected(item->text());
    }
}
