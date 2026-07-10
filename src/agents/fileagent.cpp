// 【文件说明】fileagent.cpp - 文件操作代理的实现文件
// 【作用】实现 FileAgent 类中声明的所有函数，提供文件读写和目录管理功能
// 【注意事项】所有工具执行结果以 QVariantMap 形式返回，统一包含 success 和 result/error 字段

#include "fileagent.h"    // 引入 FileAgent 自己的头文件，获取类声明
#include "tool.h"         // 引入 Tool 类头文件，用于创建和注册工具
#include <QFile>          // 引入 Qt 文件类，用于打开、读取、写入文件
#include <QDir>           // 引入 Qt 目录类，用于创建和管理目录
#include <QTextStream>    // 引入 Qt 文本流类，用于方便地读写文本内容
#include <QFileInfo>      // 引入 Qt 文件信息类，用于获取文件的元数据（大小、修改时间等）
#include <QJsonArray>     // 引入 Qt JSON 数组类，用于构建 JSON 格式的返回数据
#include <QJsonObject>    // 引入 Qt JSON 对象类，用于构建 JSON 键值对
#include <QJsonDocument>  // 引入 Qt JSON 文档类，用于将 JSON 对象序列化为字符串
#include <QDirIterator>   // 引入 Qt 目录迭代器，用于递归遍历目录树
#include <QRegularExpression> // 引入 Qt 正则表达式类（本文件暂未使用，保留以备扩展）

// 【功能说明】FileAgent 构造函数，创建文件操作代理实例
// 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
// 【说明】调用父类 Agent 的构造函数，并执行 initializeTools() 注册所有工具
FileAgent::FileAgent(QObject *parent)
    : Agent(parent)  // 调用父类 Agent 的构造函数，传入父对象指针
{
    initializeTools();  // 初始化并注册所有文件操作工具到 m_tools 列表
}

// 【功能说明】FileAgent 析构函数，销毁文件操作代理实例
// 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象，防止内存泄漏
FileAgent::~FileAgent()
{
    qDeleteAll(m_tools);  // 遍历 m_tools 列表，删除每一个 Tool 对象指针指向的内存
}

// 【功能说明】获取代理名称
// 【返回值】固定返回 QString 字符串 "FileAgent"
// 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
QString FileAgent::name() const
{
    return "FileAgent";  // 返回此 Agent 的唯一标识名称
}

// 【功能说明】获取代理功能描述
// 【返回值】返回描述此 Agent 功能的 QString 字符串
// 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
QString FileAgent::description() const
{
    return "文件操作代理，负责文件读写和目录管理";  // 返回中文功能描述
}

// 【功能说明】获取代理类型
// 【返回值】固定返回枚举值 FileAgentType
// 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
AgentType FileAgent::type() const
{
    return FileAgentType;  // 返回文件代理类型枚举值
}

// 【功能说明】获取当前代理注册的所有工具列表
// 【返回值】返回 QList<Tool*> 类型的工具指针列表
// 【说明】重写父类虚函数，外部系统调用此函数获取可用工具清单
QList<Tool*> FileAgent::tools() const
{
    return m_tools;  // 返回内部存储的工具列表（m_tools 是成员变量）
}

