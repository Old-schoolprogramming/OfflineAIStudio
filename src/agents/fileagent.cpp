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
#include <QDirIterator>
#include <QDateTime>

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

    Tool* copyFileTool = new Tool("copyFile", "复制文件或目录");
    copyFileTool->addParameter("source", "string", "源文件/目录路径", true);
    copyFileTool->addParameter("target", "string", "目标文件/目录路径", true);
    copyFileTool->addParameter("overwrite", "bool", "是否覆盖已存在的文件", false, "false");
    m_tools.append(copyFileTool);

    Tool* moveFileTool = new Tool("moveFile", "移动文件或目录");
    moveFileTool->addParameter("source", "string", "源文件/目录路径", true);
    moveFileTool->addParameter("target", "string", "目标文件/目录路径", true);
    m_tools.append(moveFileTool);

    Tool* renameFileTool = new Tool("renameFile", "重命名文件或目录");
    renameFileTool->addParameter("path", "string", "原文件/目录路径", true);
    renameFileTool->addParameter("newName", "string", "新名称", true);
    m_tools.append(renameFileTool);

    Tool* fileInfoTool = new Tool("fileInfo", "获取文件详细信息");
    fileInfoTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(fileInfoTool);

    Tool* searchFilesTool = new Tool("searchFiles", "搜索文件（按文件名模式匹配）");
    searchFilesTool->addParameter("dir", "string", "搜索起始目录", true);
    searchFilesTool->addParameter("pattern", "string", "文件名匹配模式（如*.txt）", true);
    searchFilesTool->addParameter("recursive", "bool", "是否递归搜索子目录", false, "true");
    m_tools.append(searchFilesTool);

    Tool* readFileLinesTool = new Tool("readFileLines", "读取文件指定行范围");
    readFileLinesTool->addParameter("path", "string", "文件路径", true);
    readFileLinesTool->addParameter("startLine", "int", "起始行号（从1开始）", true);
    readFileLinesTool->addParameter("endLine", "int", "结束行号", true);
    m_tools.append(readFileLinesTool);

    Tool* writeFileLinesTool = new Tool("writeFileLines", "在文件指定行插入内容");
    writeFileLinesTool->addParameter("path", "string", "文件路径", true);
    writeFileLinesTool->addParameter("startLine", "int", "插入位置行号（从1开始）", true);
    writeFileLinesTool->addParameter("content", "string", "要插入的内容", true);
    m_tools.append(writeFileLinesTool);

    Tool* directorySizeTool = new Tool("directorySize", "获取目录总大小");
    directorySizeTool->addParameter("dir", "string", "目录路径", true);
    m_tools.append(directorySizeTool);

    Tool* deleteDirectoryTool = new Tool("deleteDirectory", "删除目录（递归删除）");
    deleteDirectoryTool->addParameter("path", "string", "目录路径", true);
    m_tools.append(deleteDirectoryTool);

    Tool* compareFilesTool = new Tool("compareFiles", "比较两个文件内容");
    compareFilesTool->addParameter("file1", "string", "第一个文件路径", true);
    compareFilesTool->addParameter("file2", "string", "第二个文件路径", true);
    m_tools.append(compareFilesTool);

    Tool* fileExtensionTool = new Tool("fileExtension", "获取文件扩展名和基础名");
    fileExtensionTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(fileExtensionTool);
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
    } else if (toolName == "renameFile") {
        return renameFile(args);
    } else if (toolName == "fileInfo") {
        return fileInfo(args);
    } else if (toolName == "searchFiles") {
        return searchFiles(args);
    } else if (toolName == "readFileLines") {
        return readFileLines(args);
    } else if (toolName == "writeFileLines") {
        return writeFileLines(args);
    } else if (toolName == "directorySize") {
        return directorySize(args);
    } else if (toolName == "deleteDirectory") {
        return deleteDirectory(args);
    } else if (toolName == "compareFiles") {
        return compareFiles(args);
    } else if (toolName == "fileExtension") {
        return fileExtension(args);
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

/**
 * @brief 复制文件或目录
 * @param args 参数映射，包含source、target、overwrite
 * @return 复制结果
 *
 * @details
 * 支持文件和目录的复制。对于目录，使用QDirIterator递归复制。
 * 如果目标已存在且overwrite为false时，返回错误。
 */
QVariantMap FileAgent::copyFile(const QVariantMap& args)
{
    QVariantMap result;
    QString source = args.value("source").toString();
    QString target = args.value("target").toString();
    bool overwrite = args.value("overwrite", false).toBool();

    QFileInfo sourceInfo(source);
    if (!sourceInfo.exists()) {
        result["success"] = false;
        result["error"] = "源文件/目录不存在: " + source;
        return result;
    }

    QFileInfo targetInfo(target);
    if (targetInfo.exists() && !overwrite) {
        result["success"] = false;
        result["error"] = "目标已存在: " + target;
        return result;
    }

    if (sourceInfo.isFile()) {
        if (QFile::exists(target) && overwrite) {
            QFile::remove(target);
        }
        if (!QFile::copy(source, target)) {
            result["success"] = false;
            result["error"] = "文件复制失败";
            return result;
        }
    } else if (sourceInfo.isDir()) {
        QDir srcDir(source);
        QDir().mkpath(target);
        QDirIterator it(source, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QString srcPath = it.filePath();
            QString relPath = srcDir.relativeFilePath(srcPath);
            QString dstPath = target + "/" + relPath;
            if (it.fileInfo().isDir()) {
                QDir().mkpath(dstPath);
            } else {
                if (QFile::exists(dstPath) && overwrite) {
                    QFile::remove(dstPath);
                }
                QFile::copy(srcPath, dstPath);
            }
        }
    }

    result["success"] = true;
    result["result"] = QString("复制成功: %1 -> %2").arg(source, target);
    return result;
}

/**
 * @brief 移动文件或目录
 * @param args 参数映射，包含source、target
 * @return 移动结果
 *
 * @details
 * 使用QFile::rename或QDir::rename实现文件/目录移动。
 * 跨文件系统移动时也能正常工作（Qt会自动处理）。
 */
QVariantMap FileAgent::moveFile(const QVariantMap& args)
{
    QVariantMap result;
    QString source = args.value("source").toString();
    QString target = args.value("target").toString();

    QFileInfo sourceInfo(source);
    if (!sourceInfo.exists()) {
        result["success"] = false;
        result["error"] = "源文件/目录不存在: " + source;
        return result;
    }

    if (QFile::exists(target)) {
        result["success"] = false;
        result["error"] = "目标已存在: " + target;
        return result;
    }

    bool success = false;
    if (sourceInfo.isFile()) {
        success = QFile::rename(source, target);
    } else {
        success = QDir().rename(source, target);
    }

    if (!success) {
        result["success"] = false;
        result["error"] = "移动失败";
        return result;
    }

    result["success"] = true;
    result["result"] = QString("移动成功: %1 -> %2").arg(source, target);
    return result;
}

/**
 * @brief 重命名文件或目录
 * @param args 参数映射，包含path、newName
 * @return 重命名结果
 *
 * @details
 * 在同一目录下重命名文件或目录。newName是纯名称，不含路径。
 */
QVariantMap FileAgent::renameFile(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString newName = args.value("newName").toString();

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        result["success"] = false;
        result["error"] = "文件/目录不存在: " + path;
        return result;
    }

    QDir parentDir = fileInfo.dir();
    QString newPath = parentDir.filePath(newName);

    if (QFile::exists(newPath)) {
        result["success"] = false;
        result["error"] = "新名称已存在: " + newName;
        return result;
    }

    bool success = false;
    if (fileInfo.isFile()) {
        success = QFile::rename(path, newPath);
    } else {
        success = parentDir.rename(fileInfo.fileName(), newName);
    }

    if (!success) {
        result["success"] = false;
        result["error"] = "重命名失败";
        return result;
    }

    result["success"] = true;
    result["result"] = QString("重命名成功: %1 -> %2").arg(path, newPath);
    return result;
}

