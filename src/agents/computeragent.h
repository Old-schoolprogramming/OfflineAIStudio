#ifndef COMPUTERAGENT_H
#define COMPUTERAGENT_H

#include "agent.h"
#include <QList>
#include <QProcess>

class Tool;

/**
 * @brief 系统命令Agent - 负责执行系统命令和获取系统信息
 *
 * ComputerAgent是一个系统操作专家，它可以：
 * - 执行任意系统命令（如dir、ls、ipconfig等）
 * - 获取操作系统信息
 * - 查看当前运行的进程列表
 * - 终止指定的进程
 *
 * 注意：由于涉及系统命令执行，使用时需要注意安全风险。
 * 实际生产环境中应该对可执行的命令进行白名单限制。
 */
class ComputerAgent : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ComputerAgent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ComputerAgent();

    /**
     * @brief 获取Agent名称
     * @return "ComputerAgent"
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 系统命令Agent的功能描述
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return ComputerAgentType
     */
    AgentType type() const override;

    /**
     * @brief 获取所有工具
     * @return 工具列表
     */
    QList<Tool*> tools() const override;

    /**
     * @brief 执行指定工具
     * @param toolName 工具名称
     * @param args 工具参数
     * @return 执行结果
     */
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    /**
     * @brief 初始化所有工具
     *
     * 在构造函数中调用，创建所有这个Agent拥有的工具。
     */
    void initializeTools();

    QList<Tool*> m_tools; ///< 工具列表

    /**
     * @brief 执行系统命令
     * @param args 包含command和timeout参数
     * @return 命令执行的输出结果
     */
    QVariantMap runCommand(const QVariantMap& args);

    /**
     * @brief 获取系统信息
     * @param args （无参数）
     * @return 系统信息（OS名称、版本、内核、CPU架构等）
     */
    QVariantMap getSystemInfo(const QVariantMap& args);

    /**
     * @brief 列出当前运行的进程
     * @param args （无参数）
     * @return 进程列表文本
     */
    QVariantMap listProcesses(const QVariantMap& args);

    /**
     * @brief 终止指定的进程
     * @param args 包含pid参数
     * @return 终止结果
     */
    QVariantMap killProcess(const QVariantMap& args);
};

#endif
