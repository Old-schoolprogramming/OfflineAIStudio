// 【文件说明】searchagent.cpp - 搜索代理的实现文件
// 【作用】实现 SearchAgent 类，提供网页搜索、URL内容抓取、本地文件搜索、文本搜索和代码符号搜索功能
// 【注意事项】webSearch 和 webFetch 在离线模式下直接返回友好提示，不尝试网络连接

#include "searchagent.h"    // 引入 SearchAgent 自己的头文件，获取类声明
#include "tool.h"           // 引入 Tool 类头文件，用于创建和注册工具
#include <QNetworkReply>    // 引入 Qt 网络回复类（预留，离线模式暂未使用网络请求）
#include <QNetworkRequest>  // 引入 Qt 网络请求类（预留，离线模式暂未使用网络请求）
#include <QEventLoop>       // 引入 Qt 事件循环类（预留，用于同步网络请求时阻塞等待）
#include <QJsonArray>       // 引入 Qt JSON 数组类，用于构建搜索结果列表
#include <QJsonObject>      // 引入 Qt JSON 对象类，用于表示单个搜索结果
#include <QJsonDocument>    // 引入 Qt JSON 文档类，用于将结果序列化为 JSON 字符串
#include <QRegularExpression> // 引入 Qt 正则表达式类，用于文本搜索和代码符号匹配
#include <QUrl>             // 引入 Qt URL 类，用于处理网址编码
#include <QUrlQuery>        // 引入 Qt URL 查询类，用于构建搜索查询参数
#include <QDir>             // 引入 Qt 目录类，用于检查搜索目录是否存在
#include <QDirIterator>     // 引入 Qt 目录迭代器类，用于递归遍历目录中的文件
#include <QFile>            // 引入 Qt 文件类，用于打开和读取文件内容
#include <QFileInfo>        // 引入 Qt 文件信息类，用于获取文件名、大小、修改时间等元数据
#include <QTextStream>      // 引入 Qt 文本流类，用于读取文件文本内容

// 【功能说明】SearchAgent 构造函数，创建搜索代理实例
// 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
// 【说明】调用父类 Agent 的构造函数，并执行 initializeTools() 注册所有搜索工具
SearchAgent::SearchAgent(QObject *parent)
    : Agent(parent)  // 调用父类 Agent 的构造函数，传入父对象指针
{
    initializeTools();  // 初始化并注册所有搜索工具到 m_tools 列表
}

// 【功能说明】SearchAgent 析构函数，销毁搜索代理实例
// 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象，防止内存泄漏
SearchAgent::~SearchAgent()
{
    qDeleteAll(m_tools);  // 遍历 m_tools 列表，删除每一个 Tool 对象指针指向的内存
}

// 【功能说明】获取代理名称
// 【返回值】固定返回 QString 字符串 "SearchAgent"
// 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
QString SearchAgent::name() const
{
    return "SearchAgent";  // 返回此 Agent 的唯一标识名称
}

// 【功能说明】获取代理功能描述
// 【返回值】返回描述此 Agent 功能的 QString 字符串
// 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
QString SearchAgent::description() const
{
    return "网络搜索与内容获取代理，负责网页搜索和URL内容抓取";  // 返回中文功能描述
}

// 【功能说明】获取代理类型
// 【返回值】固定返回枚举值 SearchAgentType
// 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
AgentType SearchAgent::type() const
{
    return SearchAgentType;  // 返回搜索代理类型枚举值
}

// 【功能说明】获取当前代理注册的所有工具列表
// 【返回值】返回 QList<Tool*> 类型的工具指针列表
// 【说明】重写父类虚函数，外部系统调用此函数获取可用工具清单
QList<Tool*> SearchAgent::tools() const
{
    return m_tools;  // 返回内部存储的工具列表（m_tools 是成员变量）
}