/**
 * @brief 获取文件详细信息
 * @param args 参数映射，包含path
 * @return 文件详细信息
 *
 * @details
 * 返回文件的完整信息，包括：大小、类型、创建时间、修改时间、
 * 最后访问时间、权限、所有者等。
 */
QVariantMap FileAgent::fileInfo(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    QString infoStr = QString(
        "文件路径: %1\n"
        "文件名称: %2\n"
        "文件大小: %3 字节 (%4)\n"
        "文件类型: %5\n"
        "创建时间: %6\n"
        "修改时间: %7\n"
        "访问时间: %8\n"
        "是否文件: %9\n"
        "是否目录: %10\n"
        "是否可读: %11\n"
        "是否可写: %12\n"
        "是否可执行: %13\n"
        "所有者: %14\n"
        "组: %15"
    ).arg(
        info.absoluteFilePath(),
        info.fileName(),
        QString::number(info.size()),
        QString::number(info.size() / 1024) + " KB",
        info.suffix().isEmpty() ? "无扩展名" : info.suffix(),
        info.birthTime().toString("yyyy-MM-dd hh:mm:ss"),
        info.lastModified().toString("yyyy-MM-dd hh:mm:ss"),
        info.lastRead().toString("yyyy-MM-dd hh:mm:ss"),
        info.isFile() ? "是" : "否",
        info.isDir() ? "是" : "否",
        info.isReadable() ? "是" : "否",
        info.isWritable() ? "是" : "否",
        info.isExecutable() ? "是" : "否",
        info.owner(),
        info.group()
    );

    result["success"] = true;
    result["result"] = infoStr;
    return result;
}