// 【功能说明】初始化并注册所有文件操作工具
// 【说明】在构造函数中调用一次，为每个功能创建一个 Tool 对象并添加到 m_tools 列表
// 【工具清单】
//   readFile       - 读取文件内容
//   writeFile      - 写入文件内容（支持追加模式）
//   listFiles      - 列出目录内容
//   createDirectory- 创建目录（支持多级）
//   deleteFile     - 删除文件
//   fileExists     - 检查文件是否存在
//   copyFile       - 复制文件
//   moveFile       - 移动/重命名文件
//   searchFiles    - 递归搜索文件
//   renameFile     - 重命名文件或目录
//   deleteDirectory- 删除目录（递归）
//   getFileInfo    - 获取文件详细信息
//   readFileLines  - 读取指定行范围
//   batchWriteFiles- 批量写入多个文件
void FileAgent::initializeTools()
{
    // ========== 1. 注册 readFile 工具 ==========
    // 【工具说明】读取指定路径的文本文件内容
    // 【必填参数】path - 文件路径字符串
    Tool* readFileTool = new Tool("readFile", "读取文件内容",
        [this](const QVariantMap& args) { return this->readFile(args); });
    readFileTool->addParameter("path", "string", "文件路径", true);  // 添加必填参数：文件路径
    m_tools.append(readFileTool);  // 将工具添加到列表，使其可被外部调用

    // ========== 2. 注册 writeFile 工具 ==========
    // 【工具说明】将内容写入指定文件，可覆盖或追加
    // 【必填参数】path - 文件路径；content - 完整文件内容
    // 【可选参数】append - 是否追加模式，默认 false（覆盖）
    Tool* writeFileTool = new Tool("writeFile", "写入文件内容（content参数必须包含完整详细的内容，不能只写模板或大纲）",
        [this](const QVariantMap& args) { return this->writeFile(args); });
    writeFileTool->addParameter("path", "string", "文件路径", true);  // 必填：目标文件路径
    writeFileTool->addParameter("content", "string", "完整的文件内容（必须是完整详细的正文，不能只写标题大纲）", true);  // 必填：要写入的内容
    writeFileTool->addParameter("append", "bool", "是否追加模式", false, "false");  // 可选：true=追加，false=覆盖
    m_tools.append(writeFileTool);  // 添加到工具列表

    // ========== 3. 注册 listFiles 工具 ==========
    // 【工具说明】列出指定目录下的文件和子目录信息
    // 【必填参数】dir - 目录路径
    Tool* listFilesTool = new Tool("listFiles", "列出目录下的文件",
        [this](const QVariantMap& args) { return this->listFiles(args); });
    listFilesTool->addParameter("dir", "string", "目录路径", true);  // 必填：目标目录路径
    m_tools.append(listFilesTool);  // 添加到工具列表

    // ========== 4. 注册 createDirectory 工具 ==========
    // 【工具说明】创建目录，支持自动创建多级父目录
    // 【必填参数】path - 目录路径
    Tool* createDirTool = new Tool("createDirectory", "创建目录",
        [this](const QVariantMap& args) { return this->createDirectory(args); });
    createDirTool->addParameter("path", "string", "目录路径", true);  // 必填：要创建的目录路径
    m_tools.append(createDirTool);  // 添加到工具列表

    // ========== 5. 注册 deleteFile 工具 ==========
    // 【工具说明】删除单个文件（不删除目录）
    // 【必填参数】path - 文件路径
    Tool* deleteFileTool = new Tool("deleteFile", "删除文件",
        [this](const QVariantMap& args) { return this->deleteFile(args); });
    deleteFileTool->addParameter("path", "string", "文件路径", true);  // 必填：要删除的文件路径
    m_tools.append(deleteFileTool);  // 添加到工具列表

    // ========== 6. 注册 fileExists 工具 ==========
    // 【工具说明】检查指定路径的文件或目录是否存在
    // 【必填参数】path - 文件或目录路径
    Tool* fileExistsTool = new Tool("fileExists", "检查文件是否存在",
        [this](const QVariantMap& args) { return this->fileExists(args); });
    fileExistsTool->addParameter("path", "string", "文件路径", true);  // 必填：要检查的路径
    m_tools.append(fileExistsTool);  // 添加到工具列表

    // ========== 7. 注册 copyFile 工具 ==========
    // 【工具说明】将源文件复制到目标路径
    // 【必填参数】source - 源文件路径；dest - 目标路径
    Tool* copyFileTool = new Tool("copyFile", "复制文件到目标路径",
        [this](const QVariantMap& args) { return this->copyFile(args); });
    copyFileTool->addParameter("source", "string", "源文件路径", true);  // 必填：源文件路径
    copyFileTool->addParameter("dest", "string", "目标路径", true);      // 必填：目标路径
    m_tools.append(copyFileTool);  // 添加到工具列表

    // ========== 8. 注册 moveFile 工具 ==========
    // 【工具说明】移动或重命名文件（支持跨盘移动）
    // 【必填参数】source - 源文件路径；dest - 目标路径
    Tool* moveFileTool = new Tool("moveFile", "移动或重命名文件",
        [this](const QVariantMap& args) { return this->moveFile(args); });
    moveFileTool->addParameter("source", "string", "源文件路径", true);  // 必填：源文件路径
    moveFileTool->addParameter("dest", "string", "目标路径", true);      // 必填：目标路径
    m_tools.append(moveFileTool);  // 添加到工具列表

    // ========== 9. 注册 searchFiles 工具 ==========
    // 【工具说明】在目录中递归搜索匹配的文件名
    // 【必填参数】dir - 搜索起始目录
    // 【可选参数】pattern - 文件名匹配模式（支持 * 和 ? 通配符），默认 "*"
    // 【可选参数】maxDepth - 最大递归深度，-1 表示无限，默认 -1
    Tool* searchFilesTool = new Tool("searchFiles", "在目录中递归搜索匹配文件",
        [this](const QVariantMap& args) { return this->searchFiles(args); });
    searchFilesTool->addParameter("dir", "string", "搜索起始目录", true);  // 必填：搜索根目录
    searchFilesTool->addParameter("pattern", "string", "文件名匹配模式（支持通配符 * 和 ?）", false, "*");  // 可选：匹配模式
    searchFilesTool->addParameter("maxDepth", "int", "最大递归深度（-1 表示无限）", false, "-1");  // 可选：递归深度限制
    m_tools.append(searchFilesTool);  // 添加到工具列表

    // ========== 10. 注册 renameFile 工具 ==========
    // 【工具说明】重命名文件或目录
    // 【必填参数】oldName - 原路径；newName - 新路径
    Tool* renameFileTool = new Tool("renameFile", "重命名文件或目录",
        [this](const QVariantMap& args) { return this->renameFile(args); });
    renameFileTool->addParameter("oldName", "string", "原文件路径", true);  // 必填：原路径
    renameFileTool->addParameter("newName", "string", "新文件路径", true);  // 必填：新路径
    m_tools.append(renameFileTool);  // 添加到工具列表

    // ========== 11. 注册 deleteDirectory 工具 ==========
    // 【工具说明】递归删除目录及其内部所有内容
    // 【必填参数】path - 目录路径
    // 【可选参数】force - 是否强制删除（包含只读文件），默认 false
    Tool* deleteDirectoryTool = new Tool("deleteDirectory", "删除目录（递归删除所有内容）",
        [this](const QVariantMap& args) { return this->deleteDirectory(args); });
    deleteDirectoryTool->addParameter("path", "string", "目录路径", true);  // 必填：要删除的目录路径
    deleteDirectoryTool->addParameter("force", "bool", "是否强制删除（包含只读文件）", false, "false");  // 可选：强制删除开关
    m_tools.append(deleteDirectoryTool);  // 添加到工具列表

    // ========== 12. 注册 getFileInfo 工具 ==========
    // 【工具说明】获取文件的详细信息（大小、修改时间、类型等）
    // 【必填参数】path - 文件路径
    Tool* getFileInfoTool = new Tool("getFileInfo", "获取文件详细信息",
        [this](const QVariantMap& args) { return this->getFileInfo(args); });
    getFileInfoTool->addParameter("path", "string", "文件路径", true);  // 必填：目标文件路径
    m_tools.append(getFileInfoTool);  // 添加到工具列表

    // ========== 13. 注册 readFileLines 工具 ==========
    // 【工具说明】读取文件指定行号范围的内容
    // 【必填参数】path - 文件路径
    // 【可选参数】startLine - 起始行号（从1开始），默认 1
    // 【可选参数】endLine - 结束行号，-1 表示到文件末尾，默认 -1
    Tool* readFileLinesTool = new Tool("readFileLines", "读取文件指定行范围",
        [this](const QVariantMap& args) { return this->readFileLines(args); });
    readFileLinesTool->addParameter("path", "string", "文件路径", true);  // 必填：目标文件路径
    readFileLinesTool->addParameter("startLine", "int", "起始行（从1开始）", false, "1");  // 可选：起始行
    readFileLinesTool->addParameter("endLine", "int", "结束行（-1表示到末尾）", false, "-1");  // 可选：结束行
    m_tools.append(readFileLinesTool);  // 添加到工具列表

    // ========== 14. 注册 batchWriteFiles 工具 ==========
    // 【工具说明】批量写入多个文件，适合一次创建多个文件
    // 【必填参数】files - 文件数组，每项包含 path 和 content
    Tool* batchWriteFilesTool = new Tool("batchWriteFiles", "批量写入多个文件",
        [this](const QVariantMap& args) { return this->batchWriteFiles(args); });
    batchWriteFilesTool->addParameter("files", "array", "文件数组，每项包含path和content", true);  // 必填：文件对象数组
    m_tools.append(batchWriteFilesTool);  // 添加到工具列表
}