// 【功能说明】初始化并注册所有搜索相关工具
// 【说明】在构造函数中调用一次，为每个功能创建一个 Tool 对象并添加到 m_tools 列表
void SearchAgent::initializeTools()
{
    // ========== 1. 注册 webSearch 工具 ==========
    // 【工具说明】搜索网页（离线模式不可用，会直接返回离线提示）
    // 【必填参数】query - 搜索关键词字符串
    // 【可选参数】maxResults - 最大返回结果数，默认 5
    Tool* searchTool = new Tool("webSearch", "搜索网页（离线模式不可用）",
        [this](const QVariantMap& args) { return this->webSearch(args); });
    searchTool->addParameter("query", "string", "搜索关键词", true);
    searchTool->addParameter("maxResults", "int", "最大结果数", false, "5");
    m_tools.append(searchTool);  // 将工具添加到列表

    // ========== 2. 注册 webFetch 工具 ==========
    // 【工具说明】抓取指定 URL 的网页正文内容（离线模式不可用）
    // 【必填参数】url - 目标网页的 URL 字符串
    Tool* fetchTool = new Tool("webFetch", "抓取指定URL的网页正文内容（离线模式不可用）",
        [this](const QVariantMap& args) { return this->webFetch(args); });
    fetchTool->addParameter("url", "string", "目标网页URL", true);
    m_tools.append(fetchTool);  // 将工具添加到列表

    // ========== 3. 注册 searchFiles 工具 ==========
    // 【工具说明】在指定目录中递归搜索文件，支持通配符匹配文件名
    // 【必填参数】dir - 搜索起始目录路径
    // 【可选参数】pattern - 文件名匹配模式（支持 * 和 ? 通配符），默认 *
    // 【可选参数】maxDepth - 最大递归深度，-1 表示不限深度，默认 -1
    Tool* localSearchTool = new Tool("searchFiles", "在目录中搜索文件（支持通配符）",
        [this](const QVariantMap& args) { return this->searchFiles(args); });
    localSearchTool->addParameter("dir", "string", "搜索起始目录", true);
    localSearchTool->addParameter("pattern", "string", "文件名匹配模式（支持 * 和 ?）", false, "*");
    localSearchTool->addParameter("maxDepth", "int", "最大递归深度（-1表示无限）", false, "-1");
    m_tools.append(localSearchTool);  // 将工具添加到列表

    // ========== 4. 注册 searchText 工具 ==========
    // 【工具说明】在指定目录的所有文件中搜索匹配的文本内容，支持正则表达式
    // 【必填参数】dir - 搜索起始目录路径
    // 【必填参数】pattern - 搜索关键词或正则表达式
    // 【可选参数】filePattern - 文件类型过滤（如 *.cpp, *.h），默认 *
    // 【可选参数】maxDepth - 最大递归深度，-1 表示不限深度，默认 -1
    // 【可选参数】caseSensitive - 是否区分大小写，默认 false
    Tool* textSearchTool = new Tool("searchText", "在文件中搜索文本内容",
        [this](const QVariantMap& args) { return this->searchText(args); });
    textSearchTool->addParameter("dir", "string", "搜索起始目录", true);
    textSearchTool->addParameter("pattern", "string", "搜索关键词或正则表达式", true);
    textSearchTool->addParameter("filePattern", "string", "文件类型过滤（如 *.cpp, *.h）", false, "*");
    textSearchTool->addParameter("maxDepth", "int", "最大递归深度（-1表示无限）", false, "-1");
    textSearchTool->addParameter("caseSensitive", "bool", "是否区分大小写", false, "false");
    m_tools.append(textSearchTool);  // 将工具添加到列表

    // ========== 5. 注册 searchCode 工具 ==========
    // 【工具说明】在代码文件中搜索指定的函数、类或变量定义
    // 【必填参数】dir - 搜索起始目录路径
    // 【必填参数】symbol - 要搜索的符号名称（函数名、类名、变量名）
    // 【可选参数】language - 语言过滤（cpp, python, java, javascript 等），默认空表示全部
    Tool* codeSearchTool = new Tool("searchCode", "搜索代码中的函数、类、变量等",
        [this](const QVariantMap& args) { return this->searchCode(args); });
    codeSearchTool->addParameter("dir", "string", "搜索起始目录", true);
    codeSearchTool->addParameter("symbol", "string", "要搜索的符号名称", true);
    codeSearchTool->addParameter("language", "string", "语言过滤（cpp, python, java, javascript等）", false, "");
    m_tools.append(codeSearchTool);  // 将工具添加到列表
}