/**
 * @brief 搜索文件（按文件名模式匹配）
 * @param args 参数映射，包含dir、pattern、recursive
 * @return 匹配的文件列表
 *
 * @details
 * 支持通配符模式匹配（如*.txt, test*.cpp等）。
 * 可选递归搜索子目录。
 */
QVariantMap FileAgent::searchFiles(const QVariantMap& args)
{
    QVariantMap result;
    QString dirPath = args.value("dir").toString();
    QString pattern = args.value("pattern").toString();
    bool recursive = args.value("recursive", true).toBool();

    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + dirPath;
        return result;
    }

    QStringList nameFilters;
    nameFilters << pattern;

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive) {
        flags = QDirIterator::Subdirectories;
    }

    QDirIterator it(dirPath, nameFilters, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, flags);
    QStringList matches;
    while (it.hasNext()) {
        it.next();
        matches << it.filePath();
    }

    result["success"] = true;
    if (matches.isEmpty()) {
        result["result"] = "未找到匹配的文件";
    } else {
        result["result"] = QString("找到 %1 个匹配文件:\n").arg(matches.size()) + matches.join("\n");
    }
    return result;
}

/**
 * @brief 读取文件指定行范围
 * @param args 参数映射，包含path、startLine、endLine
 * @return 指定行的内容
 *
 * @details
 * 行号从1开始计数。如果endLine超出文件末尾，则读到文件末尾。
 * 如果startLine大于文件总行数，返回错误。
 */
QVariantMap FileAgent::readFileLines(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    int startLine = args.value("startLine", 1).toInt();
    int endLine = args.value("endLine", 0).toInt();

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
    int lineNum = 0;

    while (!in.atEnd()) {
        line = in.readLine();
        lineNum++;
        if (lineNum >= startLine && (endLine == 0 || lineNum <= endLine)) {
            lines << line;
        }
    }
    file.close();

    if (startLine > lineNum) {
        result["success"] = false;
        result["error"] = QString("起始行号 %1 超出文件总行数 %2").arg(startLine).arg(lineNum);
        return result;
    }

    result["success"] = true;
    result["result"] = QString("读取第 %1 到第 %2 行（共 %3 行）：\n").arg(startLine).arg(qMin(endLine == 0 ? lineNum : endLine, lineNum)).arg(lines.size()) + lines.join("\n");
    return result;
}

/**
 * @brief 在文件指定行写入内容
 * @param args 参数映射，包含path、startLine、content
 * @return 写入结果
 *
 * @details
 * 在指定行号位置插入新内容，原内容后移。
 * 行号从1开始计数。
 */
QVariantMap FileAgent::writeFileLines(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    int startLine = args.value("startLine", 1).toInt();
    QString content = args.value("content").toString();

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
    QStringList allLines;
    while (!in.atEnd()) {
        allLines << in.readLine();
    }
    file.close();

    if (startLine < 1 || startLine > allLines.size() + 1) {
        result["success"] = false;
        result["error"] = QString("行号 %1 超出范围（1-%2）").arg(startLine).arg(allLines.size() + 1);
        return result;
    }

    QStringList newContentLines = content.split("\n");
    for (int i = 0; i < newContentLines.size(); i++) {
        allLines.insert(startLine - 1 + i, newContentLines[i]);
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法写入文件: " + path;
        return result;
    }

    QTextStream out(&file);
    for (const QString& l : allLines) {
        out << l << "\n";
    }
    file.close();

    result["success"] = true;
    result["result"] = QString("在第 %1 行插入内容成功，共插入 %2 行").arg(startLine).arg(newContentLines.size());
    return result;
}