// 【功能说明】工具分发执行入口，根据工具名称调用对应的私有实现方法
// 【参数说明】toolName - 要执行的工具名称字符串（如 "readFile"）
// 【参数说明】args - 工具参数字典，键为 QString，值为 QVariant（支持多种数据类型）
// 【返回值】QVariantMap 执行结果，包含 success（bool）、result（QString）或 error（QString）
// 【说明】这是外部系统调用 FileAgent 功能的统一入口，类似路由器的分发功能
QVariantMap FileAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    // 使用 if-else 链根据 toolName 将请求分派到对应的私有方法
    if (toolName == "readFile") {  // 如果工具名是 readFile
        return readFile(args);     // 调用 readFile 方法读取文件
    } else if (toolName == "writeFile") {  // 如果工具名是 writeFile
        return writeFile(args);    // 调用 writeFile 方法写入文件
    } else if (toolName == "listFiles") {  // 如果工具名是 listFiles
        return listFiles(args);    // 调用 listFiles 方法列出目录
    } else if (toolName == "createDirectory") {  // 如果工具名是 createDirectory
        return createDirectory(args);  // 调用 createDirectory 方法创建目录
    } else if (toolName == "deleteFile") {  // 如果工具名是 deleteFile
        return deleteFile(args);   // 调用 deleteFile 方法删除文件
    } else if (toolName == "fileExists") {  // 如果工具名是 fileExists
        return fileExists(args);   // 调用 fileExists 方法检查存在性
    } else if (toolName == "copyFile") {  // 如果工具名是 copyFile
        return copyFile(args);     // 调用 copyFile 方法复制文件
    } else if (toolName == "moveFile") {  // 如果工具名是 moveFile
        return moveFile(args);     // 调用 moveFile 方法移动文件
    } else if (toolName == "searchFiles") {  // 如果工具名是 searchFiles
        return searchFiles(args);  // 调用 searchFiles 方法搜索文件
    } else if (toolName == "renameFile") {  // 如果工具名是 renameFile
        return renameFile(args);   // 调用 renameFile 方法重命名
    } else if (toolName == "deleteDirectory") {  // 如果工具名是 deleteDirectory
        return deleteDirectory(args);  // 调用 deleteDirectory 方法删除目录
    } else if (toolName == "getFileInfo") {  // 如果工具名是 getFileInfo
        return getFileInfo(args);  // 调用 getFileInfo 方法获取信息
    } else if (toolName == "readFileLines") {  // 如果工具名是 readFileLines
        return readFileLines(args);  // 调用 readFileLines 方法读取指定行
    } else if (toolName == "batchWriteFiles") {  // 如果工具名是 batchWriteFiles
        return batchWriteFiles(args);  // 调用 batchWriteFiles 方法批量写入
    }

    // 如果 toolName 不匹配任何已知工具，返回通用错误信息
    QVariantMap result;  // 创建结果字典
    result["success"] = false;  // 标记执行失败
    result["error"] = "Unknown tool: " + toolName;  // 返回错误提示，说明工具名称未知
    return result;  // 返回错误结果
}