// 【功能说明】工具分发执行入口，根据工具名称调用对应的私有实现方法
// 【参数说明】toolName - 要执行的工具名称字符串
// 【参数说明】args - 工具参数字典，键为 QString，值为 QVariant
// 【返回值】QVariantMap 执行结果，包含 success（bool）、result（QString）或 error（QString）
QVariantMap SearchAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    // 使用 if-else 链根据 toolName 将请求分派到对应的私有方法
    if (toolName == "webSearch") {  // 网页搜索
        return webSearch(args);
    } else if (toolName == "webFetch") {  // URL 内容抓取
        return webFetch(args);
    } else if (toolName == "searchFiles") {  // 本地文件搜索
        return searchFiles(args);
    } else if (toolName == "searchText") {  // 文本内容搜索
        return searchText(args);
    } else if (toolName == "searchCode") {  // 代码符号搜索
        return searchCode(args);
    }

    // 如果 toolName 不匹配任何已知工具，返回通用错误信息
    QVariantMap result;  // 创建结果字典
    result["success"] = false;  // 标记执行失败
    result["error"] = "Unknown tool: " + toolName;  // 返回错误提示，说明工具名称未知
    return result;  // 返回错误结果
}

// 【功能说明】网页搜索（离线模式，直接返回离线提示）
// 【参数说明】args - 参数字典，包含 query（搜索关键词）和 maxResults（最大结果数）
// 【返回值】固定返回离线模式提示，不执行实际网络请求
// 【说明】根据项目需求，离线模式下所有网络功能必须返回友好提示，不尝试下载
QVariantMap SearchAgent::webSearch(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString query = args.value("query").toString();  // 提取搜索关键词
    int maxResults = args.value("maxResults", 5).toInt();  // 提取最大结果数，默认 5
    Q_UNUSED(maxResults)  // 离线模式下不使用此参数，标记以避免编译器警告

    // 检查搜索关键词是否为空
    if (query.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "搜索关键词不能为空";  // 返回参数错误提示
        return result;  // 返回错误结果
    }

    // 离线模式：直接返回友好提示，不执行网络请求
    result["success"] = false;  // 标记失败（因为没有实际搜索结果）
    result["error"] = "当前处于离线模式，无法进行网页搜索。请连接网络后重试，或使用本地文件搜索功能。";  // 返回离线提示
    return result;  // 返回结果
}

// 【功能说明】抓取指定 URL 的网页内容（离线模式，直接返回离线提示）
// 【参数说明】args - 参数字典，包含 url（目标网页URL）
// 【返回值】固定返回离线模式提示，不执行实际网络请求
// 【说明】根据项目需求，离线模式下所有网络功能必须返回友好提示，不尝试下载
QVariantMap SearchAgent::webFetch(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString urlStr = args.value("url").toString();  // 提取目标 URL

    // 检查 URL 是否为空
    if (urlStr.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "URL 不能为空";  // 返回参数错误提示
        return result;  // 返回错误结果
    }

    // 离线模式：直接返回友好提示，不执行网络请求
    result["success"] = false;  // 标记失败（因为没有实际抓取内容）
    result["error"] = "当前处于离线模式，无法抓取网页内容。请连接网络后重试。";  // 返回离线提示
    return result;  // 返回结果
}

