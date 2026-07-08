#include "fileagent.h"
#include "tool.h"
#include <QFile>
#include <QDir>
#include <QTextStream>

FileAgent::FileAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

FileAgent::~FileAgent()
{
    qDeleteAll(m_tools);
}

QString FileAgent::name() const
{
    return "FileAgent";
}

QString FileAgent::description() const
{
    return "文件操作代理，负责文件读写和目录管理";
}

Agent::AgentType FileAgent::type() const
{
    return FileAgentType;
}

QList<Tool*> FileAgent::tools() const
{
    return m_tools;
}

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

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

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
