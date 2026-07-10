/**
 * @file securitymanager.cpp
 * @brief 安全管理器实现
 *
 * @details
 * SecurityManager 是系统的安全防线，防止 AI 生成的计划执行危险操作。
 * 在本地运行的 AI 系统中，安全检查尤为重要，因为 AI 可能生成意外的危险命令。
 *
 * 安全设计原则：
 * 1. 默认拒绝：未知命令默认不允许执行
 * 2. 黑名单优先：先检查是否在禁止列表中
 * 3. 白名单校验：只允许明确列出的安全命令
 * 4. 大小写不敏感：防止通过大小写变体绕过检查
 * 5. 路径清理：防止目录遍历攻击和系统关键目录访问
 *
 * @warning 当前实现是基础版本，生产环境建议增加：
 * - 命令参数解析和过滤
 * - 正则表达式匹配
 * - 沙箱执行环境
 * - 操作审计日志
 * - 用户二次确认机制
 */

#include "securitymanager.h"  // 【引入】自己的头文件
#include <QFileInfo>          // 【引入】Qt文件信息类

/**
 * @brief 检查命令是否允许执行
 * @param command 要检查的命令字符串
 * @return true 如果命令安全，允许执行
 *
 * @implementation
 * 提取命令的第一个词（主命令），转为小写后进行黑白名单检查。
 * 采用"默认拒绝"策略：不在白名单中的命令一律拒绝。
 */
bool SecurityManager::isCommandAllowed(const QString& command)
{
    // 【提取】获取命令的第一个词（主命令）
    // 例如 "mkdir test" → "mkdir"
    // 例如 "git clone ..." → "git"
    QString cmd = command.trimmed().split(' ').first().toLower();  // 【处理】去空白、分割、取第一个、转小写

    // 【检查】如果命令在禁止列表中，直接拒绝
    if (forbiddenCommands().contains(cmd)) {  // 【判断】黑名单匹配
        return false;  // 【返回】拒绝执行
    }

    // 【检查】如果命令在允许列表中，允许执行
    if (allowedCommands().contains(cmd)) {  // 【判断】白名单匹配
        return true;  // 【返回】允许执行
    }

    // 【默认拒绝】未知命令一律不允许执行
    return false;  // 【返回】拒绝执行
}

/**
 * @brief 清理和验证文件路径
 * @param path 原始文件路径
 * @return 清理后的安全路径，如果不安全则返回空字符串
 *
 * @implementation
 * 1. 统一路径分隔符（反斜杠 → 正斜杠）
 * 2. 检查是否包含 ".." 目录遍历
 * 3. 检查是否指向系统关键目录
 * 4. 返回安全路径或空字符串
 */
QString SecurityManager::sanitizePath(const QString& path)
{
    QString cleanPath = path;  // 【复制】创建可修改的副本

    // 【统一】将反斜杠替换为正斜杠，确保跨平台一致性
    cleanPath.replace("\\", "/");  // 【替换】所有反斜杠变正斜杠

    // 【检查】防止目录遍历攻击（如 ../../../etc/passwd）
    if (cleanPath.contains("../")) {  // 【判断】是否包含目录遍历符号
        return QString();  // 【返回】空字符串表示拒绝
    }

    // 【检查】防止访问系统关键目录
    // Windows 系统目录保护
    if (cleanPath.startsWith("C:/Windows", Qt::CaseInsensitive) ||   // 【判断】Windows系统目录
        cleanPath.startsWith("C:/Program Files", Qt::CaseInsensitive) ||  // 【判断】Program Files
        cleanPath.startsWith("/etc", Qt::CaseInsensitive) ||         // 【判断】Linux配置目录
        cleanPath.startsWith("/sys", Qt::CaseInsensitive) ||         // 【判断】Linux系统目录
        cleanPath.startsWith("/proc", Qt::CaseInsensitive)) {        // 【判断】Linux进程目录
        return QString();  // 【返回】空字符串表示拒绝
    }

    return cleanPath;  // 【返回】清理后的安全路径
}

