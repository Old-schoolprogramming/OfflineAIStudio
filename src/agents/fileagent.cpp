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
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDirIterator>
#include <QRegularExpression>

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

    Tool* copyFileTool = new Tool("copyFile", "复制文件到目标路径");
    copyFileTool->addParameter("source", "string", "源文件路径", true);
    copyFileTool->addParameter("dest", "string", "目标路径", true);
    m_tools.append(copyFileTool);

    Tool* moveFileTool = new Tool("moveFile", "移动或重命名文件");
    moveFileTool->addParameter("source", "string", "源文件路径", true);
    moveFileTool->addParameter("dest", "string", "目标路径", true);
    m_tools.append(moveFileTool);

    Tool* searchFilesTool = new Tool("searchFiles", "在目录中递归搜索匹配文件");
    searchFilesTool->addParameter("dir", "string", "搜索起始目录", true);
    searchFilesTool->addParameter("pattern", "string", "文件名匹配模式（支持通配符 * 和 ?）", false, "*");
    searchFilesTool->addParameter("maxDepth", "int", "最大递归深度（-1 表示无限）", false, "-1");
    m_tools.append(searchFilesTool);

    Tool* renameFileTool = new Tool("renameFile", "重命名文件或目录");
    renameFileTool->addParameter("oldName", "string", "原文件路径", true);
    renameFileTool->addParameter("newName", "string", "新文件路径", true);
    m_tools.append(renameFileTool);

    Tool* deleteDirectoryTool = new Tool("deleteDirectory", "删除目录（递归删除所有内容）");
    deleteDirectoryTool->addParameter("path", "string", "目录路径", true);
    deleteDirectoryTool->addParameter("force", "bool", "是否强制删除（包含只读文件）", false, "false");
    m_tools.append(deleteDirectoryTool);

    Tool* getFileInfoTool = new Tool("getFileInfo", "获取文件详细信息");
    getFileInfoTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(getFileInfoTool);

    Tool* readFileLinesTool = new Tool("readFileLines", "读取文件指定行范围");
    readFileLinesTool->addParameter("path", "string", "文件路径", true);
    readFileLinesTool->addParameter("startLine", "int", "起始行（从1开始）", false, "1");
    readFileLinesTool->addParameter("endLine", "int", "结束行（-1表示到末尾）", false, "-1");
    m_tools.append(readFileLinesTool);

    Tool* batchWriteFilesTool = new Tool("batchWriteFiles", "批量写入多个文件");
    batchWriteFilesTool->addParameter("files", "array", "文件数组，每项包含path和content", true);
    m_tools.append(batchWriteFilesTool);
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
    } else if (toolName == "copyFile") {
        return copyFile(args);
    } else if (toolName == "moveFile") {
        return moveFile(args);
    } else if (toolName == "searchFiles") {
        return searchFiles(args);
    } else if (toolName == "renameFile") {
        return renameFile(args);
    } else if (toolName == "deleteDirectory") {
        return deleteDirectory(args);
    } else if (toolName == "getFileInfo") {
        return getFileInfo(args);
    } else if (toolName == "readFileLines") {
        return readFileLines(args);
    } else if (toolName == "batchWriteFiles") {
        return batchWriteFiles(args);
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

    QFileInfo fileInfo(path);
    QString parentDir = fileInfo.path();

    if (!parentDir.isEmpty()) {
        QDir dir(parentDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                result["success"] = false;
                result["error"] = "无法创建目录: " + parentDir;
                return result;
            }
        }
    }

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

    QJsonArray filesArray;
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo& info : fileInfoList) {
        QJsonObject fileObj;
        fileObj["name"] = info.fileName();
        fileObj["path"] = info.absoluteFilePath();
        fileObj["size"] = info.size();
        fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
        fileObj["isDir"] = info.isDir();
        fileObj["icon"] = info.isDir() ? "folder" : "file";
        filesArray.append(fileObj);
    }

    result["success"] = true;
    result["result"] = QJsonDocument(filesArray).toJson(QJsonDocument::Compact);
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

    QJsonObject fileObj;
    fileObj["path"] = path;
    fileObj["exists"] = exists;
    if (exists) {
        fileObj["name"] = fileInfo.fileName();
        fileObj["size"] = fileInfo.size();
        fileObj["modified"] = fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss");
        fileObj["isDir"] = fileInfo.isDir();
        fileObj["icon"] = fileInfo.isDir() ? "folder" : "file";
    }

    result["success"] = true;
    result["result"] = QJsonDocument(fileObj).toJson(QJsonDocument::Compact);
    return result;
}