/**
 * @brief 获取目录大小
 * @param args 参数映射，包含dir
 * @return 目录总大小（字节）
 *
 * @details
 * 递归计算目录下所有文件的总大小。
 */
QVariantMap FileAgent::directorySize(const QVariantMap& args)
{
    QVariantMap result;
    QString dirPath = args.value("dir").toString();

    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + dirPath;
        return result;
    }

    qint64 totalSize = 0;
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        totalSize += it.fileInfo().size();
    }

    QString sizeStr;
    if (totalSize < 1024) {
        sizeStr = QString("%1 字节").arg(totalSize);
    } else if (totalSize < 1024 * 1024) {
        sizeStr = QString("%1 KB (%2 字节)").arg(totalSize / 1024.0, 0, 'f', 2).arg(totalSize);
    } else if (totalSize < 1024 * 1024 * 1024) {
        sizeStr = QString("%1 MB (%2 字节)").arg(totalSize / (1024.0 * 1024.0), 0, 'f', 2).arg(totalSize);
    } else {
        sizeStr = QString("%1 GB (%2 字节)").arg(totalSize / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2).arg(totalSize);
    }

    result["success"] = true;
    result["result"] = QString("目录 %1 的总大小: %2").arg(dirPath, sizeStr);
    return result;
}

/**
 * @brief 删除目录（递归删除）
 * @param args 参数映射，包含path
 * @return 删除结果
 *
 * @details
 * 使用QDir::removeRecursively递归删除目录及其所有内容。
 * 谨慎使用，删除后不可恢复！
 */
QVariantMap FileAgent::deleteDirectory(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + path;
        return result;
    }

    if (!dir.removeRecursively()) {
        result["success"] = false;
        result["error"] = "删除目录失败: " + path;
        return result;
    }

    result["success"] = true;
    result["result"] = "目录删除成功: " + path;
    return result;
}

/**
 * @brief 比较两个文件内容
 * @param args 参数映射，包含file1、file2
 * @return 比较结果
 *
 * @details
 * 逐行比较两个文件的内容，指出是否相同及差异行数。
 */
QVariantMap FileAgent::compareFiles(const QVariantMap& args)
{
    QVariantMap result;
    QString file1Path = args.value("file1").toString();
    QString file2Path = args.value("file2").toString();

    QFile file1(file1Path);
    QFile file2(file2Path);

    if (!file1.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + file1Path;
        return result;
    }
    if (!file2.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + file2Path;
        return result;
    }

    if (!file1.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + file1Path;
        return result;
    }
    if (!file2.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + file2Path;
        file1.close();
        return result;
    }

    QTextStream in1(&file1);
    QTextStream in2(&file2);
    QStringList lines1, lines2;

    while (!in1.atEnd()) lines1 << in1.readLine();
    while (!in2.atEnd()) lines2 << in2.readLine();

    file1.close();
    file2.close();

    int diffCount = 0;
    QString diffDetails;
    int maxLines = qMax(lines1.size(), lines2.size());

    for (int i = 0; i < maxLines; i++) {
        QString l1 = i < lines1.size() ? lines1[i] : "";
        QString l2 = i < lines2.size() ? lines2[i] : "";
        if (l1 != l2) {
            diffCount++;
            if (diffCount <= 10) {
                diffDetails += QString("第 %1 行不同:\n  文件1: %2\n  文件2: %3\n").arg(i + 1).arg(l1).arg(l2);
            }
        }
    }

    if (diffCount == 0) {
        result["success"] = true;
        result["result"] = "两个文件内容完全相同";
    } else {
        result["success"] = true;
        result["result"] = QString("两个文件有 %1 处不同\n\n%2").arg(diffCount).arg(diffDetails);
    }
    return result;
}

/**
 * @brief 获取文件扩展名
 * @param args 参数映射，包含path
 * @return 文件扩展名及基础名
 *
 * @details
 * 返回文件的基础名（不含扩展名）和扩展名。
 */
QVariantMap FileAgent::fileExtension(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    QString infoStr = QString(
        "完整文件名: %1\n"
        "基础名（不含扩展名）: %2\n"
        "扩展名: %3\n"
        "完整路径: %4"
    ).arg(
        info.fileName(),
        info.baseName(),
        info.suffix().isEmpty() ? "（无扩展名）" : info.suffix(),
        info.absoluteFilePath()
    );

    result["success"] = true;
    result["result"] = infoStr;
    return result;
}
