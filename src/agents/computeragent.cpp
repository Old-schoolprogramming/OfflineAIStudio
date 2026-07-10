// 【文件说明】computeragent.cpp - 系统命令代理的实现文件
// 【作用】实现 ComputerAgent 类中声明的所有函数，提供系统命令执行和系统信息获取功能
// 【注意事项】所有工具执行结果以 QVariantMap 形式返回，统一包含 success 和 result/error 字段

#include "computeragent.h"  // 引入 ComputerAgent 自己的头文件，获取类声明
#include "tool.h"           // 引入 Tool 类头文件，用于创建和注册工具
#include <QProcess>         // 引入 Qt 进程类，用于执行外部系统命令
#include <QByteArray>       // 引入 Qt 字节数组类，用于处理命令输出
#include <QRegularExpression> // 引入 Qt 正则表达式类，用于解析命令参数
#include <QSysInfo>         // 引入 Qt 系统信息类，用于获取操作系统信息
#include <QProcessEnvironment> // 引入 Qt 进程环境类，用于读取和设置环境变量
#include <QStorageInfo>     // 引入 Qt 存储信息类，用于获取磁盘容量和可用空间
#include <QNetworkInterface> // 引入 Qt 网络接口类，用于获取网卡信息和IP地址
#include <QHostAddress>     // 引入 Qt 主机地址类，用于处理IP地址

// 【功能说明】ComputerAgent 构造函数，创建系统命令代理实例
// 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
// 【说明】调用父类 Agent 的构造函数，并执行 initializeTools() 注册所有工具
ComputerAgent::ComputerAgent(QObject *parent)
    : Agent(parent)  // 调用父类 Agent 的构造函数，传入父对象指针
{
    initializeTools();  // 初始化并注册所有系统操作工具到 m_tools 列表
}

// 【功能说明】ComputerAgent 析构函数，销毁系统命令代理实例
// 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象，防止内存泄漏
ComputerAgent::~ComputerAgent()
{
    qDeleteAll(m_tools);  // 遍历 m_tools 列表，删除每一个 Tool 对象指针指向的内存
}

// 【功能说明】获取代理名称
// 【返回值】固定返回 QString 字符串 "ComputerAgent"
// 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
QString ComputerAgent::name() const
{
    return "ComputerAgent";  // 返回此 Agent 的唯一标识名称
}

// 【功能说明】获取代理功能描述
// 【返回值】返回描述此 Agent 功能的 QString 字符串
// 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
QString ComputerAgent::description() const
{
    return "系统命令代理，负责执行系统命令和获取系统信息";  // 返回中文功能描述
}

// 【功能说明】获取代理类型
// 【返回值】固定返回枚举值 ComputerAgentType
// 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
AgentType ComputerAgent::type() const
{
    return ComputerAgentType;  // 返回系统命令代理类型枚举值
}

// 【功能说明】获取当前代理注册的所有工具列表
// 【返回值】返回 QList<Tool*> 类型的工具指针列表
// 【说明】重写父类虚函数，外部系统调用此函数获取可用工具清单
QList<Tool*> ComputerAgent::tools() const
{
    return m_tools;  // 返回内部存储的工具列表（m_tools 是成员变量）
}

