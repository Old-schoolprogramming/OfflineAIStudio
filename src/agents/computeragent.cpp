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
#include <QProcessEnvironment>
#include <QDir>
#include <QStorageInfo>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QCryptographicHash>
#include <QFile>

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

    Tool* getEnvTool = new Tool("getEnv", "获取环境变量值");
    getEnvTool->addParameter("name", "string", "环境变量名称", true);
    m_tools.append(getEnvTool);

    Tool* setEnvTool = new Tool("setEnv", "设置环境变量");
    setEnvTool->addParameter("name", "string", "环境变量名称", true);
    setEnvTool->addParameter("value", "string", "环境变量值", true);
    m_tools.append(setEnvTool);

    Tool* listEnvTool = new Tool("listEnv", "列出所有环境变量");
    m_tools.append(listEnvTool);

    Tool* diskInfoTool = new Tool("diskInfo", "获取磁盘使用情况");
    diskInfoTool->addParameter("path", "string", "磁盘路径（默认根目录）", false, "/");
    m_tools.append(diskInfoTool);

    Tool* memoryInfoTool = new Tool("memoryInfo", "获取内存使用情况");
    m_tools.append(memoryInfoTool);

    Tool* networkInfoTool = new Tool("networkInfo", "获取网络接口信息");
    m_tools.append(networkInfoTool);

    Tool* checkPortTool = new Tool("checkPort", "检查端口是否被占用");
    checkPortTool->addParameter("port", "int", "端口号", true);
    checkPortTool->addParameter("host", "string", "主机地址", false, "localhost");
    m_tools.append(checkPortTool);

    Tool* pingTool = new Tool("ping", "Ping指定主机");
    pingTool->addParameter("host", "string", "目标主机地址", true);
    pingTool->addParameter("count", "int", "Ping次数", false, "4");
    m_tools.append(pingTool);

    Tool* currentDirTool = new Tool("currentDirectory", "获取当前工作目录");
    m_tools.append(currentDirTool);

    Tool* changeDirTool = new Tool("changeDirectory", "切换当前工作目录");
    changeDirTool->addParameter("path", "string", "目标目录路径", true);
    m_tools.append(changeDirTool);

    Tool* cpuInfoTool = new Tool("cpuInfo", "获取CPU信息");
    m_tools.append(cpuInfoTool);

    Tool* fileHashTool = new Tool("fileHash", "计算文件MD5哈希值");
    fileHashTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(fileHashTool);
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
    } else if (toolName == "getEnv") {
        return getEnv(args);
    } else if (toolName == "setEnv") {
        return setEnv(args);
    } else if (toolName == "listEnv") {
        return listEnv(args);
    } else if (toolName == "diskInfo") {
        return diskInfo(args);
    } else if (toolName == "memoryInfo") {
        return memoryInfo(args);
    } else if (toolName == "networkInfo") {
        return networkInfo(args);
    } else if (toolName == "checkPort") {
        return checkPort(args);
    } else if (toolName == "ping") {
        return ping(args);
    } else if (toolName == "currentDirectory") {
        return currentDirectory(args);
    } else if (toolName == "changeDirectory") {
        return changeDirectory(args);
    } else if (toolName == "cpuInfo") {
        return cpuInfo(args);
    } else if (toolName == "fileHash") {
        return fileHash(args);
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

/**
 * @brief 获取环境变量值
 * @param args 参数映射，包含name
 * @return 环境变量值
 *
 * @details
 * 使用QProcessEnvironment获取当前进程的环境变量。
 * 如果变量不存在时返回空字符串并提示。
 */
QVariantMap ComputerAgent::getEnv(const QVariantMap& args)
{
    QVariantMap result;
    QString name = args.value("name").toString();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString value = env.value(name);

    if (value.isEmpty()) {
        result["success"] = true;
        result["result"] = QString("环境变量 %1 不存在或为空").arg(name);
    } else {
        result["success"] = true;
        result["result"] = QString("%1=%2").arg(name, value);
    }
    return result;
}

/**
 * @brief 设置环境变量
 * @param args 参数映射，包含name、value
 * @return 设置结果
 *
 * @details
 * 使用qputenv设置进程环境变量。注意：这只会影响当前进程及其子进程。
 */
QVariantMap ComputerAgent::setEnv(const QVariantMap& args)
{
    QVariantMap result;
    QString name = args.value("name").toString();
    QString value = args.value("value").toString();

    bool success = qputenv(name.toUtf8(), value.toUtf8());

    if (success) {
        result["success"] = true;
        result["result"] = QString("环境变量设置成功: %1=%2").arg(name, value);
    } else {
        result["success"] = false;
        result["error"] = "环境变量设置失败";
    }
    return result;
}

/**
 * @brief 列出所有环境变量
 * @param args 无参数
 * @return 所有环境变量列表
 *
 * @details
 * 使用QProcessEnvironment获取所有系统环境变量并格式化输出。
 */
QVariantMap ComputerAgent::listEnv(const QVariantMap& args)
{
    Q_UNUSED(args)
    QVariantMap result;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList keys = env.toStringList();

    result["success"] = true;
    result["result"] = QString("共 %1 个环境变量:\n\n").arg(keys.size()) + keys.join("\n");
    return result;
}

/**
 * @brief 获取磁盘使用情况
 * @param args 参数映射，包含path
 * @return 磁盘空间信息
 *
 * @details
 * 使用QStorageInfo获取指定路径所在磁盘的总空间、
 * 已用空间、可用空间等信息。
 */
QVariantMap ComputerAgent::diskInfo(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path", "/").toString();

    QStorageInfo storage(path);
    if (!storage.isValid()) {
        result["success"] = false;
        result["error"] = "无效的路径: " + path;
        return result;
    }

    qint64 total = storage.bytesTotal();
    qint64 free = storage.bytesAvailable();
    qint64 used = total - free;
    int usagePercent = total > 0 ? (int)(used * 100 / total) : 0;

    auto formatSize = [](qint64 bytes) -> QString {
        if (bytes < 1024LL) return QString("%1 B").arg(bytes);
        else if (bytes < 1024LL * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
        else if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 2);
        else return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 2);
    };

    QString info = QString(
        "磁盘: %1\n"
        "文件系统: %2\n"
        "总空间: %3\n"
        "已用空间: %4\n"
        "可用空间: %5\n"
        "使用率: %6%"
    ).arg(
        storage.rootPath(),
        QString::fromUtf8(storage.fileSystemType()),
        formatSize(total),
        formatSize(used),
        formatSize(free),
        QString::number(usagePercent)
    );

    result["success"] = true;
    result["result"] = info;
    return result;
}

