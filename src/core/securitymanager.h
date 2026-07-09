#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QString>
#include <QVariantMap>
#include <QList>

/**
 * @brief 安全管理器 - 提供全局安全检查和参数验证
 *
 * SecurityManager提供以下安全功能：
 * - 参数验证：检查必填参数是否存在、类型是否正确
 * - 路径安全检查：防止路径遍历攻击（如 ../../）
 * - 权限检查：检查文件/目录访问权限
 * - 命令安全：检查命令是否包含危险操作符
 */
class SecurityManager
{
public:
    /**
     * @brief 验证参数是否完整
     * @param args 参数映射
     * @param requiredParams 必填参数列表
     * @return 验证结果，包含success和error字段
     */
    static QVariantMap validateParams(const QVariantMap& args, const QList<QString>& requiredParams);

    /**
     * @brief 验证路径安全性
     * @param path 要检查的路径
     * @param allowParent 是否允许父目录访问
     * @return 验证结果，包含success、error和sanitizedPath字段
     */
    static QVariantMap validatePath(const QString& path, bool allowParent = false);

    /**
     * @brief 检查文件是否可读取
     * @param path 文件路径
     * @return 检查结果
     */
    static QVariantMap checkReadPermission(const QString& path);

    /**
     * @brief 检查文件是否可写入
     * @param path 文件路径
     * @return 检查结果
     */
    static QVariantMap checkWritePermission(const QString& path);

    /**
     * @brief 检查目录是否可访问
     * @param path 目录路径
     * @return 检查结果
     */
    static QVariantMap checkDirectoryAccess(const QString& path);

    /**
     * @brief 验证命令安全性
     * @param command 命令字符串
     * @param allowedCommands 允许的命令列表
     * @return 验证结果
     */
    static QVariantMap validateCommand(const QString& command, const QStringList& allowedCommands);

    /**
     * @brief 检查是否为危险命令
     * @param command 命令字符串
     * @return true表示危险命令
     */
    static bool isDangerousCommand(const QString& command);

    /**
     * @brief 清理文件名（移除非法字符）
     * @param fileName 原始文件名
     * @return 清理后的文件名
     */
    static QString sanitizeFileName(const QString& fileName);

private:
    /**
     * @brief 检查路径是否包含路径遍历
     * @param path 路径字符串
     * @return true表示包含路径遍历
     */
    static bool containsPathTraversal(const QString& path);

    /**
     * @brief 获取危险命令列表
     */
    static QStringList dangerousCommands();

    /**
     * @brief 获取危险操作符列表
     */
    static QStringList dangerousOperators();
};

#endif