// 【功能说明】初始化并注册所有系统操作工具
// 【说明】在构造函数中调用一次，为每个功能创建一个 Tool 对象并添加到 m_tools 列表
void ComputerAgent::initializeTools()
{
    // ========== 1. 注册 runCommand 工具 ==========
    // 【工具说明】执行系统命令（有白名单限制，只允许安全命令）
    // 【必填参数】command - 要执行的命令字符串
    // 【可选参数】timeout - 超时时间（秒），默认 30
    Tool* runCommandTool = new Tool("runCommand", "执行系统命令",
        [this](const QVariantMap& args) { return this->runCommand(args); });
    runCommandTool->addParameter("command", "string", "要执行的命令", true);
    runCommandTool->addParameter("timeout", "int", "超时时间(秒)", false, "30");
    m_tools.append(runCommandTool);

    // ========== 2. 注册 getSystemInfo 工具 ==========
    // 【工具说明】获取操作系统和硬件信息（OS名称、版本、内核、CPU架构等）
    // 【参数】无参数
    Tool* systemInfoTool = new Tool("getSystemInfo", "获取系统信息",
        [this](const QVariantMap& args) { return this->getSystemInfo(args); });
    m_tools.append(systemInfoTool);

    // ========== 3. 注册 listProcesses 工具 ==========
    // 【工具说明】列出当前运行的进程列表
    // 【参数】无参数
    Tool* listProcessesTool = new Tool("listProcesses", "列出当前进程",
        [this](const QVariantMap& args) { return this->listProcesses(args); });
    m_tools.append(listProcessesTool);

    // ========== 4. 注册 killProcess 工具 ==========
    // 【工具说明】终止指定进程ID的进程
    // 【必填参数】pid - 进程ID（整数）
    Tool* killProcessTool = new Tool("killProcess", "终止进程",
        [this](const QVariantMap& args) { return this->killProcess(args); });
    killProcessTool->addParameter("pid", "int", "进程ID", true);
    m_tools.append(killProcessTool);

    // ========== 5. 注册 getEnvironmentVariable 工具 ==========
    // 【工具说明】获取指定环境变量的值
    // 【必填参数】name - 环境变量名称
    Tool* getEnvVarTool = new Tool("getEnvironmentVariable", "获取环境变量值",
        [this](const QVariantMap& args) { return this->getEnvironmentVariable(args); });
    getEnvVarTool->addParameter("name", "string", "环境变量名称", true);
    m_tools.append(getEnvVarTool);

    // ========== 6. 注册 setEnvironmentVariable 工具 ==========
    // 【工具说明】设置环境变量的值
    // 【必填参数】name - 环境变量名称；value - 环境变量值
    Tool* setEnvVarTool = new Tool("setEnvironmentVariable", "设置环境变量",
        [this](const QVariantMap& args) { return this->setEnvironmentVariable(args); });
    setEnvVarTool->addParameter("name", "string", "环境变量名称", true);
    setEnvVarTool->addParameter("value", "string", "环境变量值", true);
    m_tools.append(setEnvVarTool);

    // ========== 7. 注册 listEnvironmentVariables 工具 ==========
    // 【工具说明】列出所有环境变量及其值
    // 【参数】无参数
    Tool* listEnvVarsTool = new Tool("listEnvironmentVariables", "列出所有环境变量",
        [this](const QVariantMap& args) { return this->listEnvironmentVariables(args); });
    m_tools.append(listEnvVarsTool);

    // ========== 8. 注册 getDiskInfo 工具 ==========
    // 【工具说明】获取磁盘驱动器信息（容量、可用空间等）
    // 【参数】无参数
    Tool* getDiskInfoTool = new Tool("getDiskInfo", "获取磁盘驱动器信息",
        [this](const QVariantMap& args) { return this->getDiskInfo(args); });
    m_tools.append(getDiskInfoTool);

    // ========== 9. 注册 getMemoryInfo 工具 ==========
    // 【工具说明】获取内存使用情况
    // 【参数】无参数
    Tool* getMemoryInfoTool = new Tool("getMemoryInfo", "获取内存使用情况",
        [this](const QVariantMap& args) { return this->getMemoryInfo(args); });
    m_tools.append(getMemoryInfoTool);

    // ========== 10. 注册 getNetworkInfo 工具 ==========
    // 【工具说明】获取网络接口信息（IP地址、MAC地址等）
    // 【参数】无参数
    Tool* getNetworkInfoTool = new Tool("getNetworkInfo", "获取网络接口信息",
        [this](const QVariantMap& args) { return this->getNetworkInfo(args); });
    m_tools.append(getNetworkInfoTool);

    // ========== 11. 注册 listServices 工具 ==========
    // 【工具说明】列出系统服务状态
    // 【参数】无参数
    Tool* listServicesTool = new Tool("listServices", "列出系统服务状态",
        [this](const QVariantMap& args) { return this->listServices(args); });
    m_tools.append(listServicesTool);

    // ========== 12. 注册 startService 工具 ==========
    // 【工具说明】启动指定的系统服务
    // 【必填参数】name - 服务名称
    Tool* startServiceTool = new Tool("startService", "启动系统服务",
        [this](const QVariantMap& args) { return this->startService(args); });
    startServiceTool->addParameter("name", "string", "服务名称", true);
    m_tools.append(startServiceTool);

    // ========== 13. 注册 stopService 工具 ==========
    // 【工具说明】停止指定的系统服务
    // 【必填参数】name - 服务名称
    Tool* stopServiceTool = new Tool("stopService", "停止系统服务",
        [this](const QVariantMap& args) { return this->stopService(args); });
    stopServiceTool->addParameter("name", "string", "服务名称", true);
    m_tools.append(stopServiceTool);
}

