#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "agent.h"
#include "llmclient.h"
#include "promptbuilder.h"
#include <QList>
#include <QString>

/**
 * @brief 总控调度Agent - 系统的大脑，负责协调所有其他Agent
 *
 * Orchestrator是整个系统的核心调度者。它的工作流程如下：
 * 1. 接收用户的问题
 * 2. 将问题和所有可用Agent/工具的描述一起发送给大模型
 * 3. 分析大模型的回复，判断是否需要调用工具
 * 4. 如果需要调用工具，找到对应的Agent并执行
 * 5. 将工具执行结果返回给大模型，让它继续分析
 * 6. 重复上述过程，直到问题解决
 *
 * 这个类实现了"思考-行动"循环（ReAct模式），让大模型能够
 * 通过调用外部工具来完成复杂的任务。
 */
class Orchestrator : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit Orchestrator(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Orchestrator();

    /**
     * @brief 获取Agent名称
     * @return "Orchestrator"
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 总控Agent的功能描述
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return OrchestratorType
     */
    AgentType type() const override;

    /**
     * @brief 获取所有可用工具（汇总所有子Agent的工具）
     * @return 所有工具的列表
     */
    QList<Tool*> tools() const override;

    /**
     * @brief 执行工具（委派给对应的子Agent）
     * @param toolName 工具名称
     * @param args 工具参数
     * @return 执行结果
     */
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

    /**
     * @brief 设置LLM客户端
     * @param client LLM客户端指针对象
     */
    void setLlmClient(LlmClient* client);

    /**
     * @brief 添加一个子Agent到调度系统
     * @param agent 要添加的Agent指针对象
     */
    void addAgent(Agent* agent);

    /**
     * @brief 处理用户查询 - 主入口函数
     * @param query 用户的问题或指令
     */
    void processQuery(const QString& query);

signals:
    /**
     * @brief 收到AI消息信号
     * @param message 消息内容
     */
    void messageReceived(const QString& message);

    /**
     * @brief 选中了某个Agent/工具信号
     * @param agentName Agent或工具名称
     */
    void agentSelected(const QString& agentName);

private slots:
    /**
     * @brief LLM响应到达的槽函数
     * @param response LLM返回的文本
     */
    void onLlmResponse(const QString& response);

    /**
     * @brief LLM出错的槽函数
     * @param error 错误信息
     */
    void onLlmError(const QString& error);

private:
    /**
     * @brief 从LLM响应中解析工具调用
     * @param response LLM返回的文本
     * @return 工具调用字符串，如果没有工具调用则返回空
     */
    QString parseToolCall(const QString& response);

    /**
     * @brief 从工具调用字符串中提取工具名称
     * @param toolCall 工具调用字符串
     * @return 工具名称
     */
    QString extractToolName(const QString& toolCall);

    /**
     * @brief 从工具调用字符串中提取参数
     * @param toolCall 工具调用字符串
     * @return 参数键值对
     */
    QVariantMap extractToolArgs(const QString& toolCall);

    /**
     * @brief 查找拥有指定工具的Agent
     * @param toolName 工具名称
     * @return Agent指针，找不到返回nullptr
     */
    Agent* findAgentForTool(const QString& toolName);

    LlmClient* m_llmClient;       ///< LLM客户端指针
    QList<Agent*> m_agents;        ///< 所有子Agent列表
    PromptBuilder m_promptBuilder; ///< Prompt构建器
    QString m_currentQuery;        ///< 当前正在处理的查询
    bool m_isProcessing;           ///< 是否正在处理中
};

#endif
