/**
 * @file securitymanager.h
 * @brief 安全管理器 —— 系统安全的"守门人"
 *
 * @details
 * SecurityManager 负责审查和过滤可能危害系统的用户输入和操作。
 * 在 AI 直接控制本地系统的场景中，安全检查至关重要。
 *
 * 核心功能：
 * 1. isCommandAllowed() —— 检查命令是否允许执行
 * 2. sanitizePath() —— 清理和验证文件路径
 *
 * 安全策略：
 * - 黑名单机制：禁止执行危险命令（rm -rf、format、dd 等）
 * - 白名单机制：允许安全的常用命令（mkdir、echo、git、python 等）
 * - 路径检查：防止目录遍历攻击（../）
 * - 大小写不敏感匹配：防止绕过检查（如 Rm、FoRmAt）
 *
 * @note 当前实现是单层过滤，未来可考虑添加多层安全策略（如正则匹配、沙箱执行等）。
 */

#ifndef SECURITYMANAGER_H  // 【条件编译】防止头文件被重复包含
#define SECURITYMANAGER_H  // 【宏定义】标记该文件已被包含

#include <QString>     // 【引入】Qt字符串类
#include <QStringList> // 【引入】Qt字符串列表类

/**
 * @brief 安全管理器
 *
 * SecurityManager 是纯工具类（无 QObject 继承），不管理任何状态。
 * 所有方法都是静态的，可以直接调用，无需创建实例。
 *
 * 使用示例：
 *   if (SecurityManager::isCommandAllowed("rm -rf /")) {
 *       // 不会执行到这里，因为 rm 在黑名单中
 *   }
 */
class SecurityManager  // 【类声明】纯静态工具类
{
public:
    /**
     * @brief 检查命令是否允许执行
     * @param command 要检查的命令字符串
     * @return true 如果命令被允许执行
     *
     * @implementation
     * 1. 提取命令中的第一个词（主命令）
     * 2. 将主命令转为小写
     * 3. 检查是否在禁止列表中（黑名单匹配）
     * 4. 检查是否在允许列表中（白名单匹配）
     * 5. 如果在黑名单中 → 返回 false
     * 6. 如果在白名单中 → 返回 true
     * 7. 如果都不在 → 默认返回 false（拒绝未知命令）
     *
     * @note 采用"默认拒绝"策略：未知命令一律不允许执行，防止绕过。
     */
    static bool isCommandAllowed(const QString& command);  // 【静态方法】检查命令安全性

    /**
     * @brief 清理和验证文件路径
     * @param path 要清理的文件路径
     * @return 清理后的安全路径
     *
     * @implementation
     * 1. 将反斜杠统一为正斜杠（Qt 跨平台兼容）
     * 2. 检测并拒绝路径中包含 ".." 的目录遍历攻击
     * 3. 检测并拒绝绝对路径中的系统关键目录（如 C:\Windows、/etc）
     * 4. 返回清理后的路径
     *
     * @note 如果路径包含危险内容，返回空字符串表示拒绝。
     */
    static QString sanitizePath(const QString& path);  // 【静态方法】清理文件路径

private:
    /**
     * @brief 禁止执行的命令列表（黑名单）
     * @return 危险命令列表
     *
     * @note 这些命令可能导致数据丢失、系统损坏或安全漏洞。
     *       包括但不限于：删除、格式化、分区、直接写磁盘等。
     */
    static QStringList forbiddenCommands();  // 【静态私有方法】获取黑名单命令列表

    /**
     * @brief 允许执行的命令列表（白名单）
     * @return 安全命令列表
     *
     * @note 这些命令被认为是相对安全的日常操作。
     *       包括但不限于：文件操作、目录操作、版本控制、代码执行等。
     */
    static QStringList allowedCommands();  // 【静态私有方法】获取白名单命令列表
};

#endif // SECURITYMANAGER_H  // 【条件编译结束】