// 【功能说明】工具分发执行入口，根据工具名称调用对应的私有实现方法
// 【参数说明】toolName - 要执行的工具名称字符串
// 【参数说明】args - 工具参数字典，键为 QString，值为 QVariant
// 【返回值】QVariantMap 执行结果，包含 success（bool）、result（QString）或 error（QString）
QVariantMap ComputerAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    // 使用 if-else 链根据 toolName 将请求分派到对应的私有方法
    if (toolName == "runCommand") {  // 执行系统命令
        return runCommand(args);
    } else if (toolName == "getSystemInfo") {  // 获取系统信息
        return getSystemInfo(args);
    } else if (toolName == "listProcesses") {  // 列出进程
        return listProcesses(args);
    } else if (toolName == "killProcess") {  // 终止进程
        return killProcess(args);
    } else if (toolName == "getEnvironmentVariable") {  // 获取环境变量
        return getEnvironmentVariable(args);
    } else if (toolName == "setEnvironmentVariable") {  // 设置环境变量
        return setEnvironmentVariable(args);
    } else if (toolName == "listEnvironmentVariables") {  // 列出所有环境变量
        return listEnvironmentVariables(args);
    } else if (toolName == "getDiskInfo") {  // 获取磁盘信息
        return getDiskInfo(args);
    } else if (toolName == "getMemoryInfo") {  // 获取内存信息
        return getMemoryInfo(args);
    } else if (toolName == "getNetworkInfo") {  // 获取网络信息
        return getNetworkInfo(args);
    } else if (toolName == "listServices") {  // 列出服务
        return listServices(args);
    } else if (toolName == "startService") {  // 启动服务
        return startService(args);
    } else if (toolName == "stopService") {  // 停止服务
        return stopService(args);
    }

    // 如果 toolName 不匹配任何已知工具，返回通用错误信息
    QVariantMap result;  // 创建结果字典
    result["success"] = false;  // 标记执行失败
    result["error"] = "Unknown tool: " + toolName;  // 返回错误提示，说明工具名称未知
    return result;  // 返回错误结果
}

