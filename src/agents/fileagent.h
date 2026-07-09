#ifndef FILEAGENT_H
#define FILEAGENT_H

#include "agent.h"
#include <QList>
#include <QFile>
#include <QDir>

class Tool;

/**
 * @brief 文件操作Agent - 专门负责文件和目录的操作
 *
 * FileAgent是一个专业的文件操作专家，它可以：
 * - 读取文件内容
 * - 写入/创建文件
 * - 列出目录中的文件
 * - 创建目录
 * - 删除文件
 * - 检查文件是否存在
 *
 * 当大模型判断用户的问题涉及文件操作时，
 * 就会调用FileAgent的工具来完成任务。
 */
class FileAgent : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit FileAgent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FileAgent();

    /**
     * @brief 获取Agent名称
     * @return "FileAgent"
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 文件操作Agent的功能描述
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return FileAgentType
     */
    AgentType type() const override;

    /**
     * @brief 获取所有工具
     * @return 工具列表
     */
    QList<Tool*> tools() const override;

    /**
     * @brief 执行指定工具
     * @param toolName 工具名称
     * @param args 工具参数
     * @return 执行结果
     */
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    /**
     * @brief 初始化所有工具
     *
     * 在构造函数中调用，创建所有这个Agent拥有的工具。
     */
    void initializeTools();

    QList<Tool*> m_tools; ///< 工具列表

    /**
     * @brief 读取文件内容
     * @param args 包含path参数
     * @return 文件内容
     */
    QVariantMap readFile(const QVariantMap& args);

    /**
     * @brief 写入文件内容
     * @param args 包含path、content、append参数
     * @return 写入结果
     */
    QVariantMap writeFile(const QVariantMap& args);

    /**
     * @brief 列出目录下的文件和子目录
     * @param args 包含dir参数
     * @return 文件列表
     */
    QVariantMap listFiles(const QVariantMap& args);

    /**
     * @brief 创建目录（支持多级创建）
     * @param args 包含path参数
     * @return 创建结果
     */
    QVariantMap createDirectory(const QVariantMap& args);

    /**
     * @brief 删除文件
     * @param args 包含path参数
     * @return 删除结果
     */
    QVariantMap deleteFile(const QVariantMap& args);

    /**
     * @brief 检查文件是否存在
     * @param args 包含path参数
     * @return 检查结果
     */
    QVariantMap fileExists(const QVariantMap& args);

    /**
     * @brief 复制文件或目录
     * @param args 包含source、target、overwrite参数
     * @return 复制结果
     */
    QVariantMap copyFile(const QVariantMap& args);

    /**
     * @brief 移动文件或目录
     * @param args 包含source、target参数
     * @return 移动结果
     */
    QVariantMap moveFile(const QVariantMap& args);

    /**
     * @brief 重命名文件或目录
     * @param args 包含path、newName参数
     * @return 重命名结果
     */
    QVariantMap renameFile(const QVariantMap& args);

    /**
     * @brief 获取文件详细信息
     * @param args 包含path参数
     * @return 文件信息（大小、创建时间、修改时间、权限等）
     */
    QVariantMap fileInfo(const QVariantMap& args);

    /**
     * @brief 搜索文件（按文件名模式匹配）
     * @param args 包含dir、pattern、recursive参数
     * @return 匹配的文件列表
     */
    QVariantMap searchFiles(const QVariantMap& args);

    /**
     * @brief 读取文件指定行范围
     * @param args 包含path、startLine、endLine参数
     * @return 指定行的内容
     */
    QVariantMap readFileLines(const QVariantMap& args);

    /**
     * @brief 在文件指定行写入内容
     * @param args 包含path、startLine、content参数
     * @return 写入结果
     */
    QVariantMap writeFileLines(const QVariantMap& args);

    /**
     * @brief 获取目录大小
     * @param args 包含dir参数
     * @return 目录总大小（字节）
     */
    QVariantMap directorySize(const QVariantMap& args);

    /**
     * @brief 删除目录（递归删除）
     * @param args 包含path参数
     * @return 删除结果
     */
    QVariantMap deleteDirectory(const QVariantMap& args);

    /**
     * @brief 比较两个文件内容
     * @param args 包含file1、file2参数
     * @return 比较结果（是否相同及差异）
     */
    QVariantMap compareFiles(const QVariantMap& args);

    /**
     * @brief 获取文件扩展名
     * @param args 包含path参数
     * @return 文件扩展名及基础名
     */
    QVariantMap fileExtension(const QVariantMap& args);
};

#endif