/**
 * @brief 禁止执行的命令列表（黑名单）
 * @return 危险命令列表
 *
 * @details
 * 这些命令可能导致：
 * - 数据丢失：rm、del、format 等
 * - 系统损坏：dd、fdisk、mkfs 等
 * - 安全漏洞：curl/wget 下载执行、powershell 远程脚本等
 *
 * @note 列表使用全小写，因为 isCommandAllowed 会将命令转为小写后匹配。
 */
QStringList SecurityManager::forbiddenCommands()
{
    return QStringList()  // 【创建】字符串列表
        << "rm"          // 【添加】删除文件/目录（Unix）
        << "del"         // 【添加】删除文件（Windows）
        << "format"      // 【添加】格式化磁盘
        << "fdisk"       // 【添加】磁盘分区操作
        << "dd"          // 【添加】直接写磁盘（可破坏数据）
        << "mkfs"        // 【添加】创建文件系统（格式化）
        << "curl"        // 【添加】下载工具（可能下载恶意脚本）
        << "wget"        // 【添加】下载工具
        << "powershell"  // 【添加】PowerShell（可能执行远程脚本）
        << "reg"         // 【添加】Windows注册表操作
        << "shutdown"    // 【添加】关机/重启
        << "reboot";     // 【添加】重启系统
}

/**
 * @brief 允许执行的命令列表（白名单）
 * @return 安全命令列表
 *
 * @details
 * 这些命令被认为是日常开发中相对安全的操作：
 * - 文件/目录操作：mkdir、touch、cat、type、copy、move
 * - 版本控制：git
 * - 代码执行：python、node、gcc、cmake
 * - 文本处理：echo、sed、grep、find
 * - 信息查看：ls、dir、pwd、cd
 *
 * @note 白名单策略是"默认拒绝"的补充：只有明确列出的命令才允许执行。
 *       这可以防止 AI 使用开发者未预料到的危险命令。
 */
QStringList SecurityManager::allowedCommands()
{
    return QStringList()  // 【创建】字符串列表
        << "echo"        // 【添加】输出文本
        << "cat"         // 【添加】查看文件内容（Unix）
        << "type"        // 【添加】查看文件内容（Windows）
        << "ls"          // 【添加】列出目录（Unix）
        << "dir"         // 【添加】列出目录（Windows）
        << "mkdir"       // 【添加】创建目录
        << "cd"          // 【添加】切换目录
        << "pwd"         // 【添加】显示当前路径（Unix）
        << "copy"        // 【添加】复制文件（Windows）
        << "cp"          // 【添加】复制文件（Unix）
        << "move"        // 【添加】移动文件（Windows）
        << "mv"          // 【添加】移动文件（Unix）
        << "git"         // 【添加】版本控制
        << "python"      // 【添加】Python解释器
        << "python3"     // 【添加】Python3解释器
        << "node"        // 【添加】Node.js
        << "npm"         // 【添加】NPM包管理器
        << "cmake"       // 【添加】CMake构建工具
        << "make"        // 【添加】Make构建工具
        << "gcc"         // 【添加】GCC编译器
        << "g++"         // 【添加】G++编译器
        << "clang"       // 【添加】Clang编译器
        << "javac"       // 【添加】Java编译器
        << "java"        // 【添加】Java虚拟机
        << "go"          // 【添加】Go编译器
        << "rustc"       // 【添加】Rust编译器
        << "cargo"       // 【添加】Rust包管理器
        << "code"        // 【添加】VS Code编辑器
        << "notepad"     // 【添加】记事本（Windows）
        << "touch"       // 【添加】创建空文件（Unix）
        << "find"        // 【添加】查找文件
        << "grep"        // 【添加】文本搜索
        << "sed"         // 【添加】流编辑器
        << "awk"         // 【添加】文本处理
        << "tar"         // 【添加】归档工具
        << "zip"         // 【添加】压缩工具
        << "unzip";      // 【添加】解压工具
}
