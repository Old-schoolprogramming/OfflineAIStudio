/**
 * @file searchagent.cpp
 * @brief 网络搜索与内容获取 Agent 实现
 *
 * SearchAgent 提供 webSearch 和 webFetch 两个工具。
 * webSearch 使用 DuckDuckGo HTML 搜索端点（无需 API Key），
 * 解析返回的 HTML 提取标题/链接/摘要。
 * webFetch 使用 QNetworkAccessManager 同步抓取目标 URL，
 * 提取正文文本并截断到合理长度。
 */

#include "searchagent.h"
#include "tool.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

SearchAgent::SearchAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

SearchAgent::~SearchAgent()
{
    qDeleteAll(m_tools);
}

QString SearchAgent::name() const { return "SearchAgent"; }
QString SearchAgent::description() const { return "网络搜索与内容获取代理，负责网页搜索和URL内容抓取"; }
Agent::AgentType SearchAgent::type() const { return SearchAgentType; }
QList<Tool*> SearchAgent::tools() const { return m_tools; }

void SearchAgent::initializeTools()
{
    Tool* searchTool = new Tool("webSearch", "搜索网页（离线模式不可用）");
    searchTool->addParameter("query", "string", "搜索关键词", true);
    searchTool->addParameter("maxResults", "int", "最大结果数", false, "5");
    m_tools.append(searchTool);

    Tool* fetchTool = new Tool("webFetch", "抓取指定URL的网页正文内容（离线模式不可用）");
    fetchTool->addParameter("url", "string", "目标网页URL", true);
    m_tools.append(fetchTool);

    Tool* localSearchTool = new Tool("searchFiles", "在目录中搜索文件（支持通配符）");
    localSearchTool->addParameter("dir", "string", "搜索起始目录", true);
    localSearchTool->addParameter("pattern", "string", "文件名匹配模式（支持 * 和 ?）", false, "*");
    localSearchTool->addParameter("maxDepth", "int", "最大递归深度（-1表示无限）", false, "-1");
    m_tools.append(localSearchTool);

    Tool* textSearchTool = new Tool("searchText", "在文件中搜索文本内容");
    textSearchTool->addParameter("dir", "string", "搜索起始目录", true);
    textSearchTool->addParameter("pattern", "string", "搜索关键词或正则表达式", true);
    textSearchTool->addParameter("filePattern", "string", "文件类型过滤（如 *.cpp, *.h）", false, "*");
    textSearchTool->addParameter("maxDepth", "int", "最大递归深度（-1表示无限）", false, "-1");
    textSearchTool->addParameter("caseSensitive", "bool", "是否区分大小写", false, "false");
    m_tools.append(textSearchTool);

    Tool* codeSearchTool = new Tool("searchCode", "搜索代码中的函数、类、变量等");
    codeSearchTool->addParameter("dir", "string", "搜索起始目录", true);
    codeSearchTool->addParameter("symbol", "string", "要搜索的符号名称", true);
    codeSearchTool->addParameter("language", "string", "语言过滤（cpp, python, java, javascript等）", false, "");
    m_tools.append(codeSearchTool);
}

QVariantMap SearchAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "webSearch") {
        return webSearch(args);
    } else if (toolName == "webFetch") {
        return webFetch(args);
    } else if (toolName == "searchFiles") {
        return searchFiles(args);
    } else if (toolName == "searchText") {
        return searchText(args);
    } else if (toolName == "searchCode") {
        return searchCode(args);
    }

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

QVariantMap SearchAgent::webSearch(const QVariantMap& args)
{
    QVariantMap result;
    QString query = args.value("query").toString();
    int maxResults = args.value("maxResults", 5).toInt();

    if (query.isEmpty()) {
        result["success"] = false;
        result["error"] = "搜索关键词不能为空";
        return result;
    }

    result["success"] = false;
    result["error"] = "当前处于离线模式，无法进行网页搜索。请连接网络后重试，或使用本地文件搜索功能。";
    return result;
}

QVariantMap SearchAgent::webFetch(const QVariantMap& args)
{
    QVariantMap result;
    QString urlStr = args.value("url").toString();

    if (urlStr.isEmpty()) {
        result["success"] = false;
        result["error"] = "URL 不能为空";
        return result;
    }

    result["success"] = false;
    result["error"] = "当前处于离线模式，无法抓取网页内容。请连接网络后重试。";
    return result;
}

