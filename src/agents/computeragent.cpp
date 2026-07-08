/**
 * @file computeragent.cpp
 * @brief 系统命令代理实现
 *
 * @details
 * ComputerAgent是Agent基类的具体实现，负责执行系统命令和获取系统信息。
 * 该代理注册了4个工具：runCommand、getSystemInfo、listProcesses、killProcess。
 * 工具实现使用QProcess执行外部命令，并通过条件编译处理Windows和Unix-like系统的差异。
 * 所有工具执行结果以QVariantMap形式返回，统一包含success和result/error字段。
 */

#include "computeragent.h"
#include "tool.h"
#include <QProcess>
#include <QByteArray>
#include <QSysInfo>

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 调用initializeTools()创建并注册所有系统操作工具。
 */
ComputerAgent::ComputerAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

/**
 * @brief 析构函数
 *
 * 释放initializeTools中动态分配的所有Tool对象。
 */
ComputerAgent::~ComputerAgent()
{
    qDeleteAll(m_tools);
}

/**
 * @brief 获取代理名称
 * @return 固定返回"ComputerAgent"
 */
QString ComputerAgent::name() const
{
    return "ComputerAgent";
}

/**
 * @brief 获取代理描述
 * @return 代理功能描述字符串
 */
QString ComputerAgent::description() const
{
    return "系统命令代理，负责执行系统命令和获取系统信息";
}

/**
 * @brief 获取代理类型
 * @return 固定返回ComputerAgentType
 */
Agent::AgentType ComputerAgent::type() const
{
    return ComputerAgentType;
}

/**
 * @brief 获取工具列表
 * @return 当前代理注册的所有Tool指针列表
 */
QList<Tool*> ComputerAgent::tools() const
{
    return m_tools;
}

/**
 * @brief 初始化并注册所有系统操作工具
 *
 * @details
 * 注册的工具及参数说明：
 * - runCommand: 执行系统命令，必填参数command，可选参数timeout（默认30秒）
 * - getSystemInfo: 获取操作系统和硬件信息，无参数
 * - listProcesses: 列出当前运行进程，无参数
 * - killProcess: 终止指定进程，必填参数pid
 */
void ComputerAgent::initializeTools()
{
    Tool* runCommandTool = new Tool("runCommand", "执行系统命令");
    runCommandTool->addParameter("command", "string", "要执行的命令", true);
    runCommandTool->addParameter("timeout", "int", "超时时间(秒)", false, "30");
    m_tools.append(runCommandTool);

    Tool* systemInfoTool = new Tool("getSystemInfo", "获取系统信息");
    m_tools.append(systemInfoTool);

    Tool* listProcessesTool = new Tool("listProcesses", "列出当前进程");
    m_tools.append(listProcessesTool);

    Tool* killProcessTool = new Tool("killProcess", "终止进程");
    killProcessTool->addParameter("pid", "int", "进程ID", true);
    m_tools.append(killProcessTool);
}

/**
 * @brief 工具分发执行入口
 * @param toolName 要执行的工具名称
 * @param args 工具参数字典（键值对）
 * @return 执行结果，包含success字段和result或error字段
 *
 * @details
 * 根据toolName将请求分派到对应的私有实现方法。
 * 若toolName不匹配任何已知工具，返回错误结果。
 */
QVariantMap ComputerAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "runCommand") {
        return runCommand(args);
    } else if (toolName == "getSystemInfo") {
        return getSystemInfo(args);
    } else if (toolName == "listProcesses") {
        return listProcesses(args);
    } else if (toolName == "killProcess") {
        return killProcess(args);
    }

    // 未知工具返回通用错误
    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

/**
 * @brief 执行系统命令
 * @param args 参数映射，必须包含"command"，可选"timeout"（默认30秒）
 * @return 若成功，result字段为标准输出内容；若失败或超时，error字段包含原因
 *
 * @details
 * 执行流程：
 * 1. 提取command和timeout参数
 * 2. 使用QProcess启动命令
 * 3. 等待进程启动，失败则返回错误
 * 4. 等待进程完成（带超时），超时则强制终止并返回错误
 * 5. 读取标准输出和标准错误
 * 6. 若退出码非0，将标准错误作为错误信息返回
 * 7. 否则返回标准输出内容
 */