// 【功能说明】在指定目录中递归搜索文件，支持通配符和深度限制
// 【参数说明】args - 参数字典，包含 dir（目录）、pattern（通配符）、maxDepth（最大深度）
// 【返回值】若找到文件，result 为 JSON 数组字符串；若未找到，result 为提示文本
// 【说明】使用 QDirIterator 进行递归遍历，根据 maxDepth 限制搜索深度
QVariantMap SearchAgent::searchFiles(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString dirPath = args.value("dir").toString();  // 提取搜索起始目录路径
    QString pattern = args.value("pattern", "*").toString();  // 提取文件名匹配模式，默认 *
    int maxDepth = args.value("maxDepth", -1).toInt();  // 提取最大递归深度，默认 -1（不限）

    // 创建 QDir 对象并检查目录是否存在
    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;  // 目录不存在，标记失败
        result["error"] = "搜索目录不存在: " + dirPath;  // 返回错误提示
        return result;  // 返回错误结果
    }

    QJsonArray filesArray;  // 创建 JSON 数组，用于存储找到的文件信息对象

    // 创建目录迭代器，递归遍历所有匹配 pattern 的文件
    // QDir::Files 表示只匹配文件，QDir::NoDotAndDotDot 排除 . 和 .. 目录
    // QDirIterator::Subdirectories 表示递归子目录
    QDirIterator it(dirPath, QStringList() << pattern,
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    // 遍历迭代器找到的所有文件
    while (it.hasNext()) {
        it.next();  // 移动到下一个文件/目录项
        QFileInfo info = it.fileInfo();  // 获取当前项的文件信息

        // 计算当前文件相对于搜索根目录的深度
        // relativePath 是当前文件所在目录相对于 dirPath 的相对路径
        QString relativePath = dir.relativeFilePath(info.absolutePath());
        // 通过统计路径分隔符数量计算深度（空路径表示在根目录，深度为0）
        int depth = relativePath.isEmpty() ? 0 : relativePath.count('/') + relativePath.count('\\') + 1;

        // 如果设置了深度限制且当前深度超过限制，则跳过此文件
        if (maxDepth >= 0 && depth > maxDepth) {
            continue;  // 跳过超出深度限制的文件
        }

        // 构建 JSON 对象存储当前文件的元数据
        QJsonObject fileObj;
        fileObj["name"] = info.fileName();  // 文件名（不含路径）
        fileObj["path"] = info.absoluteFilePath();  // 绝对路径
        fileObj["size"] = info.size();  // 文件大小（字节）
        fileObj["modified"] = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");  // 最后修改时间
        fileObj["icon"] = "file";  // 图标标识（供 UI 使用）
        filesArray.append(fileObj);  // 将文件对象添加到结果数组
    }

    // 如果未找到任何匹配文件
    if (filesArray.isEmpty()) {
        result["success"] = true;  // 搜索本身成功执行，只是无结果
        result["result"] = "未找到匹配 '" + pattern + "' 的文件。";  // 返回友好提示
        return result;  // 返回结果
    }

    // 找到匹配文件，将 JSON 数组序列化为紧凑格式字符串返回
    result["success"] = true;  // 标记成功
    result["result"] = QJsonDocument(filesArray).toJson(QJsonDocument::Compact);  // 序列化为紧凑 JSON
    return result;  // 返回结果
}

