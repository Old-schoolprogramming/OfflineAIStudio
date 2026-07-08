#include "agentselector.h"
#include "agent.h"

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

AgentSelector::~AgentSelector()
{
}

void AgentSelector::addAgent(Agent* agent)
{
    QListWidgetItem* item = new QListWidgetItem(agent->name(), m_agentList);
    item->setToolTip(agent->description());
    item->setData(Qt::UserRole, QVariant::fromValue(agent));
}

void AgentSelector::clearAgents()
{
    m_agentList->clear();
}

QString AgentSelector::selectedAgentName() const
{
    QListWidgetItem* item = m_agentList->currentItem();
    if (item) {
        return item->text();
    }
    return "";
}

void AgentSelector::onItemClicked(QListWidgetItem* item)
{
    if (item) {
        emit agentSelected(item->text());
    }
}