/**
 * @brief 复制文件到目标路径
 * @param args 参数映射，必须包含"source"和"dest"键
 * @return 若成功返回复制成功提示；若失败返回错误信息
 *
 * @details
 * 支持复制到新路径或覆盖已存在的文件。
 * 如果目标路径的父目录不存在，会自动创建。
 */
QVariantMap FileAgent::copyFile(const QVariantMap& args)
{
    QVariantMap result;
    QString source = args.value("source").toString();
    QString dest = args.value("dest").toString();

    if (!QFile::exists(source)) {
        result["success"] = false;
        result["error"] = "源文件不存在: " + source;
        return result;
    }

    // 确保目标目录存在
    QFileInfo destInfo(dest);
    QDir().mkpath(destInfo.absolutePath());

    // 如果目标已存在，先删除
    if (QFile::exists(dest)) {
        QFile::remove(dest);
    }

    if (!QFile::copy(source, dest)) {
        result["success"] = false;
        result["error"] = "文件复制失败: " + source + " → " + dest;
        return result;
    }

    result["success"] = true;
    result["result"] = "文件复制成功: " + source + " → " + dest;
    return result;
}

/**
 * @brief 移动或重命名文件
 * @param args 参数映射，必须包含"source"和"dest"键
 * @return 若成功返回移动成功提示；若失败返回错误信息
 *
 * @details
 * 优先尝试 QFile::rename（原子操作，同盘高效），
 * 失败时退化为 复制 + 删除 的跨盘移动方式。
 */

QVariantMap FileAgent::moveFile(const QVariantMap& args)
{
    QVariantMap result;
    QString source = args.value("source").toString();
    QString dest = args.value("dest").toString();

    if (!QFile::exists(source)) {
        result["success"] = false;
        result["error"] = "源文件不存在: " + source;
        return result;
    }

    QFileInfo destInfo(dest);
    QDir().mkpath(destInfo.absolutePath());

    if (QFile::exists(dest)) {
        QFile::remove(dest);
    }

    // 先尝试 rename（同盘原子操作）
    if (QFile::rename(source, dest)) {
        result["success"] = true;
        result["result"] = "文件移动成功: " + source + " → " + dest;
        return result;
    }

    // rename 失败（可能是跨盘），退化为 copy + delete
    if (!QFile::copy(source, dest)) {
        result["success"] = false;
        result["error"] = "文件移动失败（复制阶段）: " + source + " → " + dest;
        return result;
    }

    if (!QFile::remove(source)) {
        result["success"] = false;
        result["error"] = "文件移动失败（删除源文件阶段）: " + source;
        return result;
    }

    result["success"] = true;
    result["result"] = "文件移动成功（跨盘复制）: " + source + " → " + dest;
    return result;
}

/**
 * @brief 递归搜索目录下的文件
 * @param args 参数映射，必须包含"dir"，可选"pattern"（默认"*"）和"maxDepth"（默认-1=无限）
 * @return JSON 数组，每项包含 name/path/size/modified/isDir/icon
 *
 * @details
 * 使用 QDirIterator 递归遍历目录树。
 * pattern 支持通配符（如 *.txt、report*、???.pdf）。
 * maxDepth 限制递归深度，-1 表示不限制。
 */
QVariantMap FileAgent::searchFiles(const QVariantMap& args)
{
    QVariantMap result;
    QString dirPath = args.value("dir").toString();
    QString pattern = args.value("pattern", "*").toString();
    int maxDepth = args.value("maxDepth", -1).toInt();

    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "搜索目录不存在: " + dirPath;
        return result;
    }

    QJsonArray filesArray;

    // 使用 QDirIterator 进行递归遍历
    QDirIterator::IteratorFlags flags = QDirIterator::Subdirectories | QDirIterator::FollowSymlinks;
    QStringList nameFilters;
    nameFilters << pattern;

    QDirIterator it(dirPath, nameFilters, QDir::Files | QDir::NoDotAndDotDot, flags);

    while (it.hasNext()) {
        it.next();

        QFileInfo info = it.fileInfo();

        // 计算当前深度
        QString relativePath = dir.relativeFilePath(info.absolutePath());
        int depth = relativePath.isEmpty() ? 0 : relativePath.count('/') + relativePath.count('\\') + 1;

        if (maxDepth >= 0 && depth > maxDepth) {
            continue;
        }

        QJsonObject fileObj;
        fileObj["name"] = info.fileName();
        fileObj["path"] = info.absoluteFilePath();
        fileObj["size"] = info.size();
        fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
        fileObj["isDir"] = false;
        fileObj["icon"] = "file";
        filesArray.append(fileObj);
    }

    if (filesArray.isEmpty()) {
        result["success"] = true;
        result["result"] = "未找到匹配 '" + pattern + "' 的文件。";
        return result;
    }

    result["success"] = true;
    result["result"] = QJsonDocument(filesArray).toJson(QJsonDocument::Compact);
    return result;
}

