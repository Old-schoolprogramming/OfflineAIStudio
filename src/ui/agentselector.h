#ifndef AGENTSELECTOR_H
#define AGENTSELECTOR_H

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>

class Agent;

/**
 * @brief Agent选择器组件 - 显示并选择可用的Agent
 *
 * 这个组件用列表形式展示所有可用的Agent，
 * 用户可以点击选择某个Agent查看详情。
 * 同时也用于在侧边栏展示系统中有哪些智能代理可用。
 */
class AgentSelector : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit AgentSelector(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AgentSelector();

    /**
     * @brief 添加一个Agent到列表
     * @param agent Agent指针对象
     */
    void addAgent(Agent* agent);

    /**
     * @brief 清空Agent列表
     */
    void clearAgents();

    /**
     * @brief 获取当前选中的Agent名称
     * @return Agent名称，未选中则返回空
     */
    QString selectedAgentName() const;

signals:
    /**
     * @brief Agent被选中的信号
     * @param agentName 选中的Agent名称
     */
    void agentSelected(const QString& agentName);

private slots:
    /**
     * @brief 列表项被点击的槽函数
     * @param item 被点击的列表项
     */
    void onItemClicked(QListWidgetItem* item);

private:
    QListWidget* m_agentList;       ///< Agent列表控件
    QLabel* m_titleLabel;           ///< 标题标签
    QVBoxLayout* m_mainLayout;      ///< 主布局
};

#endif