// 【功能说明】读取指定路径的文本文件内容
// 【参数说明】args - 参数字典，必须包含 "path" 键（文件路径字符串）
// 【返回值】若成功，result 字段为文件内容字符串；若失败，error 字段包含原因
// 【执行流程】
//   1. 从 args 中提取 path 参数
//   2. 检查文件是否存在
//   3. 以只读文本模式打开文件
//   4. 使用 QTextStream 读取全部内容
//   5. 关闭文件并返回结果
QVariantMap FileAgent::readFile(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典，用于存储执行结果
    QString path = args.value("path").toString();  // 从参数中提取 "path"，转为 QString 类型

    QFile file(path);  // 创建 QFile 对象，关联到指定路径
    if (!file.exists()) {  // 检查文件是否存在，若不存在
        result["success"] = false;  // 标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 立即返回错误结果
    }

    // 以只读文本模式打开文件（ReadOnly | Text）
    // Text 模式会自动处理换行符（Windows 下 \r\n 转为 \n）
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;  // 标记失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息（可能是权限不足）
        return result;  // 立即返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流，绑定到已打开的文件
    QString content = in.readAll();  // 读取文件的全部内容到 content 字符串
    file.close();  // 关闭文件，释放系统资源

    result["success"] = true;  // 标记成功
    result["result"] = content;  // 将文件内容放入 result 字段
    return result;  // 返回成功结果
}

