// 【文件说明】computeragent.h - 系统命令Agent的头文件
// 【作用】声明 ComputerAgent 类，定义系统命令执行和系统信息获取的接口
// 【注意事项】头文件只声明类和函数，具体实现在 computeragent.cpp 中

#ifndef COMPUTERAGENT_H  // 防止重复包含的宏定义开始
#define COMPUTERAGENT_H  // 定义 COMPUTERAGENT_H 宏

#include "agent.h"   // 引入 Agent 基类头文件，ComputerAgent 继承自 Agent
#include <QList>     // 引入 Qt 列表容器，用于存储工具对象列表
#include <QProcess>  // 引入 Qt 进程类，用于执行外部系统命令

class Tool;  // 前向声明 Tool 类，告诉编译器有这个类，减少编译依赖

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
class ComputerAgent : public Agent  // ComputerAgent 继承自 Agent 基类
{
    Q_OBJECT  // Qt 宏，启用信号与槽、元对象系统等 Qt 特性

public:
    // 【功能说明】构造函数，创建系统命令代理实例
    // 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
    explicit ComputerAgent(QObject *parent = nullptr);

    // 【功能说明】析构函数，销毁系统命令代理实例
    // 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象
    ~ComputerAgent();

    // 【功能说明】获取代理名称
    // 【返回值】固定返回 QString 字符串 "ComputerAgent"
    // 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
    QString name() const override;

    // 【功能说明】获取代理描述
    // 【返回值】返回描述此 Agent 功能的 QString 字符串
    // 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
    QString description() const override;

    // 【功能说明】获取代理类型
    // 【返回值】返回枚举值 ComputerAgentType
    // 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
    AgentType type() const override;

    // 【功能说明】获取所有工具
    // 【返回值】返回 QList<Tool*> 类型的工具指针列表
    // 【说明】重写父类虚函数，返回此 Agent 注册的所有可用工具
    QList<Tool*> tools() const override;

    // 【功能说明】执行指定工具
    // 【参数说明】toolName - 要执行的工具名称字符串
    // 【参数说明】args - 工具参数字典，键值对形式
    // 【返回值】执行结果字典，包含 success（是否成功）和 result/error（结果或错误信息）
    // 【说明】重写父类虚函数，根据工具名称分发到对应的私有实现方法
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    // 【功能说明】初始化所有系统操作工具
    // 【说明】在构造函数中调用，创建所有这个Agent拥有的工具并注册到 m_tools 列表中
    void initializeTools();

    QList<Tool*> m_tools;  ///< 工具列表成员变量，存储此 Agent 拥有的所有 Tool 对象指针

    // 【功能说明】执行系统命令
    // 【参数说明】args - 包含 command（命令字符串）和 timeout（超时秒数，默认30）
    // 【返回值】命令执行的输出结果或错误信息
    QVariantMap runCommand(const QVariantMap& args);

    // 【功能说明】获取操作系统和硬件信息
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】系统信息字符串，包含 OS 名称、版本、内核、CPU 架构等
    QVariantMap getSystemInfo(const QVariantMap& args);

    // 【功能说明】列出当前运行的进程
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】进程列表文本
    QVariantMap listProcesses(const QVariantMap& args);

    // 【功能说明】终止指定的进程
    // 【参数说明】args - 包含 pid（进程ID）
    // 【返回值】终止结果字典
    QVariantMap killProcess(const QVariantMap& args);

    // 【功能说明】获取指定环境变量的值
    // 【参数说明】args - 包含 name（环境变量名称）
    // 【返回值】环境变量的值字符串
    QVariantMap getEnvironmentVariable(const QVariantMap& args);

    // 【功能说明】设置环境变量
    // 【参数说明】args - 包含 name（变量名）和 value（变量值）
    // 【返回值】设置结果字典
    QVariantMap setEnvironmentVariable(const QVariantMap& args);

    // 【功能说明】列出所有环境变量
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】环境变量列表文本
    QVariantMap listEnvironmentVariables(const QVariantMap& args);

    // 【功能说明】获取磁盘驱动器信息
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】磁盘信息文本
    QVariantMap getDiskInfo(const QVariantMap& args);

    // 【功能说明】获取内存使用情况
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】内存信息文本
    QVariantMap getMemoryInfo(const QVariantMap& args);

    // 【功能说明】获取网络接口信息
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】网络信息文本
    QVariantMap getNetworkInfo(const QVariantMap& args);

    // 【功能说明】列出系统服务状态
    // 【参数说明】args - 无参数（此工具无需参数）
    // 【返回值】服务列表文本
    QVariantMap listServices(const QVariantMap& args);

    // 【功能说明】启动系统服务
    // 【参数说明】args - 包含 name（服务名称）
    // 【返回值】启动结果字典
    QVariantMap startService(const QVariantMap& args);

    // 【功能说明】停止系统服务
    // 【参数说明】args - 包含 name（服务名称）
    // 【返回值】停止结果字典
    QVariantMap stopService(const QVariantMap& args);
};

#endif  // COMPUTERAGENT_H  // 宏定义结束，与开头的 #ifndef 配对
