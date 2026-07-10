#include "environmentdetector.h"  // 【引入自身头文件】包含EnvironmentDetector类声明
#include <QProcess>               // 【引入进程类】用于在后台运行系统命令
#include <QDir>                   // 【引入目录类】操作文件路径
#include <QFileInfo>              // 【引入文件信息类】检查文件是否存在
#include <QNetworkInformation>    // 【引入网络信息类】Qt6中用于检测网络连接状态

/**
 * @brief 构造函数
 * @param parent 父QObject对象（用于Qt对象树自动内存管理）
 *
 * 初始化离线模式标志为false（默认假设有网络）。
 */
EnvironmentDetector::EnvironmentDetector(QObject *parent)
    : QObject(parent),        // 【调用基类构造函数】将parent传给QObject
      m_offlineMode(false)    // 【初始化成员】默认不在离线模式
{
}

/**
 * @brief 执行环境检测
 *
 * 检测流程：
 * 1. 清空之前的检测结果
 * 2. 检测网络状态（是否离线）
 * 3. 依次检测编译器、构建工具、Qt工具、Python、Git等
 * 4. 发射detectionCompleted信号通知检测完成
 */
void EnvironmentDetector::detect()
{
    m_tools.clear();  // 【清空旧数据】移除之前检测到的所有工具信息，重新检测

    // 【检测网络状态】Qt 6 中 QNetworkConfigurationManager 已移除，改用 QNetworkInformation
    QNetworkInformation::load(QNetworkInformation::Feature::Reachability);  // 【加载网络信息模块】启用可达性检测功能
    auto *netInfo = QNetworkInformation::instance();  // 【获取网络信息单例】
    // 【判断离线状态】如果网络信息存在且可达性为Disconnected，则标记为离线模式
    m_offlineMode = (netInfo && netInfo->reachability() == QNetworkInformation::Reachability::Disconnected);

    // ========== 检测编译器 ==========
    detectTool("gcc", {"gcc", "gcc.exe"});       // 【检测GCC编译器】Linux/Windows下的可能文件名
    detectTool("g++", {"g++", "g++.exe"});       // 【检测G++编译器】C++编译器
    detectTool("clang", {"clang", "clang.exe"}); // 【检测Clang编译器】LLVM编译器前端
    detectTool("cl", {"cl.exe"});                // 【检测MSVC编译器】微软Visual C++编译器

    // ========== 检测构建工具 ==========
    detectTool("cmake", {"cmake", "cmake.exe"}); // 【检测CMake】跨平台构建系统
    detectTool("make", {"make", "mingw32-make", "mingw64-make", "make.exe"}); // 【检测Make】包括MinGW变体
    detectTool("ninja", {"ninja", "ninja.exe"}); // 【检测Ninja】高性能构建工具

    // ========== 检测Qt工具 ==========
    detectTool("qmake", {"qmake", "qmake.exe"}); // 【检测qmake】Qt项目的构建工具
    detectTool("moc", {"moc", "moc.exe"});       // 【检测moc】Qt元对象编译器
    detectTool("uic", {"uic", "uic.exe"});       // 【检测uic】Qt用户界面编译器

    // ========== 检测Python ==========
    detectTool("python", {"python", "python3", "python.exe", "python3.exe"}); // 【检测Python解释器】
    detectTool("pip", {"pip", "pip3", "pip.exe", "pip3.exe"});               // 【检测pip包管理器】

    // ========== 检测其他工具 ==========
    detectTool("git", {"git", "git.exe"});       // 【检测Git】版本控制工具

    emit detectionCompleted();  // 【发射信号】通知其他组件环境检测已完成
}

/**
 * @brief 检测单个工具的私有方法
 * @param name 工具的逻辑名称（如"gcc"、"cmake"）
 * @param possibleNames 该工具在系统中可能的文件名列表
 *
 * 遍历possibleNames中的每个名称，使用where(Windows)或which(Linux/Mac)命令查找可执行文件路径。
 * 如果找到则记录路径、版本，并标记为可用。
 */
void EnvironmentDetector::detectTool(const QString& name, const QStringList& possibleNames)
{
    ToolInfo info;           // 【创建工具信息结构体】存储本次检测结果
    info.name = name;        // 【记录逻辑名】如"gcc"
    info.available = false;  // 【初始标记为不可用】后续如果找到则改为true

    // 【遍历候选文件名】尝试每个可能的名称，直到找到可用的
    for (const QString& toolName : possibleNames) {
        QProcess process;  // 【创建进程对象】用于运行系统查找命令
#ifdef Q_OS_WIN  // 【条件编译】如果是Windows系统
        process.start("where", QStringList() << toolName);  // 【Windows命令】where用于查找可执行文件位置
#else            // 【非Windows】Linux或macOS
        process.start("which", QStringList() << toolName);  // 【Unix命令】which用于查找可执行文件位置
#endif

        if (process.waitForFinished(3000)) {  // 【等待命令完成】最多等待3秒
            // 【读取标准输出】where/which命令会输出找到的完整路径
            QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
            // 【读取标准错误】某些情况下错误输出也有用
            QString error = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();

            // 【判断查找成功】输出非空且退出码为0表示命令成功执行
            if (!output.isEmpty() && process.exitCode() == 0) {
                QStringList lines = output.split('\n');  // 【按行分割】where可能返回多行（多个匹配）
                QString path = lines.first().trimmed();   // 【取第一个匹配】通常第一个就是想要的

                QFileInfo fileInfo(path);  // 【获取文件信息】
                if (fileInfo.exists() && fileInfo.isFile()) {  // 【验证有效性】确认文件确实存在且是普通文件
                    info.path = path;                          // 【记录路径】保存完整路径
                    info.version = getToolVersion(path);       // 【获取版本】运行工具查询版本号
                    info.available = true;                     // 【标记可用】该工具可用
                    break;  // 【跳出循环】已找到可用工具，无需继续尝试其他名称
                }
            }
        }
    }

    m_tools[name] = info;  // 【保存结果】将检测信息存入成员变量映射中，键为工具逻辑名
}