// 【功能说明】在文件中搜索匹配的文本内容，支持正则表达式和上下文提取
// 【参数说明】args - 参数字典，包含 dir（目录）、pattern（正则）、filePattern（文件过滤）、maxDepth（深度）、caseSensitive（大小写敏感）
// 【返回值】若找到匹配，result 为 JSON 数组字符串；若未找到，result 为提示文本
// 【说明】逐行读取文件内容，使用 QRegularExpression 匹配，提取匹配点前后30字符作为上下文
QVariantMap SearchAgent::searchText(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString dirPath = args.value("dir").toString();  // 提取搜索起始目录路径
    QString pattern = args.value("pattern").toString();  // 提取搜索关键词或正则表达式
    QString filePattern = args.value("filePattern", "*").toString();  // 提取文件类型过滤，默认 *
    int maxDepth = args.value("maxDepth", -1).toInt();  // 提取最大递归深度，默认 -1（不限）
    bool caseSensitive = args.value("caseSensitive", false).toBool();  // 提取是否区分大小写，默认 false

    // 参数校验
    if (dirPath.isEmpty() || pattern.isEmpty()) {
        result["success"] = false;  // 参数缺失，标记失败
        result["error"] = "搜索目录和模式不能为空";  // 返回参数错误提示
        return result;  // 返回错误结果
    }

    // 创建 QDir 对象并检查目录是否存在
    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;  // 目录不存在，标记失败
        result["error"] = "搜索目录不存在: " + dirPath;  // 返回错误提示
        return result;  // 返回错误结果
    }

    // 创建正则表达式对象
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (!caseSensitive) {  // 不区分大小写
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    QRegularExpression regex(pattern, options);

    // 检查正则表达式是否有效
    if (!regex.isValid()) {
        result["success"] = false;  // 正则无效，标记失败
        result["error"] = "无效的正则表达式: " + regex.errorString();  // 返回错误提示
        return result;  // 返回错误结果
    }

    QJsonArray matchesArray;  // 创建 JSON 数组，用于存储匹配结果

    // 创建目录迭代器，递归遍历所有匹配 filePattern 的文件
    QDirIterator it(dirPath, QStringList() << filePattern,
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    // 遍历所有文件
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();

        // 计算深度
        QString relativePath = dir.relativeFilePath(info.absolutePath());
        int depth = relativePath.isEmpty() ? 0 : relativePath.count('/') + relativePath.count('\\') + 1;

        if (maxDepth >= 0 && depth > maxDepth) {
            continue;  // 跳过超出深度限制的文件
        }

        // 打开文件读取
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;  // 无法读取的文件跳过
        }

        QTextStream in(&file);  // 创建文本流
        in.setEncoding(QStringConverter::Utf8);  // 设置 UTF-8 编码
        int lineNum = 0;  // 当前行号

        // 逐行读取文件内容
        while (!in.atEnd()) {
            QString line = in.readLine();  // 读取一行
            lineNum++;  // 行号自增

            // 在当前行中查找所有匹配
            QRegularExpressionMatchIterator matchIt = regex.globalMatch(line);
            while (matchIt.hasNext()) {
                QRegularExpressionMatch match = matchIt.next();

                // 构建匹配结果对象
                QJsonObject matchObj;
                matchObj["file"] = info.absoluteFilePath();  // 文件路径
                matchObj["line"] = lineNum;  // 行号
                matchObj["column"] = match.capturedStart();  // 匹配位置
                matchObj["match"] = match.captured(0);  // 匹配文本
                // 提取匹配点前后 30 字符作为上下文
                int start = qMax(0, match.capturedStart() - 30);
                int end = qMin(line.length(), match.capturedEnd() + 30);
                matchObj["context"] = line.mid(start, end - start);  // 上下文片段
                matchesArray.append(matchObj);  // 添加到结果数组
            }
        }
        file.close();  // 关闭文件
    }

    // 如果未找到任何匹配
    if (matchesArray.isEmpty()) {
        result["success"] = true;  // 搜索本身成功执行，只是无结果
        result["result"] = "未找到匹配 '" + pattern + "' 的文本。";  // 返回友好提示
        return result;  // 返回结果
    }

    // 找到匹配，将 JSON 数组序列化为紧凑格式字符串返回
    result["success"] = true;  // 标记成功
    result["result"] = QJsonDocument(matchesArray).toJson(QJsonDocument::Compact);  // 序列化为紧凑 JSON
    return result;  // 返回结果
}

