#include "securitymanager.h"
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

QVariantMap SecurityManager::validateParams(const QVariantMap& args, const QList<QString>& requiredParams)
{
    QVariantMap result;
    QStringList missingParams;

    for (const QString& param : requiredParams) {
        if (!args.contains(param) || args.value(param).isNull() || 
            args.value(param).toString().isEmpty()) {
            missingParams.append(param);
        }
    }

    if (missingParams.isEmpty()) {
        result["success"] = true;
    } else {
        result["success"] = false;
        result["error"] = "缺少必填参数: " + missingParams.join(", ");
    }

    return result;
}

QVariantMap SecurityManager::validatePath(const QString& path, bool allowParent)
{
    QVariantMap result;

    if (path.isEmpty()) {
        result["success"] = false;
        result["error"] = "路径不能为空";
        return result;
    }

    if (!allowParent && containsPathTraversal(path)) {
        result["success"] = false;
        result["error"] = "不允许路径遍历访问";
        return result;
    }

    QString sanitizedPath = path;
    sanitizedPath.replace(QRegularExpression("\\\\+"), "/");
    sanitizedPath.replace(QRegularExpression("/+"), "/");

    while (sanitizedPath.contains("//")) {
        sanitizedPath.replace("//", "/");
    }

    if (!allowParent) {
        while (sanitizedPath.contains("/../")) {
            int idx = sanitizedPath.indexOf("/../");
            if (idx > 0) {
                int prevSlash = sanitizedPath.lastIndexOf("/", idx - 1);
                sanitizedPath.remove(prevSlash, idx - prevSlash + 4);
            } else {
                sanitizedPath.remove(0, 4);
            }
        }
        sanitizedPath.remove(QRegularExpression("^\\.\\./"));
    }

    result["success"] = true;
    result["sanitizedPath"] = sanitizedPath;

    return result;
}

QVariantMap SecurityManager::checkReadPermission(const QString& path)
{
    QVariantMap result;

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    if (!info.isReadable()) {
        result["success"] = false;
        result["error"] = "文件不可读取: " + path;
        return result;
    }

    result["success"] = true;
    result["result"] = "文件可读取";
    return result;
}

QVariantMap SecurityManager::checkWritePermission(const QString& path)
{
    QVariantMap result;

    QFileInfo info(path);
    if (info.exists()) {
        if (!info.isWritable()) {
            result["success"] = false;
            result["error"] = "文件不可写入: " + path;
            return result;
        }
    } else {
        QString parentDir = info.path();
        QDir dir(parentDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                result["success"] = false;
                result["error"] = "无法创建父目录: " + parentDir;
                return result;
            }
        } else if (!QFileInfo(parentDir).isWritable()) {
        result["success"] = false;
        result["error"] = "父目录不可写入: " + parentDir;
        return result;
    }
    }

    result["success"] = true;
    result["result"] = "文件可写入";
    return result;
}

QVariantMap SecurityManager::checkDirectoryAccess(const QString& path)
{
    QVariantMap result;

    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            result["success"] = false;
            result["error"] = "无法创建目录: " + path;
            return result;
        }
    }

    if (!dir.isReadable()) {
        result["success"] = false;
        result["error"] = "目录不可读取: " + path;
        return result;
    }

    if (!QFileInfo(path).isWritable()) {
        result["success"] = false;
        result["error"] = "目录不可写入: " + path;
        return result;
    }

    result["success"] = true;
    result["result"] = "目录可访问";
    return result;
}

QVariantMap SecurityManager::validateCommand(const QString& command, const QStringList& allowedCommands)
{
    QVariantMap result;

    if (command.isEmpty()) {
        result["success"] = false;
        result["error"] = "命令不能为空";
        return result;
    }

    if (isDangerousCommand(command)) {
        result["success"] = false;
        result["error"] = "该命令被安全策略禁止（危险操作）";
        return result;
    }

    QString cmdName = command.split(QRegularExpression("\\s+")).first().toLower();
    cmdName = cmdName.split('"').last();
    cmdName = cmdName.split('\\').last();
    cmdName = cmdName.split('/').last();

    if (!allowedCommands.isEmpty() && !allowedCommands.contains(cmdName)) {
        result["success"] = false;
        result["error"] = QString("不允许执行命令: %1").arg(cmdName);
        return result;
    }

    result["success"] = true;
    return result;
}

bool SecurityManager::isDangerousCommand(const QString& command)
{
    QString lowerCmd = command.toLower();

    for (const QString& dangerousCmd : dangerousCommands()) {
        if (lowerCmd.contains(dangerousCmd)) {
            return true;
        }
    }

    for (const QString& dangerousOp : dangerousOperators()) {
        if (lowerCmd.contains(dangerousOp)) {
            return true;
        }
    }

    return false;
}

QString SecurityManager::sanitizeFileName(const QString& fileName)
{
    QString sanitized = fileName;
    sanitized.remove(QRegularExpression("[\\\\/:*?\"<>|]"));
    sanitized.remove(QRegularExpression("[\\x00-\\x1F]"));
    return sanitized.trimmed();
}

bool SecurityManager::containsPathTraversal(const QString& path)
{
    QString normalized = path.toLower();
    normalized.replace("\\", "/");

    return normalized.contains("/../") || 
           normalized.startsWith("../") || 
           normalized.startsWith("./../") ||
           normalized.contains("..\\") ||
           normalized.startsWith("..\\") ||
           normalized.startsWith(".\\..\\");
}

QStringList SecurityManager::dangerousCommands()
{
    return {
        "format", "rd /s", "rm -rf /", "rm -rf /*", "mkfs", "dd if",
        "del /f /s", "rmdir /s", ":(){:|:&};:", "curl | sh", "wget | sh",
        "chmod -R 777 /", "chown -R", "rm -rf ~", "rm -rf .*"
    };
}

QStringList SecurityManager::dangerousOperators()
{
    return {
        " && ", " ; ", " || ", " | ", ">", ">>", "<", "<<",
        "$(", "`", "eval", "exec", "system", "powershell", "cmd.exe"
    };
}