/**
 * @brief 获取内存使用情况
 * @param args 无参数
 * @return 内存信息
 *
 * @details
 * 通过执行系统命令获取内存信息：
 * - Linux: 读取/proc/meminfo
 * - Windows: 使用systeminfo
 */
QVariantMap ComputerAgent::memoryInfo(const QVariantMap& args)
{
    Q_UNUSED(args)
    QVariantMap result;

    QProcess process;
#ifdef Q_OS_WIN
    process.start("wmic", QStringList() << "OS" << "get" << "TotalVisibleMemorySize,FreePhysicalMemory" << "/value");
#else
    process.start("cat", QStringList() << "/proc/meminfo");
#endif

    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法获取内存信息";
        return result;
    }

    if (!process.waitForFinished(3000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "获取内存信息超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    result["success"] = true;
    result["result"] = output;
    return result;
}

/**
 * @brief 获取网络接口信息
 * @param args 无参数
 * @return 网络接口列表及IP地址
 *
 * @details
 * 使用QNetworkInterface获取所有网络接口的信息，
 * 包括接口名称、MAC地址、IP地址等。
 */
QVariantMap ComputerAgent::networkInfo(const QVariantMap& args)
{
    Q_UNUSED(args)
    QVariantMap result;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    QString info;

    for (const auto& iface : interfaces) {
        info += QString("=== 接口: %1 ===\n").arg(iface.name());
        info += QString("  显示名: %1\n").arg(iface.humanReadableName());
        info += QString("  MAC地址: %1\n").arg(iface.hardwareAddress());
        info += QString("  状态: %1\n").arg(iface.flags().testFlag(QNetworkInterface::IsUp) ? "启用" : "禁用");

        QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const auto& entry : entries) {
            info += QString("  IP: %1/%2\n").arg(entry.ip().toString()).arg(entry.prefixLength());
        }
        info += "\n";
    }

    result["success"] = true;
    result["result"] = QString("共 %1 个网络接口:\n\n%2").arg(interfaces.size()).arg(info);
    return result;
}