// 【功能说明】在代码中搜索函数、类、变量等符号定义
// 【参数说明】args - 参数字典，包含 dir（目录）、symbol（符号名）、language（语言过滤）
// 【返回值】JSON 格式的搜索结果
// 【说明】使用正则表达式搜索指定符号的定义位置
QVariantMap SearchAgent::searchCode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString dirPath = args.value("dir").toString();  // 提取搜索目录
    QString symbol = args.value("symbol").toString();  // 提取要搜索的符号名
    QString language = args.value("language", "").toString();  // 提取语言过滤，默认空（全部）

    // 参数校验
    if (dirPath.isEmpty() || symbol.isEmpty()) {
        result["success"] = false;  // 参数缺失
        result["error"] = "搜索目录和符号名不能为空";  // 返回错误提示
        return result;  // 返回错误结果
    }

    // 创建 QDir 对象并检查目录是否存在
    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;  // 目录不存在
        result["error"] = "搜索目录不存在: " + dirPath;  // 返回错误提示
        return result;  // 返回错误结果
    }

    // 根据语言选择文件扩展名和匹配模式
    QStringList filters;  // 文件过滤器
    QString funcPattern;  // 函数匹配模式
    QString classPattern;  // 类匹配模式

    if (language.isEmpty() || language == "cpp") {
        filters << "*.cpp" << "*.h" << "*.hpp" << "*.cc";  // C++ 文件
        funcPattern = R"(\b\w+\s+\w+\s*\([^)]*\)\s*\{?)";  // 简化函数模式
        classPattern = R"(\bclass\s+\w+)";  // 类模式
    } else if (language == "python") {
        filters << "*.py";  // Python 文件
        funcPattern = R"(\bdef\s+\w+\s*\()";  // 函数模式
        classPattern = R"(\bclass\s+\w+)";  // 类模式
    } else if (language == "javascript" || language == "js") {
        filters << "*.js" << "*.ts";  // JS/TS 文件
        funcPattern = R"(\bfunction\s+\w+\s*\()";  // 函数模式
        classPattern = R"(\bclass\s+\w+)";  // 类模式
    } else if (language == "java") {
        filters << "*.java";  // Java 文件
        funcPattern = R"(\b\w+\s+\w+\s*\([^)]*\)\s*\{?)";  // 函数模式
        classPattern = R"(\bclass\s+\w+)";  // 类模式
    } else {
        filters << "*.*";  // 全部文件
        funcPattern = R"(\b\w+\s*\([^)]*\))";
        classPattern = R"(\bclass\s+\w+)";
    }

    QJsonArray resultsArray;  // 搜索结果数组

    // 构建查找符号的正则（匹配符号作为函数名、类名等出现）
    QRegularExpression symbolRegex("\\b" + QRegularExpression::escape(symbol) + "\\b");
    QRegularExpression classRegex(classPattern);

    // 遍历目录中的文件
    QDirIterator it(dirPath, filters,
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();

        // 打开文件读取
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        int lineNum = 0;

        // 逐行扫描
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;

            // 检查是否包含目标符号
            if (symbolRegex.match(line).hasMatch()) {
                QJsonObject matchObj;
                matchObj["file"] = info.absoluteFilePath();
                matchObj["line"] = lineNum;
                matchObj["code"] = line.trimmed();
                matchObj["type"] = classRegex.match(line).hasMatch() ? "class" : "function";
                resultsArray.append(matchObj);
            }
        }
        file.close();
    }

    // 如果未找到任何匹配
    if (resultsArray.isEmpty()) {
        result["success"] = true;
        result["result"] = "未找到符号 '" + symbol + "' 的定义。";
        return result;
    }

    // 找到匹配，序列化返回
    result["success"] = true;
    result["result"] = QJsonDocument(resultsArray).toJson(QJsonDocument::Compact);
    return result;
}