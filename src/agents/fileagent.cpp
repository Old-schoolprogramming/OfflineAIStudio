/**
 * @file fileagent.cpp
 * @brief 文件操作代理实现
 *
 * @details
 * FileAgent是Agent基类的具体实现，负责文件读写和目录管理。
 * 该代理注册了6个工具：readFile、writeFile、listFiles、createDirectory、deleteFile、fileExists，
 * 每个工具对应一个私有方法，直接操作本地文件系统。
 * 所有工具执行结果以QVariantMap形式返回，统一包含success和result/error字段。
 */

#include "fileagent.h"
#include "tool.h"
#include <QFile>
#include <QDir>
#include <QTextStream>

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 调用initializeTools()创建并注册所有文件操作工具。
 */
FileAgent::FileAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

/**
 * @brief 析构函数
 *
 * 释放initializeTools中动态分配的所有Tool对象。
 */
FileAgent::~FileAgent()
{
    qDeleteAll(m_tools);
}

/**
 * @brief 获取代理名称
 * @return 固定返回"FileAgent"
 */
QString FileAgent::name() const
{
    return "FileAgent";
}

/**
 * @brief 获取代理描述
 * @return 代理功能描述字符串
 */
QString FileAgent::description() const
{
    return "文件操作代理，负责文件读写和目录管理";
}

/**
 * @brief 获取代理类型
 * @return 固定返回FileAgentType
 */
Agent::AgentType FileAgent::type() const
{
    return FileAgentType;
}

/**
 * @brief 获取工具列表
 * @return 当前代理注册的所有Tool指针列表
 */
QList<Tool*> FileAgent::tools() const
{
    return m_tools;
}

/**
 * @brief 初始化并注册所有文件操作工具
 *
 * @details
 * 注册的工具及参数说明：
 * - readFile: 读取文件内容，必填参数path
 * - writeFile: 写入文件内容，必填参数path、content，可选参数append（默认false）
 * - listFiles: 列出目录内容，必填参数dir
 * - createDirectory: 创建目录（支持多级），必填参数path
 * - deleteFile: 删除文件，必填参数path
 * - fileExists: 检查文件是否存在，必填参数path
 */
void FileAgent::initializeTools()
{
    Tool* readFileTool = new Tool("readFile", "读取文件内容");
    readFileTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(readFileTool);

    Tool* writeFileTool = new Tool("writeFile", "写入文件内容");
    writeFileTool->addParameter("path", "string", "文件路径", true);
    writeFileTool->addParameter("content", "string", "文件内容", true);
    writeFileTool->addParameter("append", "bool", "是否追加模式", false, "false");
    m_tools.append(writeFileTool);

    Tool* listFilesTool = new Tool("listFiles", "列出目录下的文件");
    listFilesTool->addParameter("dir", "string", "目录路径", true);
    m_tools.append(listFilesTool);

    Tool* createDirTool = new Tool("createDirectory", "创建目录");
    createDirTool->addParameter("path", "string", "目录路径", true);
    m_tools.append(createDirTool);

    Tool* deleteFileTool = new Tool("deleteFile", "删除文件");
    deleteFileTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(deleteFileTool);

    Tool* fileExistsTool = new Tool("fileExists", "检查文件是否存在");
    fileExistsTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(fileExistsTool);
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
QVariantMap FileAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "readFile") {
        return readFile(args);
    } else if (toolName == "writeFile") {
        return writeFile(args);
    } else if (toolName == "listFiles") {
        return listFiles(args);
    } else if (toolName == "createDirectory") {
        return createDirectory(args);
    } else if (toolName == "deleteFile") {
        return deleteFile(args);
    } else if (toolName == "fileExists") {
        return fileExists(args);
    }

    // 未知工具返回通用错误
    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

/**
 * @brief 读取文件内容
 * @param args 参数映射，必须包含"path"键
 * @return 若成功，result字段为文件内容字符串；若失败，error字段包含原因
 *
 * @details
 * 执行流程：
 * 1. 提取path参数
 * 2. 检查文件是否存在
 * 3. 以只读文本模式打开文件
 * 4. 使用QTextStream读取全部内容
 * 5. 关闭文件并返回结果
 */
QVariantMap FileAgent::readFile(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFile file(path);
    if (!file.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + path;
        return result;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    result["success"] = true;
    result["result"] = content;
    return result;
}

/**
 * @brief 写入文件内容
 * @param args 参数映射，必须包含"path"和"content"，可选"append"
 * @return 若成功，result字段包含成功提示；若失败，error字段包含原因
 *
 * @details
 * 执行流程：
 * 1. 提取path、content和append参数（append默认false）
 * 2. 根据append决定打开模式：WriteOnly|Append|Text 或 WriteOnly|Text
 * 3. 打开文件并写入内容
 * 4. 关闭文件并返回结果
 */
QVariantMap FileAgent::writeFile(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString content = args.value("content").toString();
    bool append = args.value("append", false).toBool();

    QFile file(path);
    QIODevice::OpenMode mode = append ? 
        (QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) :
        (QIODevice::WriteOnly | QIODevice::Text);

    if (!file.open(mode)) {
        result["success"] = false;
        result["error"] = "无法打开文件进行写入: " + path;
        return result;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    result["success"] = true;
    result["result"] = "文件写入成功: " + path;
    return result;
}

/**
 * @brief 列出目录内容
 * @param args 参数映射，必须包含"dir"键
 * @return 若成功，result字段为文件和子目录名称列表（以换行分隔）；若失败返回错误信息
 *
 * @details
 * 使用QDir::entryList获取目录下所有文件和子目录（排除.和..），
 * 将QStringList用换行符连接为单个字符串返回。
 */
QVariantMap FileAgent::listFiles(const QVariantMap& args)
{
    QVariantMap result;
    QString dirPath = args.value("dir").toString();

    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + dirPath;
        return result;
    }

    QStringList files = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QString resultStr = files.join("\n");

    result["success"] = true;
    result["result"] = resultStr;
    return result;
}

/**
 * @brief 创建目录
 * @param args 参数映射，必须包含"path"键
 * @return 若成功返回创建成功提示；若失败返回错误信息
 *
 * @details
 * 使用QDir::mkpath创建目录，支持自动创建多级父目录。
 */
QVariantMap FileAgent::createDirectory(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QDir dir;
    if (!dir.mkpath(path)) {
        result["success"] = false;
        result["error"] = "无法创建目录: " + path;
        return result;
    }

    result["success"] = true;
    result["result"] = "目录创建成功: " + path;
    return result;
}

/**
 * @brief 删除文件
 * @param args 参数映射，必须包含"path"键
 * @return 若成功返回删除成功提示；若失败返回错误信息
 *
 * @details
 * 先检查文件是否存在，再调用QFile::remove执行删除。
 * 注意：该方法仅删除文件，不删除目录。
 */
QVariantMap FileAgent::deleteFile(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFile file(path);
    if (!file.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    if (!file.remove()) {
        result["success"] = false;
        result["error"] = "无法删除文件: " + path;
        return result;
    }

    result["success"] = true;
    result["result"] = "文件删除成功: " + path;
    return result;
}

/**
 * @brief 检查文件是否存在
 * @param args 参数映射，必须包含"path"键
 * @return 固定success=true，result字段为"文件存在"或"文件不存在"
 *
 * @details
 * 使用QFileInfo检查路径是否存在，不区分文件和目录。
 */
QVariantMap FileAgent::fileExists(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFileInfo fileInfo(path);
    bool exists = fileInfo.exists();

    result["success"] = true;
    result["result"] = exists ? "文件存在" : "文件不存在";
    return result;
}