/**
 * @brief 检查端口是否被占用
 * @param args 参数映射，包含port、host
 * @return 端口占用情况
 *
 * @details
 * 通过尝试连接到指定端口来判断端口是否被占用。
 * 如果能连接上，说明端口已被占用。
 */
QVariantMap ComputerAgent::checkPort(const QVariantMap& args)
{
    QVariantMap result;
    int port = args.value("port").toInt();
    QString host = args.value("host", "localhost").toString();

    QTcpSocket socket;
    socket.connectToHost(host, port);

    if (socket.waitForConnected(1000)) {
        socket.disconnectFromHost();
        result["success"] = true;
        result["result"] = QString("端口 %1 (%2) 已被占用").arg(port).arg(host);
    } else {
        result["success"] = true;
        result["result"] = QString("端口 %1 (%2) 未被占用").arg(port).arg(host);
    }
    return result;
}

/**
 * @brief Ping指定主机
 * @param args 参数映射，包含host、count
 * @return Ping结果
 *
 * @details
 * 执行系统ping命令测试网络连通性。
 * 跨平台实现：Windows和Linux/macOS使用不同参数。
 */
QVariantMap ComputerAgent::ping(const QVariantMap& args)
{
    QVariantMap result;
    QString host = args.value("host").toString();
    int count = args.value("count", 4).toInt();

    QProcess process;
#ifdef Q_OS_WIN
    process.start("ping", QStringList() << "-n" << QString::number(count) << host);
#else
    process.start("ping", QStringList() << "-c" << QString::number(count) << host);
#endif

    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法执行ping命令";
        return result;
    }

    if (!process.waitForFinished(10000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "ping超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    result["success"] = true;
    result["result"] = output;
    return result;
}

/**
 * @brief 获取当前工作目录
 * @param args 无参数
 * @return 当前工作目录路径
 */
QVariantMap ComputerAgent::currentDirectory(const QVariantMap& args)
{
    Q_UNUSED(args)
    QVariantMap result;
    result["success"] = true;
    result["result"] = QDir::currentPath();
    return result;
}

/**
 * @brief 切换当前工作目录
 * @param args 参数映射，包含path
 * @return 切换结果
 */
QVariantMap ComputerAgent::changeDirectory(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + path;
        return result;
    }

    if (!QDir::setCurrent(path)) {
        result["success"] = false;
        result["error"] = "切换目录失败";
        return result;
    }

    result["success"] = true;
    result["result"] = "当前目录: " + QDir::currentPath();
    return result;
}

/**
 * @brief 获取CPU信息
 * @param args 无参数
 * @return CPU详细信息
 *
 * @details
 * 通过系统命令获取CPU信息：
 * - Linux: 读取/proc/cpuinfo
 * - Windows: 使用wmic cpu get
 */
QVariantMap ComputerAgent::cpuInfo(const QVariantMap& args)
{
    Q_UNUSED(args)
    QVariantMap result;

    QProcess process;
#ifdef Q_OS_WIN
    process.start("wmic", QStringList() << "cpu" << "get" << "Name,NumberOfCores,NumberOfLogicalProcessors,MaxClockSpeed" << "/value");
#else
    process.start("cat", QStringList() << "/proc/cpuinfo");
#endif

    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "无法获取CPU信息";
        return result;
    }

    if (!process.waitForFinished(3000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "获取CPU信息超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    result["success"] = true;
    result["result"] = output;
    return result;
}

/**
 * @brief 计算文件MD5哈希值
 * @param args 参数映射，包含path
 * @return MD5哈希值
 *
 * @details
 * 使用QCryptographicHash计算文件的MD5哈希值。
 */
QVariantMap ComputerAgent::fileHash(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFile file(path);
    if (!file.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + path;
        return result;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&file)) {
        result["success"] = false;
        result["error"] = "读取文件失败";
        file.close();
        return result;
    }
    file.close();

    QString md5 = QString::fromUtf8(hash.result().toHex());
    result["success"] = true;
    result["result"] = QString("文件 %1 的MD5值: %2").arg(path, md5);
    return result;
}
