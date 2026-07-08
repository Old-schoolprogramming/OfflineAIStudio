#ifndef AGENT_H
#define AGENT_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QObject>

class Tool;

/**
 * @brief Agent基类 - 所有智能代理的抽象基类
 *
 * Agent是系统中的核心概念，每个Agent都有自己的专长领域和工具集。
 * 例如：FileAgent擅长文件操作，ComputerAgent擅长系统命令。
 * 所有具体的Agent都必须继承这个基类并实现纯虚函数。
 */
class Agent : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Agent类型枚举 - 用于区分不同种类的Agent
     */
    enum AgentType {
        FileAgentType,       ///< 文件操作Agent
        ComputerAgentType,   ///< 系统命令Agent
        OrchestratorType     ///< 总控调度Agent
    };

    /**
     * @brief 构造函数
     * @param parent 父对象，用于Qt内存管理
     */
    explicit Agent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    virtual ~Agent();

    /**
     * @brief 获取Agent名称
     * @return Agent的名称字符串
     */
    virtual QString name() const = 0;

    /**
     * @brief 获取Agent描述
     * @return Agent的功能描述文字
     */
    virtual QString description() const = 0;

    /**
     * @brief 获取Agent类型
     * @return Agent的枚举类型
     */
    virtual AgentType type() const = 0;

    /**
     * @brief 获取Agent拥有的所有工具
     * @return 工具指针列表
     */
    virtual QList<Tool*> tools() const = 0;

    /**
     * @brief 执行指定的工具
     * @param toolName 工具名称
     * @param args 工具参数（键值对形式）
     * @return 执行结果，包含success标志和result/error信息
     */
    virtual QVariantMap executeTool(const QString& toolName, const QVariantMap& args) = 0;

    /**
     * @brief 将工具列表格式化为Prompt文本
     * @return 格式化后的工具描述文本，用于输入给大模型
     */
    QString formatToolsForPrompt() const;

signals:
    /**
     * @brief 工具执行开始信号
     * @param agentName 执行的Agent名称
     */
    void executionStarted(const QString& agentName);

    /**
     * @brief 工具执行完成信号
     * @param agentName 执行的Agent名称
     * @param result 执行结果
     */
    void executionCompleted(const QString& agentName, const QVariantMap& result);

    /**
     * @brief 工具执行出错信号
     * @param agentName 执行的Agent名称
     * @param error 错误信息
     */
    void executionError(const QString& agentName, const QString& error);
};

#endif
