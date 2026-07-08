#include "computeragent.h"
#include "tool.h"
#include <QProcess>
#include <QByteArray>
#include <QSysInfo>

ComputerAgent::ComputerAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

ComputerAgent::~ComputerAgent()
{
    qDeleteAll(m_tools);
}

QString ComputerAgent::name() const
{
    return "ComputerAgent";
}

QString ComputerAgent::description() const
{
    return "系统命令代理，负责执行系统命令和获取系统信息";
}

Agent::AgentType ComputerAgent::type() const
{
    return ComputerAgentType;
}

QList<Tool*> ComputerAgent::tools() const
{
    return m_tools;
}

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

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

QVariantMap ComputerAgent::runCommand(const QVariantMap& args)
{
    QVariantMap result;
    QString command = args.value("command").toString();
    int timeout = args.value("timeout", 30).toInt();

    QProcess process;
    process.start(command);
    
    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法启动命令";
        return result;
    }

    if (!process.waitForFinished(timeout * 1000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "命令执行超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(process.readAllStandardError());

    if (process.exitCode() != 0) {
        result["success"] = false;
        result["error"] = error.isEmpty() ? "命令执行失败" : error;
        return result;
    }

    result["success"] = true;
    result["result"] = output;
    return result;
}

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
    
    if (!error.isEmpty()) {
        result["success"] = false;
        result["error"] = error;
        return result;
    }

    result["success"] = true;
    result["result"] = QString("进程 %1 已终止").arg(pid);
    return result;
}
