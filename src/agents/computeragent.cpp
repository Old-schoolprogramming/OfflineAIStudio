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
#include <QRegularExpression>
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

    Tool* getEnvVarTool = new Tool("getEnvironmentVariable", "获取环境变量值");
    getEnvVarTool->addParameter("name", "string", "环境变量名称", true);
    m_tools.append(getEnvVarTool);

    Tool* setEnvVarTool = new Tool("setEnvironmentVariable", "设置环境变量");
    setEnvVarTool->addParameter("name", "string", "环境变量名称", true);
    setEnvVarTool->addParameter("value", "string", "环境变量值", true);
    m_tools.append(setEnvVarTool);

    Tool* listEnvVarsTool = new Tool("listEnvironmentVariables", "列出所有环境变量");
    m_tools.append(listEnvVarsTool);

    Tool* getDiskInfoTool = new Tool("getDiskInfo", "获取磁盘驱动器信息");
    m_tools.append(getDiskInfoTool);

    Tool* getMemoryInfoTool = new Tool("getMemoryInfo", "获取内存使用情况");
    m_tools.append(getMemoryInfoTool);

    Tool* getNetworkInfoTool = new Tool("getNetworkInfo", "获取网络接口信息");
    m_tools.append(getNetworkInfoTool);

    Tool* listServicesTool = new Tool("listServices", "列出系统服务状态");
    m_tools.append(listServicesTool);

    Tool* startServiceTool = new Tool("startService", "启动系统服务");
    startServiceTool->addParameter("name", "string", "服务名称", true);
    m_tools.append(startServiceTool);

    Tool* stopServiceTool = new Tool("stopService", "停止系统服务");
    stopServiceTool->addParameter("name", "string", "服务名称", true);
    m_tools.append(stopServiceTool);
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
    } else if (toolName == "getEnvironmentVariable") {
        return getEnvironmentVariable(args);
    } else if (toolName == "setEnvironmentVariable") {
        return setEnvironmentVariable(args);
    } else if (toolName == "listEnvironmentVariables") {
        return listEnvironmentVariables(args);
    } else if (toolName == "getDiskInfo") {
        return getDiskInfo(args);
    } else if (toolName == "getMemoryInfo") {
        return getMemoryInfo(args);
    } else if (toolName == "getNetworkInfo") {
        return getNetworkInfo(args);
    } else if (toolName == "listServices") {
        return listServices(args);
    } else if (toolName == "startService") {
        return startService(args);
    } else if (toolName == "stopService") {
        return stopService(args);
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

    // 允许常用命令：包括编译工具、构建工具、包管理等
    static const QStringList allowedCommands = {
        "dir", "ls", "type", "cat", "echo", "whoami", "hostname", "systeminfo",
        "tasklist", "ipconfig", "netstat", "ping", "tracert", "where", "which",
        "find", "findstr", "wmic", "python", "python3", "pip", "pip3",
        "node", "npm", "npx", "git", "curl", "wget", "mkdir", "cd", "copy",
        "xcopy", "robocopy", "move", "ren", "date", "time", "ver", "set",
        // 编译工具
        "gcc", "g++", "clang", "clang++", "cl", "msbuild",
        // 构建工具
        "cmake", "make", "ninja", "mingw32-make", "mingw64-make",
        // 其他开发工具
        "qmake", "qbs", "moc", "uic", "rcc", "pkg-config",
        // 打包/部署工具
        "tar", "zip", "unzip", "7z", "docker", "docker-compose",
        // 测试工具
        "ctest", "gtest", "cunit", "pytest", "googletest",
        // 代码检查工具
        "cppcheck", "clang-tidy", "flake8", "pylint", "black", "clang-format"
    };

    // 危险操作过滤：只阻止真正会破坏系统的命令
    QString lowerCmd = command.toLower();
    if (lowerCmd.contains("format") || lowerCmd.contains("rd /s") ||
        lowerCmd.contains("rm -rf /") || lowerCmd.contains(":(){:|:&};:") ||
        lowerCmd.contains("del /f /s") || lowerCmd.contains("rmdir /s")) {
        result["success"] = false;
        result["error"] = "该命令被安全策略禁止（危险操作）";
        return result;
    }

    QString cmdName = command.split(QRegularExpression("\\s+")).first().toLower();
    // 去掉引号和路径前缀
    cmdName = cmdName.split('"').last();
    cmdName = cmdName.split('\\').last();
    cmdName = cmdName.split('/').last();

    if (!allowedCommands.contains(cmdName)) {
        result["success"] = false;
        result["error"] = QString("不允许执行命令: %1。建议使用专用工具（如 writeFile）代替 shell 命令。").arg(cmdName);
        return result;
    }

    QProcess process;
#ifdef Q_OS_WIN
    process.start("cmd.exe", QStringList() << "/c" << command);
#else
    process.start("/bin/sh", QStringList() << "-c" << command);
#endif

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

QVariantMap ComputerAgent::getEnvironmentVariable(const QVariantMap& args)
{
    QVariantMap result;
    QString name = args.value("name").toString();
    QString value = qgetenv(name.toUtf8().constData());

    if (value.isEmpty()) {
        result["success"] = false;
        result["error"] = "环境变量不存在: " + name;
        return result;
    }

    result["success"] = true;
    result["result"] = value;
    return result;
}

QVariantMap ComputerAgent::setEnvironmentVariable(const QVariantMap& args)
{
    QVariantMap result;
    QString name = args.value("name").toString();
    QString value = args.value("value").toString();

    qputenv(name.toUtf8().constData(), value.toUtf8().constData());

    result["success"] = true;
    result["result"] = "环境变量 " + name + " 已设置";
    return result;
}

QVariantMap ComputerAgent::listEnvironmentVariables(const QVariantMap& args)
{
    QVariantMap result;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("set");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#else
    QProcess process;
    process.start("/bin/sh", QStringList() << "-c" << "env");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#endif

    result["success"] = true;
    result["result"] = output;
    return result;
}

QVariantMap ComputerAgent::getDiskInfo(const QVariantMap& args)
{
    QVariantMap result;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("wmic", QStringList() << "logicaldisk" << "get" << "caption,freespace,size");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#else
    QProcess process;
    process.start("df", QStringList() << "-h");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#endif

    result["success"] = true;
    result["result"] = output;
    return result;
}

QVariantMap ComputerAgent::getMemoryInfo(const QVariantMap& args)
{
    QVariantMap result;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("systeminfo");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    QStringList lines = output.split('\n');
    QString memoryInfo;
    for (const QString& line : lines) {
        if (line.contains("物理内存") || line.contains("虚拟内存")) {
            memoryInfo += line + "\n";
        }
    }
    
    if (memoryInfo.isEmpty()) {
        memoryInfo = output.left(2000);
    }
#else
    QProcess process;
    process.start("free", QStringList() << "-h");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#endif

    result["success"] = true;
    result["result"] = output;
    return result;
}

QVariantMap ComputerAgent::getNetworkInfo(const QVariantMap& args)
{
    QVariantMap result;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("ipconfig", QStringList() << "/all");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#else
    QProcess process;
    process.start("ifconfig");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    if (output.isEmpty()) {
        process.start("ip", QStringList() << "addr");
        process.waitForFinished(5000);
        output = QString::fromLocal8Bit(process.readAllStandardOutput());
    }
#endif

    result["success"] = true;
    result["result"] = output;
    return result;
}

QVariantMap ComputerAgent::listServices(const QVariantMap& args)
{
    QVariantMap result;

#ifdef Q_OS_WIN
    QProcess process;
    process.start("sc", QStringList() << "query");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
#else
    QProcess process;
    process.start("systemctl", QStringList() << "list-units" << "--type=service");
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    
    if (output.isEmpty()) {
        process.start("service", QStringList() << "--status-all");
        process.waitForFinished(5000);
        output = QString::fromLocal8Bit(process.readAllStandardOutput());
    }
#endif

    result["success"] = true;
    result["result"] = output;
    return result;
}

QVariantMap ComputerAgent::startService(const QVariantMap& args)
{
    QVariantMap result;
    QString name = args.value("name").toString();

#ifdef Q_OS_WIN
    QProcess process;
    process.start("sc", QStringList() << "start" << name);
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(process.readAllStandardError());
    
    if (output.contains("SUCCESS") || output.contains("1056")) {
        result["success"] = true;
        result["result"] = "服务 " + name + " 启动成功";
    } else {
        result["success"] = false;
        result["error"] = error.isEmpty() ? output : error;
    }
#else
    QProcess process;
    process.start("systemctl", QStringList() << "start" << name);
    process.waitForFinished(5000);
    QString error = QString::fromLocal8Bit(process.readAllStandardError());
    
    if (error.isEmpty()) {
        result["success"] = true;
        result["result"] = "服务 " + name + " 启动成功";
    } else {
        result["success"] = false;
        result["error"] = error;
    }
#endif

    return result;
}

QVariantMap ComputerAgent::stopService(const QVariantMap& args)
{
    QVariantMap result;
    QString name = args.value("name").toString();

#ifdef Q_OS_WIN
    QProcess process;
    process.start("sc", QStringList() << "stop" << name);
    process.waitForFinished(5000);
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(process.readAllStandardError());
    
    if (output.contains("SUCCESS") || output.contains("1056")) {
        result["success"] = true;
        result["result"] = "服务 " + name + " 停止成功";
    } else {
        result["success"] = false;
        result["error"] = error.isEmpty() ? output : error;
    }
#else
    QProcess process;
    process.start("systemctl", QStringList() << "stop" << name);
    process.waitForFinished(5000);
    QString error = QString::fromLocal8Bit(process.readAllStandardError());
    
    if (error.isEmpty()) {
        result["success"] = true;
        result["result"] = "服务 " + name + " 停止成功";
    } else {
        result["success"] = false;
        result["error"] = error;
    }
#endif

    return result;
}