// 【功能说明】执行系统命令（有白名单和黑名单安全限制）
// 【参数说明】args - 参数字典，必须包含 "command"（命令字符串），可选 "timeout"（超时秒数，默认30）
// 【返回值】若成功，result 字段为标准输出内容；若失败或超时，error 字段包含原因
// 【安全说明】只允许白名单中的命令，禁止危险操作（如格式化磁盘、递归删除根目录等）
QVariantMap ComputerAgent::runCommand(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString command = args.value("command").toString();  // 提取命令字符串
    int timeout = args.value("timeout", 30).toInt();  // 提取超时时间，默认 30 秒

    // 定义允许执行的命令白名单
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

    // 危险操作过滤：阻止真正会破坏系统的命令
    QString lowerCmd = command.toLower();  // 转为小写进行不区分大小写的检查
    if (lowerCmd.contains("format") || lowerCmd.contains("rd /s") ||
        lowerCmd.contains("rm -rf /") || lowerCmd.contains(":(){:|:&};") ||
        lowerCmd.contains("del /f /s") || lowerCmd.contains("rmdir /s")) {
        result["success"] = false;  // 标记失败
        result["error"] = "该命令被安全策略禁止（危险操作）";  // 返回安全提示
        return result;  // 返回错误结果
    }

    // 提取命令名称（第一个单词），去除引号和路径前缀
    QString cmdName = command.split(QRegularExpression("\\s+")).first().toLower();  // 按空白拆分，取第一个
    cmdName = cmdName.split('"').last();  // 去除引号
    cmdName = cmdName.split('\\').last();  // 去除 Windows 路径前缀
    cmdName = cmdName.split('/').last();  // 去除 Unix 路径前缀

    // 检查命令是否在白名单中
    if (!allowedCommands.contains(cmdName)) {
        result["success"] = false;  // 不在白名单中，标记失败
        result["error"] = QString("不允许执行命令: %1。建议使用专用工具（如 writeFile）代替 shell 命令。").arg(cmdName);  // 返回提示
        return result;  // 返回错误结果
    }

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    process.start("cmd.exe", QStringList() << "/c" << command);  // 使用 cmd.exe 执行命令
#else  // Linux/macOS 平台
    process.start("/bin/sh", QStringList() << "-c" << command);  // 使用 sh 执行命令
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法启动命令";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，带超时限制
    if (!process.waitForFinished(timeout * 1000)) {  // timeout 转换为毫秒
        process.kill();  // 超时则强制终止进程
        result["success"] = false;  // 标记失败
        result["error"] = "命令执行超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 读取命令输出
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 标准输出
    QString error = QString::fromLocal8Bit(process.readAllStandardError());  // 标准错误

    // 检查退出码
    if (process.exitCode() != 0) {  // 如果退出码非 0，视为执行失败
        result["success"] = false;  // 标记失败
        result["error"] = error.isEmpty() ? "命令执行失败" : error;  // 返回错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = output;  // 返回标准输出内容
    return result;  // 返回成功结果
}

// 【功能说明】获取操作系统和硬件信息
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】固定 success=true，result 字段包含操作系统、版本、内核、CPU架构等信息
// 【说明】使用 QSysInfo 静态方法获取系统信息，跨平台支持 Windows/Linux/macOS
QVariantMap ComputerAgent::getSystemInfo(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用，避免编译器警告
    QVariantMap result;  // 创建结果字典

    // 使用 QSysInfo 获取各项系统信息
    QString osName = QSysInfo::prettyProductName();  // 操作系统友好名称（如 "Windows 10"）
    QString osVersion = QSysInfo::productVersion();  // 操作系统版本号
    QString kernelType = QSysInfo::kernelType();  // 内核类型（如 winnt、linux、darwin）
    QString kernelVersion = QSysInfo::kernelVersion();  // 内核版本
    QString cpuArch = QSysInfo::currentCpuArchitecture();  // CPU 架构（如 x86_64、arm64）

    // 格式化系统信息字符串
    QString info = QString("操作系统: %1\n版本: %2\n内核类型: %3\n内核版本: %4\nCPU架构: %5")
        .arg(osName, osVersion, kernelType, kernelVersion, cpuArch);

    result["success"] = true;  // 标记成功
    result["result"] = info;  // 返回系统信息
    return result;  // 返回结果
}

// 【功能说明】列出当前运行的进程
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】若成功，result 字段为进程列表文本；若失败或超时，error 字段包含原因
// 【说明】跨平台实现：Windows 使用 tasklist，Linux/macOS 使用 ps aux
QVariantMap ComputerAgent::listProcesses(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用
    QVariantMap result;  // 创建结果字典

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    process.start("tasklist");  // 执行 tasklist 命令获取进程列表
#else  // Linux/macOS 平台
    process.start("ps", QStringList() << "aux");  // 执行 ps aux 命令
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法获取进程列表";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，超时 5 秒
    if (!process.waitForFinished(5000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "获取进程列表超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 读取标准输出

    result["success"] = true;  // 标记成功
    result["result"] = output;  // 返回进程列表
    return result;  // 返回结果
}

// 【功能说明】终止指定进程
// 【参数说明】args - 参数字典，必须包含 "pid" 键（进程ID整数）
// 【返回值】若成功，result 字段包含终止成功提示；若失败或超时，error 字段包含原因
// 【说明】跨平台实现：Windows 使用 taskkill /F /PID，Linux/macOS 使用 kill -9
QVariantMap ComputerAgent::killProcess(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    int pid = args.value("pid").toInt();  // 提取进程 ID

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    process.start("taskkill", QStringList() << "/F" << "/PID" << QString::number(pid));  // 强制终止指定 PID
#else  // Linux/macOS 平台
    process.start("kill", QStringList() << "-9" << QString::number(pid));  // 发送 SIGKILL 信号强制终止
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法终止进程";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，超时 5 秒
    if (!process.waitForFinished(5000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "终止进程超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString error = QString::fromLocal8Bit(process.readAllStandardError());  // 读取标准错误

    // 如果标准错误有输出，视为终止失败
    if (!error.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = error;  // 将标准错误内容作为错误信息返回
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = QString("进程 %1 已终止").arg(pid);  // 返回终止成功提示，包含进程ID
    return result;  // 返回成功结果
}

// 【功能说明】获取指定环境变量的值
// 【参数说明】args - 参数字典，必须包含 "name" 键（环境变量名称字符串）
// 【返回值】若变量存在，success=true，result 为变量值；若不存在，error 提示未找到
// 【说明】使用 QProcessEnvironment::systemEnvironment() 读取当前进程的环境变量
QVariantMap ComputerAgent::getEnvironmentVariable(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString name = args.value("name").toString();  // 提取环境变量名称

    // 检查变量名是否为空
    if (name.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "环境变量名称不能为空";  // 返回错误提示
        return result;  // 返回错误结果
    }

    // 从系统环境变量中查找指定名称的值
    QString value = QProcessEnvironment::systemEnvironment().value(name);

    // 判断是否找到该环境变量
    if (value.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = QString("未找到环境变量: %1").arg(name);  // 返回未找到提示
    } else {
        result["success"] = true;  // 标记成功
        result["result"] = value;  // 返回环境变量的值
    }
    return result;  // 返回结果
}

// 【功能说明】设置环境变量的值（仅对当前进程有效）
// 【参数说明】args - 参数字典，必须包含 "name"（变量名）和 "value"（变量值）
// 【返回值】success=true 表示设置成功，result 返回提示信息
// 【说明】使用 qputenv 设置环境变量，注意此设置仅影响当前进程及其子进程
QVariantMap ComputerAgent::setEnvironmentVariable(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString name = args.value("name").toString();  // 提取变量名
    QString value = args.value("value").toString();  // 提取变量值

    // 检查变量名是否为空
    if (name.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "环境变量名称不能为空";  // 返回错误提示
        return result;  // 返回错误结果
    }

    // 使用 qputenv 设置环境变量，传入 QByteArray 格式
    bool ok = qputenv(name.toUtf8(), value.toUtf8());

    if (ok) {
        result["success"] = true;  // 标记成功
        result["result"] = QString("环境变量 %1 已设置为 %2").arg(name, value);  // 返回成功提示
    } else {
        result["success"] = false;  // 标记失败
        result["error"] = QString("无法设置环境变量: %1").arg(name);  // 返回错误提示
    }
    return result;  // 返回结果
}

// 【功能说明】列出所有环境变量及其值
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】success=true，result 字段包含所有环境变量的键值对列表文本
// 【说明】使用 QProcessEnvironment::systemEnvironment().toStringList() 获取环境变量列表
QVariantMap ComputerAgent::listEnvironmentVariables(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用，避免编译器警告
    QVariantMap result;  // 创建结果字典

    // 获取系统环境变量的键值对列表
    QStringList envList = QProcessEnvironment::systemEnvironment().toStringList();

    // 按字母顺序排序，方便查看
    envList.sort();

    result["success"] = true;  // 标记成功
    result["result"] = envList.join("\n");  // 用换行符拼接所有环境变量
    return result;  // 返回结果
}

// 【功能说明】获取磁盘驱动器信息（容量、可用空间、文件系统类型等）
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】success=true，result 字段包含各磁盘的信息文本
// 【说明】使用 QStorageInfo::mountedVolumes() 获取已挂载的磁盘卷信息，跨平台支持
QVariantMap ComputerAgent::getDiskInfo(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用，避免编译器警告
    QVariantMap result;  // 创建结果字典
    QStringList infoList;  // 用于拼接各磁盘信息的字符串列表

    // 遍历所有已挂载的磁盘卷
    for (const QStorageInfo& storage : QStorageInfo::mountedVolumes()) {
        // 跳过无效或未就绪的磁盘
        if (!storage.isValid() || !storage.isReady()) {
            continue;  // 跳过无效磁盘，继续处理下一个
        }

        // 格式化单个磁盘的信息字符串
        QString diskInfo = QString("设备: %1\n  挂载点: %2\n  文件系统: %3\n  总容量: %4 GB\n  可用空间: %5 GB\n  已用空间: %6 GB")
            .arg(storage.device())  // 设备名称
            .arg(storage.rootPath())  // 挂载路径
            .arg(storage.fileSystemType())  // 文件系统类型（如 NTFS、ext4）
            .arg(storage.bytesTotal() / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2)  // 总容量转换为GB
            .arg(storage.bytesAvailable() / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2)  // 可用空间转换为GB
            .arg((storage.bytesTotal() - storage.bytesAvailable()) / 1024.0 / 1024.0 / 1024.0, 0, 'f', 2);  // 已用空间

        infoList.append(diskInfo);  // 将当前磁盘信息添加到列表
    }

    result["success"] = true;  // 标记成功
    result["result"] = infoList.join("\n\n");  // 用空行分隔各磁盘信息
    return result;  // 返回结果
}

// 【功能说明】获取内存使用情况（总内存、可用内存等）
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】success=true，result 字段包含内存信息文本
// 【说明】Qt 没有直接提供内存使用 API，因此使用系统命令获取：Windows 用 wmic，Linux 用 free
QVariantMap ComputerAgent::getMemoryInfo(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用，避免编译器警告
    QVariantMap result;  // 创建结果字典

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    // 使用 wmic 命令获取物理内存信息（总容量和可用容量，单位为 Byte）
    process.start("wmic", QStringList() << "ComputerSystem" << "get" << "TotalPhysicalMemory");
#else  // Linux/macOS 平台
    // 使用 free 命令获取内存信息（-m 表示以 MB 显示）
    process.start("free", QStringList() << "-m");
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法获取内存信息";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，超时 5 秒
    if (!process.waitForFinished(5000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "获取内存信息超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 读取标准输出
    result["success"] = true;  // 标记成功
    result["result"] = output;  // 返回内存信息
    return result;  // 返回结果
}

// 【功能说明】获取网络接口信息（IP地址、MAC地址、子网掩码等）
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】success=true，result 字段包含各网络接口的信息文本
// 【说明】使用 QNetworkInterface::allInterfaces() 获取本机所有网卡信息，跨平台支持
QVariantMap ComputerAgent::getNetworkInfo(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用，避免编译器警告
    QVariantMap result;  // 创建结果字典
    QStringList infoList;  // 用于拼接各接口信息的字符串列表

    // 遍历所有网络接口
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        // 跳过处于 Down 状态（未启用）的接口
        if (iface.flags() & QNetworkInterface::IsLoopBack) {
            continue;  // 跳过本地回环接口（如 lo、Loopback Pseudo-Interface），减少冗余信息
        }

        QStringList addrList;  // 存储当前接口的所有 IP 地址
        for (const QHostAddress& addr : iface.allAddresses()) {
            // 只显示 IPv4 地址，过滤掉 IPv6 和本地地址
            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                addrList.append(addr.toString());  // 将 IP 地址转为字符串添加
            }
        }

        // 格式化当前接口的信息
        QString ifaceInfo = QString("接口名称: %1\n  MAC地址: %2\n  IP地址: %3\n  状态: %4")
            .arg(iface.humanReadableName())  // 人类可读的接口名称
            .arg(iface.hardwareAddress())  // MAC 地址（物理地址）
            .arg(addrList.isEmpty() ? "无" : addrList.join(", "))  // IP 地址列表，若无则显示"无"
            .arg((iface.flags() & QNetworkInterface::IsUp) ? "已启用" : "已禁用");  // 接口启用状态

        infoList.append(ifaceInfo);  // 将当前接口信息添加到列表
    }

    result["success"] = true;  // 标记成功
    result["result"] = infoList.join("\n\n");  // 用空行分隔各接口信息
    return result;  // 返回结果
}

// 【功能说明】列出系统服务状态
// 【参数说明】args - 空参数（此工具无需参数）
// 【返回值】若成功，result 字段为服务列表文本；若失败或超时，error 字段包含原因
// 【说明】跨平台实现：Windows 使用 sc query，Linux 使用 systemctl list-units --type=service
QVariantMap ComputerAgent::listServices(const QVariantMap& args)
{
    Q_UNUSED(args)  // 标记 args 未使用
    QVariantMap result;  // 创建结果字典

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    process.start("sc", QStringList() << "query");  // 执行 sc query 查询所有服务状态
#else  // Linux 平台
    process.start("systemctl", QStringList() << "list-units" << "--type=service" << "--no-pager");  // 列出所有服务单元
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法获取服务列表";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，超时 10 秒（服务列表可能较长）
    if (!process.waitForFinished(10000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "获取服务列表超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 读取标准输出

    result["success"] = true;  // 标记成功
    result["result"] = output;  // 返回服务列表
    return result;  // 返回结果
}

// 【功能说明】启动指定的系统服务
// 【参数说明】args - 参数字典，必须包含 "name" 键（服务名称字符串）
// 【返回值】若成功，result 字段包含启动成功提示；若失败或超时，error 字段包含原因
// 【说明】跨平台实现：Windows 使用 sc start，Linux 使用 systemctl start
QVariantMap ComputerAgent::startService(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString serviceName = args.value("name").toString();  // 提取服务名称

    // 检查服务名称是否为空
    if (serviceName.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "服务名称不能为空";  // 返回错误提示
        return result;  // 返回错误结果
    }

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    process.start("sc", QStringList() << "start" << serviceName);  // 执行 sc start 启动服务
#else  // Linux 平台
    process.start("systemctl", QStringList() << "start" << serviceName);  // 执行 systemctl start 启动服务
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法启动服务";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，超时 10 秒（服务启动可能需要时间）
    if (!process.waitForFinished(10000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "启动服务超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 读取命令输出
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 标准输出
    QString errorStr = QString::fromLocal8Bit(process.readAllStandardError());  // 标准错误

    // 检查退出码和标准错误
    if (process.exitCode() != 0 || !errorStr.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = errorStr.isEmpty() ? "启动服务失败" : errorStr;  // 返回错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = QString("服务 '%1' 启动成功\n%2").arg(serviceName, output);  // 返回成功提示及输出
    return result;  // 返回成功结果
}

// 【功能说明】停止指定的系统服务
// 【参数说明】args - 参数字典，必须包含 "name" 键（服务名称字符串）
// 【返回值】若成功，result 字段包含停止成功提示；若失败或超时，error 字段包含原因
// 【说明】跨平台实现：Windows 使用 sc stop，Linux 使用 systemctl stop
QVariantMap ComputerAgent::stopService(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString serviceName = args.value("name").toString();  // 提取服务名称

    // 检查服务名称是否为空
    if (serviceName.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "服务名称不能为空";  // 返回错误提示
        return result;  // 返回错误结果
    }

    QProcess process;  // 创建 QProcess 对象
#ifdef Q_OS_WIN  // Windows 平台
    process.start("sc", QStringList() << "stop" << serviceName);  // 执行 sc stop 停止服务
#else  // Linux 平台
    process.start("systemctl", QStringList() << "stop" << serviceName);  // 执行 systemctl stop 停止服务
#endif

    // 等待进程启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "无法停止服务";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待进程结束，超时 10 秒（服务停止可能需要时间）
    if (!process.waitForFinished(10000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "停止服务超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 读取命令输出
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 标准输出
    QString errorStr = QString::fromLocal8Bit(process.readAllStandardError());  // 标准错误

    // 检查退出码和标准错误
    if (process.exitCode() != 0 || !errorStr.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = errorStr.isEmpty() ? "停止服务失败" : errorStr;  // 返回错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = QString("服务 '%1' 停止成功\n%2").arg(serviceName, output);  // 返回成功提示及输出
    return result;  // 返回成功结果
}