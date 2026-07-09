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

    /**
     * @brief 获取环境变量
     * @param args 包含name参数
     * @return 环境变量值
     */
    QVariantMap getEnv(const QVariantMap& args);

    /**
     * @brief 设置环境变量
     * @param args 包含name、value参数
     * @return 设置结果
     */
    QVariantMap setEnv(const QVariantMap& args);

    /**
     * @brief 列出所有环境变量
     * @param args 无参数
     * @return 所有环境变量列表
     */
    QVariantMap listEnv(const QVariantMap& args);

    /**
     * @brief 获取磁盘信息
     * @param args 包含path参数（可选，默认/）
     * @return 磁盘使用情况信息
     */
    QVariantMap diskInfo(const QVariantMap& args);

    /**
     * @brief 获取内存信息
     * @param args 无参数
     * @return 内存使用情况信息
     */
    QVariantMap memoryInfo(const QVariantMap& args);

    /**
     * @brief 获取网络信息
     * @param args 无参数
     * @return 网络接口和连接信息
     */
    QVariantMap networkInfo(const QVariantMap& args);

    /**
     * @brief 检查端口是否被占用
     * @param args 包含port参数
     * @return 端口占用情况
     */
    QVariantMap checkPort(const QVariantMap& args);

    /**
     * @brief Ping指定主机
     * @param args 包含host、count参数
     * @return Ping结果
     */
    QVariantMap ping(const QVariantMap& args);

    /**
     * @brief 获取当前工作目录
     * @param args 无参数
     * @return 当前工作目录路径
     */
    QVariantMap currentDirectory(const QVariantMap& args);

    /**
     * @brief 改变当前工作目录
     * @param args 包含path参数
     * @return 切换结果
     */
    QVariantMap changeDirectory(const QVariantMap& args);

    /**
     * @brief 获取CPU信息
     * @param args 无参数
     * @return CPU详细信息
     */
    QVariantMap cpuInfo(const QVariantMap& args);

    /**
     * @brief 计算文件的MD5哈希
     * @param args 包含path参数
     * @return MD5哈希值
     */
    QVariantMap fileHash(const QVariantMap& args);
};

#endif