QVariantMap FileAgent::renameFile(const QVariantMap& args)
{
    QVariantMap result;
    QString oldName = args.value("oldName").toString();
    QString newName = args.value("newName").toString();

    if (!QFile::exists(oldName)) {
        result["success"] = false;
        result["error"] = "源文件不存在: " + oldName;
        return result;
    }

    if (QFile::exists(newName)) {
        QFile::remove(newName);
    }

    if (!QFile::rename(oldName, newName)) {
        result["success"] = false;
        result["error"] = "重命名失败: " + oldName + " → " + newName;
        return result;
    }

    result["success"] = true;
    result["result"] = "重命名成功: " + oldName + " → " + newName;
    return result;
}

QVariantMap FileAgent::deleteDirectory(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    bool force = args.value("force", false).toBool();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + path;
        return result;
    }

    QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QString fullPath = dir.absoluteFilePath(entry);
        QFileInfo info(fullPath);

        if (info.isDir()) {
            QVariantMap subResult = deleteDirectory({{"path", fullPath}, {"force", force}});
            if (!subResult["success"].toBool()) {
                result["success"] = false;
                result["error"] = subResult["error"].toString();
                return result;
            }
        } else {
            QFile file(fullPath);
            if (force) {
                file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
            }
            if (!file.remove()) {
                result["success"] = false;
                result["error"] = "无法删除文件: " + fullPath;
                return result;
            }
        }
    }

    if (!dir.rmdir(path)) {
        result["success"] = false;
        result["error"] = "无法删除目录: " + path;
        return result;
    }

    result["success"] = true;
    result["result"] = "目录删除成功: " + path;
    return result;
}

QVariantMap FileAgent::getFileInfo(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    QJsonObject fileObj;
    fileObj["name"] = info.fileName();
    fileObj["path"] = info.absoluteFilePath();
    fileObj["size"] = info.size();
    fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
    fileObj["isDir"] = info.isDir();
    fileObj["isFile"] = info.isFile();
    fileObj["isSymLink"] = info.isSymLink();
    fileObj["owner"] = info.owner();
    fileObj["suffix"] = info.suffix();
    fileObj["baseName"] = info.baseName();
    fileObj["icon"] = info.isDir() ? "folder" : "file";

    result["success"] = true;
    result["result"] = QJsonDocument(fileObj).toJson(QJsonDocument::Compact);
    return result;
}

QVariantMap FileAgent::readFileLines(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    int startLine = args.value("startLine", 1).toInt();
    int endLine = args.value("endLine", -1).toInt();

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
    QStringList lines;
    QString line;
    int currentLine = 0;

    while (!in.atEnd()) {
        line = in.readLine();
        currentLine++;
        if (currentLine >= startLine && (endLine == -1 || currentLine <= endLine)) {
            lines.append(line);
        }
        if (endLine != -1 && currentLine > endLine) {
            break;
        }
    }

    file.close();

    result["success"] = true;
    result["result"] = lines.join("\n");
    return result;
}

QVariantMap FileAgent::batchWriteFiles(const QVariantMap& args)
{
    QVariantMap result;
    QVariantList filesList = args.value("files").toList();

    if (filesList.isEmpty()) {
        result["success"] = false;
        result["error"] = "文件列表为空";
        return result;
    }

    QStringList successList;
    QStringList errorList;

    for (const QVariant& fileVar : filesList) {
        QVariantMap fileMap = fileVar.toMap();
        QString filePath = fileMap.value("path").toString();
        QString content = fileMap.value("content").toString();

        if (filePath.isEmpty()) {
            errorList.append("文件路径为空");
            continue;
        }

        QFileInfo fileInfo(filePath);
        QString parentDir = fileInfo.path();
        if (!parentDir.isEmpty()) {
            QDir dir(parentDir);
            if (!dir.exists()) {
                if (!dir.mkpath(".")) {
                    errorList.append("无法创建目录: " + parentDir);
                    continue;
                }
            }
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            errorList.append("无法写入文件: " + filePath);
            continue;
        }

        QTextStream out(&file);
        out << content;
        file.close();
        successList.append(filePath);
    }

    QString resultText;
    if (!successList.isEmpty()) {
        resultText += "成功写入 " + QString::number(successList.size()) + " 个文件:\n";
        resultText += successList.join("\n");
    }
    if (!errorList.isEmpty()) {
        if (!resultText.isEmpty()) resultText += "\n\n";
        resultText += "失败 " + QString::number(errorList.size()) + " 个文件:\n";
        resultText += errorList.join("\n");
    }

    result["success"] = errorList.isEmpty();
    result["result"] = resultText;
    return result;
}