QVariantMap ComputerAgent::runCommand(const QVariantMap& args)
{
    QVariantMap result;
    QString command = args.value("command").toString();
    int timeout = args.value("timeout", 30).toInt();

    QProcess process;
    process.start(command);
    
    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法启动命令";
        return result;
    }

    // 等待进程结束，超时则强制终止
    if (!process.waitForFinished(timeout * 1000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "命令执行超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(process.readAllStandardError());

    // 非零退出码视为执行失败
    if (process.exitCode() != 0) {
        result["success"] = false;
        result["error"] = error.isEmpty() ? "命令执行失败" : error;
        return result;
    }

    result["success"] = true;
    result["result"] = output;
    return result;
}

/**
 * @brief 获取系统信息
 * @param args 空参数（此工具无需参数）
 * @return 固定success=true，result字段包含操作系统、版本、内核、CPU架构等信息
 *
 * @details
 * 使用QSysInfo静态方法收集系统信息，包括：
 * - prettyProductName: 操作系统名称
 * - productVersion: 操作系统版本
 * - kernelType: 内核类型（如winnt、linux、darwin）
 * - kernelVersion: 内核版本
 * - currentCpuArchitecture: CPU架构（如x86_64、arm64）
 */
QVariantMap ComputerAgent::getSystemInfo(const QVariantMap& args)
{
    QVariantMap result;

    QString osName = QSysInfo::prettyProductName();
    QString osVersion = QSysInfo::productVersion();
    QString kernelType = QSysInfo::kernelType();
    QString kernelVersion = QSysInfo::kernelVersion();
    QString cpuArch = QSysInfo::currentCpuArchitecture();

    QString info = QString("操作系统: %1\n版本: %2\n内核类型: %3\n内核版本: %4\nCPU架构: %5")
        .arg(osName, osVersion, kernelType, kernelVersion, cpuArch);

    result["success"] = true;
    result["result"] = info;
    return result;
}

/**
 * @brief 列出当前运行进程
 * @param args 空参数（此工具无需参数）
 * @return 若成功，result字段为进程列表文本；若失败或超时，error字段包含原因
 *
 * @details
 * 跨平台实现：
 * - Windows: 执行tasklist命令
 * - Unix-like（Linux/macOS）: 执行ps aux命令
 *
 * 命令输出通过QProcess::readAllStandardOutput读取，使用本地8位编码转换。
 */
QVariantMap ComputerAgent::listProcesses(const QVariantMap& args)
{
    QVariantMap result;

    QProcess process;
#ifdef Q_OS_WIN
    process.start("tasklist");
#else
    process.start("ps", QStringList() << "aux");
#endif

    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法获取进程列表";
        return result;
    }

    if (!process.waitForFinished(5000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "获取进程列表超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());

    result["success"] = true;
    result["result"] = output;
    return result;
}

/**
 * @brief 终止指定进程
 * @param args 参数映射，必须包含"pid"键（进程ID）
 * @return 若成功，result字段包含终止成功提示；若失败或超时，error字段包含原因
 *
 * @details
 * 跨平台实现：
 * - Windows: 执行taskkill /F /PID <pid>
 * - Unix-like: 执行kill -9 <pid>
 *
 * 通过读取标准错误判断命令是否成功执行。
 * 若标准错误非空，则将其内容作为错误信息返回。
 */
QVariantMap ComputerAgent::killProcess(const QVariantMap& args)
{
    QVariantMap result;
    int pid = args.value("pid").toInt();

    QProcess process;
#ifdef Q_OS_WIN
    process.start("taskkill", QStringList() << "/F" << "/PID" << QString::number(pid));
#else
    process.start("kill", QStringList() << "-9" << QString::number(pid));
#endif

    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法终止进程";
        return result;
    }

    if (!process.waitForFinished(5000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "终止进程超时";
        return result;
    }

    QString error = QString::fromLocal8Bit(process.readAllStandardError());
    
    // 若标准错误有输出，视为终止失败
    if (!error.isEmpty()) {
        result["success"] = false;
        result["error"] = error;
        return result;
    }

    result["success"] = true;
    result["result"] = QString("进程 %1 已终止").arg(pid);
    return result;
}