/**
 * @brief 获取工具的版本信息
 * @param path 工具的完整可执行文件路径
 * @return 版本信息字符串（第一行输出），如果无法获取则返回空字符串
 *
 * 根据工具名称判断应使用哪个参数（--version），运行后捕获第一行输出。
 */
QString EnvironmentDetector::getToolVersion(const QString& path)
{
    QProcess process;  // 【创建进程对象】

    QStringList args;  // 【参数列表】存储传递给工具的参数
    QString toolName = QFileInfo(path).baseName().toLower();  // 【提取文件名】不含路径和扩展名，转小写便于比较

    // 【判断工具类型】不同类型工具都用--version参数
    if (toolName.contains("gcc") || toolName.contains("g++") || toolName.contains("clang")) {
        args = {"--version"};  // 【GCC/Clang系列】--version输出版本信息
    } else if (toolName.contains("cmake")) {
        args = {"--version"};  // 【CMake】--version
    } else if (toolName.contains("ninja")) {
        args = {"--version"};  // 【Ninja】--version
    } else if (toolName.contains("python")) {
        args = {"--version"};  // 【Python】--version
    } else if (toolName.contains("qmake")) {
        args = {"--version"};  // 【qmake】--version
    } else if (toolName.contains("git")) {
        args = {"--version"};  // 【Git】--version
    } else {
        return "";  // 【未知工具】不尝试获取版本，直接返回空字符串
    }

    process.start(path, args);  // 【启动进程】运行工具并传入版本参数
    if (process.waitForFinished(3000)) {  // 【等待完成】最多3秒
        QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 【读取输出】
        QStringList lines = output.split('\n');  // 【按行分割】
        if (!lines.isEmpty()) {  // 【检查非空】
            return lines.first().trimmed();  // 【返回第一行】通常是版本信息的主行，如"gcc (Ubuntu 11.2.0) 11.2.0"
        }
    }

    return "";  // 【获取失败】超时或没有输出，返回空字符串
}

/**
 * @brief 获取所有已检测工具的信息
 * @return 只读引用，返回工具名到ToolInfo的映射
 */
const QMap<QString, EnvironmentDetector::ToolInfo>& EnvironmentDetector::tools() const
{
    return m_tools;  // 【返回映射】返回存储所有工具信息的字典
}

/**
 * @brief 查询指定工具是否可用
 * @param name 工具逻辑名称
 * @return true表示该工具已被找到且可用
 */
bool EnvironmentDetector::isToolAvailable(const QString& name) const
{
    return m_tools.contains(name) && m_tools[name].available;  // 【检查存在且可用】先在映射中查找，再确认available标志
}

/**
 * @brief 获取指定工具的完整路径
 * @param name 工具逻辑名称
 * @return 完整路径字符串，如果不可用则返回空字符串
 */
QString EnvironmentDetector::toolPath(const QString& name) const
{
    if (m_tools.contains(name) && m_tools[name].available) {  // 【检查可用性】
        return m_tools[name].path;  // 【返回路径】如"/usr/bin/gcc"
    }
    return "";  // 【不可用】返回空字符串
}

/**
 * @brief 判断是否处于离线模式
 * @return true表示当前无网络连接
 */
bool EnvironmentDetector::isOfflineMode() const
{
    return m_offlineMode;  // 【返回标志】检测时根据网络状态设置
}

/**
 * @brief 获取可用编译器列表
 * @return 字符串列表，包含系统中找到的所有编译器名称
 */
QStringList EnvironmentDetector::availableCompilers() const
{
    QStringList compilers;  // 【创建空列表】
    if (isToolAvailable("gcc")) compilers << "gcc";      // 【检查GCC】如果可用则加入列表
    if (isToolAvailable("g++")) compilers << "g++";      // 【检查G++】
    if (isToolAvailable("clang")) compilers << "clang";  // 【检查Clang】
    if (isToolAvailable("cl")) compilers << "cl";        // 【检查MSVC】
    return compilers;  // 【返回列表】如["gcc", "g++"]
}

/**
 * @brief 获取可用构建工具列表
 * @return 字符串列表，包含系统中找到的所有构建工具名称
 */
QStringList EnvironmentDetector::availableBuildTools() const
{
    QStringList tools;  // 【创建空列表】
    if (isToolAvailable("cmake")) tools << "cmake";    // 【检查CMake】
    if (isToolAvailable("make")) tools << "make";      // 【检查Make】
    if (isToolAvailable("ninja")) tools << "ninja";    // 【检查Ninja】
    return tools;  // 【返回列表】如["cmake", "ninja"]
}