// 【功能说明】将内容写入指定文件，支持覆盖或追加模式
// 【参数说明】args - 参数字典，必须包含 "path"（文件路径）和 "content"（内容）
// 【参数说明】可选 "append"（bool），true 为追加，false 为覆盖（默认 false）
// 【返回值】若成功，result 字段包含成功提示；若失败，error 字段包含原因
// 【执行流程】
//   1. 提取 path、content 和 append 参数
//   2. 如果父目录不存在，自动创建
//   3. 根据 append 决定打开模式
//   4. 打开文件并写入内容
//   5. 关闭文件并返回结果
QVariantMap FileAgent::writeFile(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径参数
    QString content = args.value("content").toString();  // 提取要写入的内容参数
    bool append = args.value("append", false).toBool();  // 提取追加模式参数，默认 false

    QFileInfo fileInfo(path);  // 创建 QFileInfo 对象，获取文件路径相关信息
    QString parentDir = fileInfo.path();  // 获取文件所在父目录的路径（如 "/a/b/c.txt" 返回 "/a/b"）

    // 如果父目录路径不为空，检查并创建父目录
    if (!parentDir.isEmpty()) {
        QDir dir(parentDir);  // 创建 QDir 对象表示父目录
        if (!dir.exists()) {  // 如果父目录不存在
            if (!dir.mkpath(".")) {  // 尝试创建目录（mkpath(".") 创建当前目录及其所有不存在的父目录）
                result["success"] = false;  // 创建失败，标记失败
                result["error"] = "无法创建目录: " + parentDir;  // 设置错误信息
                return result;  // 返回错误结果
            }
        }
    }

    QFile file(path);  // 创建 QFile 对象，关联到目标路径
    // 根据 append 参数选择打开模式
    // 追加模式：WriteOnly | Append | Text
    // 覆盖模式：WriteOnly | Text
    QIODevice::OpenMode mode = append ?
        (QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text) :
        (QIODevice::WriteOnly | QIODevice::Text);

    if (!file.open(mode)) {  // 尝试以指定模式打开文件
        result["success"] = false;  // 打开失败，标记失败
        result["error"] = "无法打开文件进行写入: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream out(&file);  // 创建文本输出流，绑定到已打开的文件
    out << content;  // 将 content 字符串写入文件
    file.close();  // 关闭文件，确保内容刷新到磁盘

    result["success"] = true;  // 标记成功
    result["result"] = "文件写入成功: " + path;  // 设置成功提示信息
    return result;  // 返回成功结果
}

// 【功能说明】列出指定目录下的文件和子目录信息
// 【参数说明】args - 参数字典，必须包含 "dir" 键（目录路径字符串）
// 【返回值】若成功，result 字段为 JSON 数组字符串；若失败，error 字段包含原因
// 【说明】返回的信息包括文件名、绝对路径、大小、修改时间、是否为目录、图标类型
QVariantMap FileAgent::listFiles(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString dirPath = args.value("dir").toString();  // 提取目录路径参数

    QDir dir(dirPath);  // 创建 QDir 对象，表示目标目录
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "目录不存在: " + dirPath;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QJsonArray filesArray;  // 创建 JSON 数组，用于存储目录中每个文件/子目录的信息
    // 获取目录下所有文件和子目录的详细信息（排除 . 和 ..）
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    // 遍历目录中的每一个文件/子目录
    for (const QFileInfo& info : fileInfoList) {
        QJsonObject fileObj;  // 为每个条目创建 JSON 对象
        fileObj["name"] = info.fileName();  // 设置文件名（不含路径）
        fileObj["path"] = info.absoluteFilePath();  // 设置绝对路径
        fileObj["size"] = info.size();  // 设置文件大小（字节）
        fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");  // 设置最后修改时间，格式化为可读字符串
        fileObj["isDir"] = info.isDir();  // 设置是否为目录
        fileObj["icon"] = info.isDir() ? "folder" : "file";  // 设置图标类型：目录用 folder，文件用 file
        filesArray.append(fileObj);  // 将 JSON 对象添加到数组
    }

    result["success"] = true;  // 标记成功
    // 将 JSON 数组序列化为紧凑格式的字符串（不含多余空格和换行）
    result["result"] = QJsonDocument(filesArray).toJson(QJsonDocument::Compact);
    return result;  // 返回成功结果
}

// 【功能说明】创建目录，支持自动创建多级父目录
// 【参数说明】args - 参数字典，必须包含 "path" 键（目录路径字符串）
// 【返回值】若成功，result 字段包含成功提示；若失败，error 字段包含原因
// 【说明】使用 QDir::mkpath 实现，可一次性创建深层目录结构（如 "/a/b/c"）
QVariantMap FileAgent::createDirectory(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取目录路径参数

    QDir dir;  // 创建空的 QDir 对象（不关联具体路径）
    // 调用 mkpath 创建目录及其所有不存在的父目录
    if (!dir.mkpath(path)) {  // 如果创建失败（可能是权限不足或路径非法）
        result["success"] = false;  // 标记失败
        result["error"] = "无法创建目录: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "目录创建成功: " + path;  // 设置成功提示
    return result;  // 返回成功结果
}

// 【功能说明】删除单个文件（不删除目录）
// 【参数说明】args - 参数字典，必须包含 "path" 键（文件路径字符串）
// 【返回值】若成功，result 字段包含删除成功提示；若失败，error 字段包含原因
// 【执行流程】
//   1. 检查文件是否存在
//   2. 调用 QFile::remove 删除文件
//   3. 返回操作结果
QVariantMap FileAgent::deleteFile(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径参数

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.remove()) {  // 尝试删除文件，若失败（可能是权限不足或文件被占用）
        result["success"] = false;  // 标记失败
        result["error"] = "无法删除文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "文件删除成功: " + path;  // 设置成功提示
    return result;  // 返回成功结果
}

// 【功能说明】检查指定路径是否存在，并返回文件/目录的详细信息
// 【参数说明】args - 参数字典，必须包含 "path" 键（路径字符串）
// 【返回值】固定 success=true，result 字段为 JSON 字符串，包含 exists（是否存在）及其他信息
// 【说明】使用 QFileInfo 检查路径，不区分文件和目录
QVariantMap FileAgent::fileExists(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取路径参数

    QFileInfo fileInfo(path);  // 创建 QFileInfo 对象，获取路径信息
    bool exists = fileInfo.exists();  // 检查路径是否存在（true=存在，false=不存在）

    QJsonObject fileObj;  // 创建 JSON 对象存储结果
    fileObj["path"] = path;  // 记录查询的路径
    fileObj["exists"] = exists;  // 记录是否存在
    if (exists) {  // 如果路径存在，额外返回详细信息
        fileObj["name"] = fileInfo.fileName();  // 文件名或目录名
        fileObj["size"] = fileInfo.size();  // 大小（字节），目录通常返回 0
        fileObj["modified"] = fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss");  // 最后修改时间
        fileObj["isDir"] = fileInfo.isDir();  // 是否为目录
        fileObj["icon"] = fileInfo.isDir() ? "folder" : "file";  // 图标类型
    }

    result["success"] = true;  // 此工具始终返回成功（查询操作本身不会失败）
    // 将 JSON 对象序列化为紧凑格式字符串
    result["result"] = QJsonDocument(fileObj).toJson(QJsonDocument::Compact);
    return result;  // 返回结果
}

// 【功能说明】复制文件到目标路径
// 【参数说明】args - 参数字典，必须包含 "source"（源文件路径）和 "dest"（目标路径）
// 【返回值】若成功，result 字段包含复制成功提示；若失败，error 字段包含原因
// 【说明】如果目标路径的父目录不存在会自动创建；如果目标已存在会先删除再复制
QVariantMap FileAgent::copyFile(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString source = args.value("source").toString();  // 提取源文件路径
    QString dest = args.value("dest").toString();  // 提取目标路径

    if (!QFile::exists(source)) {  // 检查源文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "源文件不存在: " + source;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 确保目标目录存在：获取目标路径的父目录并创建
    QFileInfo destInfo(dest);  // 创建 QFileInfo 获取目标路径信息
    QDir().mkpath(destInfo.absolutePath());  // 创建目标路径的父目录（如果不存在）

    // 如果目标文件已存在，先删除（QFile::copy 不能直接覆盖）
    if (QFile::exists(dest)) {
        QFile::remove(dest);  // 删除已存在的目标文件
    }

    // 执行复制操作：将 source 复制到 dest
    if (!QFile::copy(source, dest)) {  // 如果复制失败
        result["success"] = false;  // 标记失败
        result["error"] = "文件复制失败: " + source + " → " + dest;  // 设置错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "文件复制成功: " + source + " → " + dest;  // 设置成功提示
    return result;  // 返回成功结果
}

// 【功能说明】移动或重命名文件
// 【参数说明】args - 参数字典，必须包含 "source"（源文件路径）和 "dest"（目标路径）
// 【返回值】若成功，result 字段包含移动成功提示；若失败，error 字段包含原因
// 【说明】优先使用 QFile::rename（同盘原子操作，高效），失败时退化为 copy + delete 实现跨盘移动
QVariantMap FileAgent::moveFile(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString source = args.value("source").toString();  // 提取源文件路径
    QString dest = args.value("dest").toString();  // 提取目标路径

    if (!QFile::exists(source)) {  // 检查源文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "源文件不存在: " + source;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 确保目标目录存在
    QFileInfo destInfo(dest);  // 获取目标路径信息
    QDir().mkpath(destInfo.absolutePath());  // 创建目标父目录（如果不存在）

    // 如果目标已存在，先删除（避免 rename 失败）
    if (QFile::exists(dest)) {
        QFile::remove(dest);  // 删除已存在的目标文件
    }

    // 第一步：尝试使用 rename（同盘移动，速度快，是原子操作）
    if (QFile::rename(source, dest)) {  // 如果 rename 成功
        result["success"] = true;  // 标记成功
        result["result"] = "文件移动成功: " + source + " → " + dest;  // 设置成功提示
        return result;  // 返回成功结果
    }

    // 第二步：rename 失败（通常是跨盘移动），退化为 copy + delete 方式
    if (!QFile::copy(source, dest)) {  // 先尝试复制文件
        result["success"] = false;  // 复制失败，标记失败
        result["error"] = "文件移动失败（复制阶段）: " + source + " → " + dest;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!QFile::remove(source)) {  // 复制成功后，删除源文件
        result["success"] = false;  // 删除源文件失败，标记失败
        result["error"] = "文件移动失败（删除源文件阶段）: " + source;  // 设置错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "文件移动成功（跨盘复制）: " + source + " → " + dest;  // 设置成功提示
    return result;  // 返回成功结果
}

// 【功能说明】递归搜索目录下匹配指定模式的文件
// 【参数说明】args - 参数字典，必须包含 "dir"（搜索目录）
// 【参数说明】可选 "pattern"（文件名匹配模式，默认 "*"），可选 "maxDepth"（最大深度，默认 -1=无限）
// 【返回值】JSON 数组字符串，每项包含 name/path/size/modified/isDir/icon
// 【说明】使用 QDirIterator 进行递归遍历，pattern 支持通配符（如 *.txt、report*）
QVariantMap FileAgent::searchFiles(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString dirPath = args.value("dir").toString();  // 提取搜索目录路径
    QString pattern = args.value("pattern", "*").toString();  // 提取匹配模式，默认 "*"（匹配所有）
    int maxDepth = args.value("maxDepth", -1).toInt();  // 提取最大递归深度，默认 -1 表示不限制

    QDir dir(dirPath);  // 创建 QDir 对象
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "搜索目录不存在: " + dirPath;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QJsonArray filesArray;  // 创建 JSON 数组存储搜索结果

    // 设置 QDirIterator 的遍历标志：递归子目录 | 跟随符号链接
    QDirIterator::IteratorFlags flags = QDirIterator::Subdirectories | QDirIterator::FollowSymlinks;
    QStringList nameFilters;  // 创建文件名过滤器列表
    nameFilters << pattern;  // 将匹配模式加入过滤器

    // 创建目录迭代器：从 dirPath 开始，使用 nameFilters 过滤，只遍历文件，排除 . 和 ..
    QDirIterator it(dirPath, nameFilters, QDir::Files | QDir::NoDotAndDotDot, flags);

    // 开始遍历目录树
    while (it.hasNext()) {  // 如果还有下一个文件
        it.next();  // 移动到下一个文件

        QFileInfo info = it.fileInfo();  // 获取当前文件的信息

        // 计算当前文件相对于搜索根目录的深度
        QString relativePath = dir.relativeFilePath(info.absolutePath());
        // 通过统计路径分隔符数量计算深度：/ 和 \ 都算作层级分隔
        int depth = relativePath.isEmpty() ? 0 : relativePath.count('/') + relativePath.count('\\') + 1;

        // 如果设置了最大深度且当前深度超过限制，则跳过此文件
        if (maxDepth >= 0 && depth > maxDepth) {
            continue;  // 跳过当前循环，继续下一个文件
        }

        // 构建 JSON 对象记录文件信息
        QJsonObject fileObj;
        fileObj["name"] = info.fileName();  // 文件名
        fileObj["path"] = info.absoluteFilePath();  // 绝对路径
        fileObj["size"] = info.size();  // 文件大小
        fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");  // 修改时间
        fileObj["isDir"] = false;  // 搜索结果只包含文件，所以固定为 false
        fileObj["icon"] = "file";  // 图标类型固定为 file
        filesArray.append(fileObj);  // 添加到结果数组
    }

    // 如果搜索结果为空
    if (filesArray.isEmpty()) {
        result["success"] = true;  // 查询本身成功，只是没找到文件
        result["result"] = "未找到匹配 '" + pattern + "' 的文件。";  // 返回友好提示
        return result;  // 返回结果
    }

    result["success"] = true;  // 标记成功
    // 将结果数组序列化为紧凑 JSON 字符串
    result["result"] = QJsonDocument(filesArray).toJson(QJsonDocument::Compact);
    return result;  // 返回成功结果
}

// 【功能说明】重命名文件或目录
// 【参数说明】args - 参数字典，必须包含 "oldName"（原路径）和 "newName"（新路径）
// 【返回值】若成功，result 字段包含重命名成功提示；若失败，error 字段包含原因
// 【说明】如果新路径已存在，会先删除已存在的文件/目录
QVariantMap FileAgent::renameFile(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString oldName = args.value("oldName").toString();  // 提取原文件/目录路径
    QString newName = args.value("newName").toString();  // 提取新文件/目录路径

    if (!QFile::exists(oldName)) {  // 检查原路径是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "源文件不存在: " + oldName;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 如果新路径已存在，先删除（避免 rename 失败）
    if (QFile::exists(newName)) {
        QFile::remove(newName);  // 删除已存在的目标
    }

    // 执行重命名操作
    if (!QFile::rename(oldName, newName)) {  // 如果重命名失败
        result["success"] = false;  // 标记失败
        result["error"] = "重命名失败: " + oldName + " → " + newName;  // 设置错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "重命名成功: " + oldName + " → " + newName;  // 设置成功提示
    return result;  // 返回成功结果
}

// 【功能说明】递归删除目录及其内部所有内容
// 【参数说明】args - 参数字典，必须包含 "path"（目录路径），可选 "force"（是否强制删除，默认 false）
// 【返回值】若成功，result 字段包含删除成功提示；若失败，error 字段包含原因
// 【说明】递归遍历目录内容，先删除内部文件和子目录，最后删除空目录本身
QVariantMap FileAgent::deleteDirectory(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取目录路径
    bool force = args.value("force", false).toBool();  // 提取强制删除参数，默认 false

    QDir dir(path);  // 创建 QDir 对象
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "目录不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 获取目录下所有文件和子目录（排除 . 和 ..）
    QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    // 遍历目录中的每一个条目
    for (const QString& entry : entries) {
        QString fullPath = dir.absoluteFilePath(entry);  // 获取条目的绝对路径
        QFileInfo info(fullPath);  // 获取条目信息

        if (info.isDir()) {  // 如果条目是子目录
            // 递归调用 deleteDirectory 删除子目录
            QVariantMap subResult = deleteDirectory({{"path", fullPath}, {"force", force}});
            if (!subResult["success"].toBool()) {  // 如果子目录删除失败
                result["success"] = false;  // 标记失败
                result["error"] = subResult["error"].toString();  // 传递子目录的错误信息
                return result;  // 返回错误结果，停止继续删除
            }
        } else {  // 如果条目是文件
            QFile file(fullPath);  // 创建 QFile 对象
            if (force) {  // 如果开启了强制删除模式
                // 修改文件权限为读写，确保可以删除只读文件
                file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
            }
            if (!file.remove()) {  // 尝试删除文件
                result["success"] = false;  // 删除失败，标记失败
                result["error"] = "无法删除文件: " + fullPath;  // 设置错误信息
                return result;  // 返回错误结果
            }
        }
    }

    // 删除完内部所有内容后，删除空目录本身
    if (!dir.rmdir(path)) {  // 尝试删除目录
        result["success"] = false;  // 删除失败，标记失败
        result["error"] = "无法删除目录: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "目录删除成功: " + path;  // 设置成功提示
    return result;  // 返回成功结果
}

// 【功能说明】获取文件的详细信息
// 【参数说明】args - 参数字典，必须包含 "path" 键（文件路径字符串）
// 【返回值】JSON 字符串，包含文件名、路径、大小、修改时间、类型、所有者、后缀等信息
// 【说明】使用 QFileInfo 获取全面的文件元数据
QVariantMap FileAgent::getFileInfo(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径

    QFileInfo info(path);  // 创建 QFileInfo 对象获取文件信息
    if (!info.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QJsonObject fileObj;  // 创建 JSON 对象存储文件信息
    fileObj["name"] = info.fileName();  // 文件名（不含路径）
    fileObj["path"] = info.absoluteFilePath();  // 绝对路径
    fileObj["size"] = info.size();  // 文件大小（字节）
    fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");  // 最后修改时间
    fileObj["isDir"] = info.isDir();  // 是否为目录
    fileObj["isFile"] = info.isFile();  // 是否为普通文件
    fileObj["isSymLink"] = info.isSymLink();  // 是否为符号链接（快捷方式）
    fileObj["owner"] = info.owner();  // 文件所有者
    fileObj["suffix"] = info.suffix();  // 文件后缀（如 txt、cpp）
    fileObj["baseName"] = info.baseName();  // 不含后缀的文件名
    fileObj["icon"] = info.isDir() ? "folder" : "file";  // 图标类型

    result["success"] = true;  // 标记成功
    // 将 JSON 对象序列化为紧凑格式字符串
    result["result"] = QJsonDocument(fileObj).toJson(QJsonDocument::Compact);
    return result;  // 返回成功结果
}

// 【功能说明】读取文件指定行号范围的内容
// 【参数说明】args - 参数字典，必须包含 "path"（文件路径）
// 【参数说明】可选 "startLine"（起始行，默认 1），可选 "endLine"（结束行，默认 -1=到末尾）
// 【返回值】若成功，result 字段为指定行的内容字符串；若失败，error 字段包含原因
// 【说明】适用于查看大文件的局部内容，逐行读取避免一次性加载过多内容
QVariantMap FileAgent::readFileLines(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径
    int startLine = args.value("startLine", 1).toInt();  // 提取起始行，默认第 1 行
    int endLine = args.value("endLine", -1).toInt();  // 提取结束行，默认 -1 表示到文件末尾

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 以只读文本模式打开文件
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;  // 打开失败，标记失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流
    QStringList lines;  // 创建字符串列表，存储符合条件的行
    QString line;  // 临时变量，存储当前读取的行
    int currentLine = 0;  // 当前行号计数器，从 0 开始（实际文件行号从 1 开始）

    // 逐行读取文件内容
    while (!in.atEnd()) {  // 如果还未读到文件末尾
        line = in.readLine();  // 读取一行内容
        currentLine++;  // 行号加 1
        // 判断当前行是否在指定范围内
        // 条件：currentLine >= startLine 且（endLine == -1 或 currentLine <= endLine）
        if (currentLine >= startLine && (endLine == -1 || currentLine <= endLine)) {
            lines.append(line);  // 将符合条件的行加入列表
        }
        // 如果已读到结束行之后，提前退出循环，提高大文件读取效率
        if (endLine != -1 && currentLine > endLine) {
            break;  // 跳出循环
        }
    }

    file.close();  // 关闭文件

    result["success"] = true;  // 标记成功
    result["result"] = lines.join("\n");  // 用换行符连接所有行，形成完整文本
    return result;  // 返回成功结果
}

// 【功能说明】批量写入多个文件
// 【参数说明】args - 参数字典，必须包含 "files" 数组参数
// 【参数说明】files 数组中每项是 QVariantMap，包含 "path"（文件路径）和 "content"（文件内容）
// 【返回值】result 字段包含批量写入的汇总信息，列出成功和失败的文件
// 【说明】即使部分文件失败，也会继续处理其他文件，最后统一返回结果
QVariantMap FileAgent::batchWriteFiles(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QVariantList filesList = args.value("files").toList();  // 提取文件列表参数

    // 如果文件列表为空，直接返回错误
    if (filesList.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "文件列表为空";  // 设置错误信息
        return result;  // 返回错误结果
    }

    QStringList successList;  // 创建列表，记录成功写入的文件路径
    QStringList errorList;    // 创建列表，记录失败的错误信息

    // 遍历文件列表，逐个处理
    for (const QVariant& fileVar : filesList) {
        QVariantMap fileMap = fileVar.toMap();  // 将 QVariant 转为 QVariantMap
        QString filePath = fileMap.value("path").toString();  // 提取当前文件的路径
        QString content = fileMap.value("content").toString();  // 提取当前文件的内容

        // 检查文件路径是否为空
        if (filePath.isEmpty()) {
            errorList.append("文件路径为空");  // 记录错误
            continue;  // 跳过当前文件，继续处理下一个
        }

        // 检查并创建父目录
        QFileInfo fileInfo(filePath);  // 获取文件路径信息
        QString parentDir = fileInfo.path();  // 获取父目录路径
        if (!parentDir.isEmpty()) {  // 如果父目录路径不为空
            QDir dir(parentDir);  // 创建 QDir 对象
            if (!dir.exists()) {  // 如果父目录不存在
                if (!dir.mkpath(".")) {  // 尝试创建父目录
                    errorList.append("无法创建目录: " + parentDir);  // 记录错误
                    continue;  // 跳过当前文件
                }
            }
        }

        // 打开文件并写入内容
        QFile file(filePath);  // 创建 QFile 对象
        // 以只写文本模式打开（覆盖模式，如果文件已存在则清空）
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            errorList.append("无法写入文件: " + filePath);  // 记录错误
            continue;  // 跳过当前文件
        }

        QTextStream out(&file);  // 创建文本输出流
        out << content;  // 写入内容
        file.close();  // 关闭文件
        successList.append(filePath);  // 记录成功写入的文件路径
    }

    // 构建汇总结果文本
    QString resultText;  // 创建结果文本字符串
    if (!successList.isEmpty()) {  // 如果有成功写入的文件
        resultText += "成功写入 " + QString::number(successList.size()) + " 个文件:\n";  // 添加成功数量
        resultText += successList.join("\n");  // 列出所有成功文件路径
    }
    if (!errorList.isEmpty()) {  // 如果有失败的文件
        if (!resultText.isEmpty()) resultText += "\n\n";  // 如果前面有成功信息，添加空行分隔
        resultText += "失败 " + QString::number(errorList.size()) + " 个文件:\n";  // 添加失败数量
        resultText += errorList.join("\n");  // 列出所有错误信息
    }

    // 如果没有任何错误，标记整体成功；否则标记失败
    result["success"] = errorList.isEmpty();
    result["result"] = resultText;  // 将汇总文本放入结果
    return result;  // 返回结果
}