QVariantMap SearchAgent::searchFiles(const QVariantMap& args)
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
    QDirIterator it(dirPath, QStringList() << pattern, 
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();

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

QVariantMap SearchAgent::searchText(const QVariantMap& args)
{
    QVariantMap result;
    QString dirPath = args.value("dir").toString();
    QString pattern = args.value("pattern").toString();
    QString filePattern = args.value("filePattern", "*").toString();
    int maxDepth = args.value("maxDepth", -1).toInt();
    bool caseSensitive = args.value("caseSensitive", false).toBool();

    if (pattern.isEmpty()) {
        result["success"] = false;
        result["error"] = "搜索关键词不能为空";
        return result;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "搜索目录不存在: " + dirPath;
        return result;
    }

    QJsonArray resultsArray;
    QDirIterator it(dirPath, QStringList() << filePattern, 
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    QRegularExpression re(pattern, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);

    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();

        QString relativePath = dir.relativeFilePath(info.absolutePath());
        int depth = relativePath.isEmpty() ? 0 : relativePath.count('/') + relativePath.count('\\') + 1;

        if (maxDepth >= 0 && depth > maxDepth) {
            continue;
        }

        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        if (content.contains(re)) {
            QRegularExpressionMatchIterator matchIt = re.globalMatch(content);
            QStringList matches;
            int count = 0;
            while (matchIt.hasNext() && count < 5) {
                QRegularExpressionMatch m = matchIt.next();
                QString context = content.mid(qMax(0, m.capturedStart() - 30), 60);
                matches.append(context.replace("\n", " ").replace("\r", ""));
                count++;
            }

            QJsonObject resultObj;
            resultObj["file"] = info.absoluteFilePath();
            resultObj["matches"] = matches.size();
            resultObj["context"] = QJsonArray::fromStringList(matches);
            resultsArray.append(resultObj);
        }
    }

    if (resultsArray.isEmpty()) {
        result["success"] = true;
        result["result"] = "未找到包含 '" + pattern + "' 的文件。";
        return result;
    }

    result["success"] = true;
    result["result"] = QJsonDocument(resultsArray).toJson(QJsonDocument::Compact);
    return result;
}

QVariantMap SearchAgent::searchCode(const QVariantMap& args)
{
    QVariantMap result;
    QString dirPath = args.value("dir").toString();
    QString symbol = args.value("symbol").toString();
    QString language = args.value("language").toString().toLower();

    if (symbol.isEmpty()) {
        result["success"] = false;
        result["error"] = "符号名称不能为空";
        return result;
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "搜索目录不存在: " + dirPath;
        return result;
    }

    QStringList filters;
    if (language == "cpp") {
        filters << "*.cpp" << "*.h" << "*.cxx" << "*.hpp";
    } else if (language == "python") {
        filters << "*.py";
    } else if (language == "java") {
        filters << "*.java";
    } else if (language == "javascript" || language == "js") {
        filters << "*.js" << "*.jsx" << "*.ts" << "*.tsx";
    } else if (language == "q") {
        filters << "*.qml" << "*.qrc";
    } else {
        filters << "*.cpp" << "*.h" << "*.cxx" << "*.hpp" << "*.py" << "*.java" << "*.js" << "*.ts" << "*.qml";
    }

    QJsonArray resultsArray;
    QDirIterator it(dirPath, filters, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    QRegularExpression classRe(QString("class\\s+%1").arg(QRegularExpression::escape(symbol)));
    QRegularExpression funcRe(QString("(void|int|float|double|bool|QString|QVariant|\\w+)\\s+%1\\s*\\(").arg(QRegularExpression::escape(symbol)));
    QRegularExpression varRe(QString("(const\\s+)?(\\w+)\\s+%1\\s*[=;]").arg(QRegularExpression::escape(symbol)));

    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();

        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        QStringList matches;

        QRegularExpressionMatchIterator classIt = classRe.globalMatch(content);
        while (classIt.hasNext()) {
            QRegularExpressionMatch m = classIt.next();
            matches.append("类定义: " + m.captured());
        }

        QRegularExpressionMatchIterator funcIt = funcRe.globalMatch(content);
        while (funcIt.hasNext()) {
            QRegularExpressionMatch m = funcIt.next();
            matches.append("函数: " + m.captured());
        }

        QRegularExpressionMatchIterator varIt = varRe.globalMatch(content);
        while (varIt.hasNext()) {
            QRegularExpressionMatch m = varIt.next();
            matches.append("变量: " + m.captured());
        }

        if (!matches.isEmpty()) {
            QJsonObject resultObj;
            resultObj["file"] = info.absoluteFilePath();
            resultObj["matches"] = QJsonArray::fromStringList(matches);
            resultsArray.append(resultObj);
        }
    }

    if (resultsArray.isEmpty()) {
        result["success"] = true;
        result["result"] = "未找到符号 '" + symbol + "' 的定义或引用。";
        return result;
    }

    result["success"] = true;
    result["result"] = QJsonDocument(resultsArray).toJson(QJsonDocument::Compact);
    return result;
}
