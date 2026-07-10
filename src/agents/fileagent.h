// 【文件说明】fileagent.h - 文件操作Agent的头文件
// 【作用】声明 FileAgent 类，定义文件和目录操作相关的接口
// 【注意事项】头文件只声明类和函数，具体实现在 fileagent.cpp 中

#ifndef FILEAGENT_H  // 防止重复包含的宏定义开始
#define FILEAGENT_H  // 定义 FILEAGENT_H 宏，避免同一文件被多次编译

#include "agent.h"  // 引入 Agent 基类头文件，FileAgent 继承自 Agent
#include <QList>    // 引入 Qt 列表容器，用于存储工具对象列表
#include <QFile>    // 引入 Qt 文件操作类，用于文件读写
#include <QDir>     // 引入 Qt 目录操作类，用于目录管理

class Tool;  // 前向声明 Tool 类，告诉编译器有这个类，减少编译依赖

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
class FileAgent : public Agent  // FileAgent 继承自 Agent 基类
{
    Q_OBJECT  // Qt 宏，启用信号与槽、元对象系统等 Qt 特性

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针，用于内存管理和对象树层次结构
     * 【说明】创建 FileAgent 实例时自动调用，初始化所有文件操作工具
     */
    explicit FileAgent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     * 【说明】销毁 FileAgent 实例时自动调用，释放所有动态分配的工具对象
     */
    ~FileAgent();

    /**
     * @brief 获取Agent名称
     * @return 固定返回字符串 "FileAgent"
     * 【说明】重写父类虚函数，用于标识此 Agent 的类型
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 返回文件操作Agent的功能描述字符串
     * 【说明】重写父类虚函数，向系统说明此 Agent 能做什么
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return 返回枚举值 FileAgentType
     * 【说明】重写父类虚函数，返回此 Agent 的类型标识
     */
    AgentType type() const override;

    /**
     * @brief 获取所有工具
     * @return 返回 Tool 指针的列表
     * 【说明】重写父类虚函数，返回此 Agent 注册的所有可用工具
     */
    QList<Tool*> tools() const override;

    /**
     * @brief 执行指定工具
     * @param toolName 要执行的工具名称字符串（如 "readFile"）
     * @param args 工具参数字典，键值对形式（如 {"path": "/tmp/test.txt"}）
     * @return 执行结果字典，包含 success（是否成功）和 result/error（结果或错误信息）
     * 【说明】重写父类虚函数，根据工具名称分发到对应的私有实现方法
     */
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    /**
     * @brief 初始化所有工具
     * 【说明】在构造函数中调用，创建所有这个Agent拥有的工具并注册到 m_tools 列表中
     */
    void initializeTools();

    QList<Tool*> m_tools;  ///< 工具列表成员变量，存储此 Agent 拥有的所有 Tool 对象指针

    /**
     * @brief 读取文件内容
     * @param args 参数映射，必须包含 "path" 键（文件路径）
     * @return 若成功，result 字段为文件内容字符串；若失败，error 字段包含原因
     * 【说明】对应工具 "readFile"，用于读取指定路径的文本文件内容
     */
    QVariantMap readFile(const QVariantMap& args);

    /**
     * @brief 写入文件内容
     * @param args 参数映射，必须包含 "path"（文件路径）和 "content"（内容），可选 "append"（是否追加）
     * @return 写入结果字典
     * 【说明】对应工具 "writeFile"，用于创建新文件或覆盖/追加已有文件
     */
    QVariantMap writeFile(const QVariantMap& args);

    /**
     * @brief 列出目录下的文件和子目录
     * @param args 参数映射，必须包含 "dir" 键（目录路径）
     * @return 文件列表 JSON 字符串
     * 【说明】对应工具 "listFiles"，用于查看指定目录中的内容
     */
    QVariantMap listFiles(const QVariantMap& args);

    /**
     * @brief 创建目录（支持多级创建）
     * @param args 参数映射，必须包含 "path" 键（目录路径）
     * @return 创建结果字典
     * 【说明】对应工具 "createDirectory"，自动创建不存在的父目录
     */
    QVariantMap createDirectory(const QVariantMap& args);

    /**
     * @brief 删除文件
     * @param args 参数映射，必须包含 "path" 键（文件路径）
     * @return 删除结果字典
     * 【说明】对应工具 "deleteFile"，仅删除文件，不删除目录
     */
    QVariantMap deleteFile(const QVariantMap& args);

    /**
     * @brief 检查文件是否存在
     * @param args 参数映射，必须包含 "path" 键（文件或目录路径）
     * @return 检查结果字典，包含存在状态和文件信息
     * 【说明】对应工具 "fileExists"，用于判断路径是否存在
     */
    QVariantMap fileExists(const QVariantMap& args);

    /**
     * @brief 复制文件
     * @param args 参数映射，必须包含 "source"（源文件路径）和 "dest"（目标路径）
     * @return 复制结果字典
     * 【说明】对应工具 "copyFile"，将源文件复制到目标位置
     */
    QVariantMap copyFile(const QVariantMap& args);

    /**
     * @brief 移动/重命名文件
     * @param args 参数映射，必须包含 "source"（源文件路径）和 "dest"（目标路径）
     * @return 移动结果字典
     * 【说明】对应工具 "moveFile"，支持同盘重命名和跨盘移动
     */
    QVariantMap moveFile(const QVariantMap& args);

    /**
     * @brief 递归搜索目录下的文件
     * @param args 参数映射，必须包含 "dir"，可选 "pattern"（匹配模式，默认"*"）和 "maxDepth"（最大深度，默认-1无限）
     * @return 匹配的文件列表 JSON 字符串
     * 【说明】对应工具 "searchFiles"，使用通配符在目录树中查找文件
     */
    QVariantMap searchFiles(const QVariantMap& args);

    /**
     * @brief 重命名文件或目录
     * @param args 参数映射，必须包含 "oldName"（原路径）和 "newName"（新路径）
     * @return 重命名结果字典
     * 【说明】对应工具 "renameFile"，修改文件或目录的名称
     */
    QVariantMap renameFile(const QVariantMap& args);

    /**
     * @brief 删除目录（递归删除所有内容）
     * @param args 参数映射，必须包含 "path"，可选 "force"（是否强制删除只读文件，默认false）
     * @return 删除结果字典
     * 【说明】对应工具 "deleteDirectory"，删除目录及其内部所有文件和子目录
     */
    QVariantMap deleteDirectory(const QVariantMap& args);

    /**
     * @brief 获取文件详细信息
     * @param args 参数映射，必须包含 "path" 键（文件路径）
     * @return 文件信息 JSON 字符串，包含大小、修改时间、类型等
     * 【说明】对应工具 "getFileInfo"，获取文件的元数据信息
     */
    QVariantMap getFileInfo(const QVariantMap& args);

    /**
     * @brief 读取文件指定行范围
     * @param args 参数映射，必须包含 "path"，可选 "startLine"（起始行，默认1）和 "endLine"（结束行，默认-1到末尾）
     * @return 指定行的内容字符串
     * 【说明】对应工具 "readFileLines"，用于查看大文件的局部内容
     */
    QVariantMap readFileLines(const QVariantMap& args);

    /**
     * @brief 批量写入多个文件
     * @param args 参数映射，必须包含 "files" 数组参数，每项包含 path 和 content
     * @return 批量写入结果字典，列出成功和失败的文件
     * 【说明】对应工具 "batchWriteFiles"，一次操作创建或修改多个文件
     */
    QVariantMap batchWriteFiles(const QVariantMap& args);
};

#endif  // FILEAGENT_H  // 宏定义结束，与开头的 #ifndef 配对
