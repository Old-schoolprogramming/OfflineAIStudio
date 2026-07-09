/**
 * @file searchagent.cpp
 * @brief 搜索代理实现
 *
 * @details
 * SearchAgent是Agent基类的具体实现，负责文件内容搜索和替换操作。
 * 该代理注册了多个搜索工具：searchInFiles、regexSearch、replaceInFiles、
 * searchByName、countOccurrences、findFilesWithKeywords。
 * 所有工具执行结果以QVariantMap形式返回，统一包含success和result/error字段。
 */

#include "searchagent.h"
#include "tool.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 调用initializeTools()创建并注册所有搜索工具。
 */
SearchAgent::SearchAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

/**
 * @brief 析构函数
 *
 * 释放initializeTools中动态分配的所有Tool对象。
 */
SearchAgent::~SearchAgent()
{
    qDeleteAll(m_tools);
}

/**
 * @brief 获取代理名称
 * @return 固定返回"SearchAgent"
 */
QString SearchAgent::name() const
{
    return "SearchAgent";
}

/**
 * @brief 获取代理描述
 * @return 代理功能描述字符串
 */
QString SearchAgent::description() const
{
    return "搜索代理，负责文件内容搜索、正则搜索、文本替换等操作";
}

/**
 * @brief 获取代理类型
 * @return 固定返回SearchAgentType
 */
Agent::AgentType SearchAgent::type() const
{
    return SearchAgentType;
}

/**
 * @brief 获取工具列表
 * @return 当前代理注册的所有Tool指针列表
 */
QList<Tool*> SearchAgent::tools() const
{
    return m_tools;
}

/**
 * @brief 初始化并注册所有搜索工具
 *
 * @details
 * 注册的工具及参数说明：
 * - searchInFiles: 文件内容搜索，必填path、pattern，可选recursive、caseSensitive
 * - regexSearch: 正则表达式搜索，必填path、pattern，可选recursive
 * - replaceInFiles: 文本替换，必填path、search、replace，可选recursive
 * - searchByName: 按文件名搜索，必填dir、pattern，可选recursive
 * - countOccurrences: 统计出现次数，必填path、pattern，可选recursive
 * - findFilesWithKeywords: 多关键词搜索，必填dir、keywords，可选recursive
 */
void SearchAgent::initializeTools()
{
    Tool* searchInFilesTool = new Tool("searchInFiles", "在文件内容中搜索文本");
    searchInFilesTool->addParameter("path", "string", "文件或目录路径", true);
    searchInFilesTool->addParameter("pattern", "string", "搜索的文本", true);
    searchInFilesTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    searchInFilesTool->addParameter("caseSensitive", "bool", "是否区分大小写", false, "false");
    m_tools.append(searchInFilesTool);

    Tool* regexSearchTool = new Tool("regexSearch", "使用正则表达式搜索");
    regexSearchTool->addParameter("path", "string", "文件或目录路径", true);
    regexSearchTool->addParameter("pattern", "string", "正则表达式", true);
    regexSearchTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(regexSearchTool);

    Tool* replaceInFilesTool = new Tool("replaceInFiles", "批量替换文件中的文本");
    replaceInFilesTool->addParameter("path", "string", "文件或目录路径", true);
    replaceInFilesTool->addParameter("search", "string", "要搜索的文本", true);
    replaceInFilesTool->addParameter("replace", "string", "替换为的文本", true);
    replaceInFilesTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(replaceInFilesTool);

    Tool* searchByNameTool = new Tool("searchByName", "按文件名搜索文件");
    searchByNameTool->addParameter("dir", "string", "搜索起始目录", true);
    searchByNameTool->addParameter("pattern", "string", "文件名匹配模式（如*.cpp）", true);
    searchByNameTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(searchByNameTool);

    Tool* countOccurrencesTool = new Tool("countOccurrences", "统计文本出现次数");
    countOccurrencesTool->addParameter("path", "string", "文件或目录路径", true);
    countOccurrencesTool->addParameter("pattern", "string", "要统计的文本", true);
    countOccurrencesTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(countOccurrencesTool);

    Tool* findFilesWithKeywordsTool = new Tool("findFilesWithKeywords", "查找包含所有关键词的文件");
    findFilesWithKeywordsTool->addParameter("dir", "string", "搜索起始目录", true);
    findFilesWithKeywordsTool->addParameter("keywords", "string", "关键词列表（用逗号分隔）", true);
    findFilesWithKeywordsTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(findFilesWithKeywordsTool);
}

/**
 * @brief 工具分发执行入口
 * @param toolName 要执行的工具名称
 * @param args 工具参数字典（键值对）
 * @return 执行结果，包含success字段和result或error字段
 */
QVariantMap SearchAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "searchInFiles") {
        return searchInFiles(args);
    } else if (toolName == "regexSearch") {
        return regexSearch(args);
    } else if (toolName == "replaceInFiles") {
        return replaceInFiles(args);
    } else if (toolName == "searchByName") {
        return searchByName(args);
    } else if (toolName == "countOccurrences") {
        return countOccurrences(args);
    } else if (toolName == "findFilesWithKeywords") {
        return findFilesWithKeywords(args);
    }

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

/**
 * @brief 在文件内容中搜索文本
 * @param args 参数映射，包含path、pattern、recursive、caseSensitive
 * @return 搜索结果
 *
 * @details
 * 在指定文件或目录下搜索包含指定文本的行，显示文件名、行号和内容。
 */
QVariantMap SearchAgent::searchInFiles(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString pattern = args.value("pattern").toString();
    bool recursive = args.value("recursive", true).toBool();
    bool caseSensitive = args.value("caseSensitive", false).toBool();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    QStringList results;
    int matchCount = 0;
    int fileCount = 0;
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        int lineNum = 0;
        int fileMatches = 0;
        QStringList fileResults;

        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            if (line.contains(pattern, cs)) {
                fileMatches++;
                fileResults << QString("  第%1行: %2").arg(lineNum).arg(line.trimmed().left(100));
            }
        }
        file.close();

        if (fileMatches > 0) {
            fileCount++;
            matchCount += fileMatches;
            results << QString("%1 (%2处匹配)").arg(QFileInfo(filePath).filePath()).arg(fileMatches);
            results.append(fileResults);
            results << "";
        }
    };

    if (info.isFile()) {
        processFile(path);
    } else {
        QDirIterator it(path, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            processFile(it.next());
        }
    }

    result["success"] = true;
    if (matchCount == 0) {
        result["result"] = "未找到匹配的内容";
    } else {
        result["result"] = QString("找到 %1 处匹配（共 %2 个文件）：\n\n%3")
            .arg(matchCount).arg(fileCount).arg(results.join("\n"));
    }
    return result;
}

/**
 * @brief 使用正则表达式搜索
 * @param args 参数映射，包含path、pattern、recursive
 * @return 搜索结果
 *
 * @details
 * 使用QRegularExpression进行正则匹配搜索。
 */
QVariantMap SearchAgent::regexSearch(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString pattern = args.value("pattern").toString();
    bool recursive = args.value("recursive", true).toBool();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    QRegularExpression regex(pattern);
    if (!regex.isValid()) {
        result["success"] = false;
        result["error"] = "无效的正则表达式: " + regex.errorString();
        return result;
    }

    QStringList results;
    int matchCount = 0;
    int fileCount = 0;

    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        int lineNum = 0;
        int fileMatches = 0;
        QStringList fileResults;

        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                fileMatches++;
                fileResults << QString("  第%1行: %2").arg(lineNum).arg(line.trimmed().left(100));
            }
        }
        file.close();

        if (fileMatches > 0) {
            fileCount++;
            matchCount += fileMatches;
            results << QString("%1 (%2处匹配)").arg(QFileInfo(filePath).filePath()).arg(fileMatches);
            results.append(fileResults);
            results << "";
        }
    };

    if (info.isFile()) {
        processFile(path);
    } else {
        QDirIterator it(path, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            processFile(it.next());
        }
    }

    result["success"] = true;
    if (matchCount == 0) {
        result["result"] = "未找到匹配的内容";
    } else {
        result["result"] = QString("正则搜索找到 %1 处匹配（共 %2 个文件）：\n\n%3")
            .arg(matchCount).arg(fileCount).arg(results.join("\n"));
    }
    return result;
}

/**
 * @brief 批量替换文件中的文本
 * @param args 参数映射，包含path、search、replace、recursive
 * @return 替换结果
 *
 * @details
 * 在指定文件或目录下批量替换文本。
 * 统计替换的文件数和总替换次数。
 */
QVariantMap SearchAgent::replaceInFiles(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString searchText = args.value("search").toString();
    QString replaceText = args.value("replace").toString();
    bool recursive = args.value("recursive", true).toBool();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    int fileCount = 0;
    int totalReplacements = 0;

    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        int count = content.count(searchText);
        if (count > 0) {
            content.replace(searchText, replaceText);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << content;
                file.close();
                fileCount++;
                totalReplacements += count;
            }
        }
    };

    if (info.isFile()) {
        processFile(path);
    } else {
        QDirIterator it(path, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            processFile(it.next());
        }
    }

    result["success"] = true;
    result["result"] = QString("替换完成：共修改 %1 个文件，替换 %2 处文本")
        .arg(fileCount).arg(totalReplacements);
    return result;
}

/**
 * @brief 按文件名搜索文件
 * @param args 参数映射，包含dir、pattern、recursive
 * @return 匹配的文件列表
 *
 * @details
 * 使用通配符模式匹配文件名进行搜索。
 */
QVariantMap SearchAgent::searchByName(const QVariantMap& args)
{
    QVariantMap result;
    QString dir = args.value("dir").toString();
    QString pattern = args.value("pattern").toString();
    bool recursive = args.value("recursive", true).toBool();

    QDir directory(dir);
    if (!directory.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + dir;
        return result;
    }

    QStringList nameFilters;
    nameFilters << pattern;

    QDirIterator it(dir, nameFilters, QDir::Files | QDir::NoDotAndDotDot,
                    recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

    QStringList matches;
    while (it.hasNext()) {
        matches << it.next();
    }

    result["success"] = true;
    if (matches.isEmpty()) {
        result["result"] = "未找到匹配的文件";
    } else {
        result["result"] = QString("找到 %1 个匹配文件:\n%2").arg(matches.size()).arg(matches.join("\n"));
    }
    return result;
}

/**
 * @brief 统计文本出现次数
 * @param args 参数映射，包含path、pattern、recursive
 * @return 统计结果
 */
QVariantMap SearchAgent::countOccurrences(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString pattern = args.value("pattern").toString();
    bool recursive = args.value("recursive", true).toBool();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    int totalCount = 0;
    int fileCount = 0;

    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        int count = content.count(pattern);
        if (count > 0) {
            fileCount++;
            totalCount += count;
        }
    };

    if (info.isFile()) {
        processFile(path);
    } else {
        QDirIterator it(path, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            processFile(it.next());
        }
    }

    result["success"] = true;
    result["result"] = QString("统计结果：\"%1\" 共出现 %2 次（分布在 %3 个文件中）")
        .arg(pattern).arg(totalCount).arg(fileCount);
    return result;
}

/**
 * @brief 查找包含所有指定关键词的文件
 * @param args 参数映射，包含dir、keywords、recursive
 * @return 匹配的文件列表
 *
 * @details
 * 查找同时包含所有关键词的文件。关键词用逗号分隔。
 */
QVariantMap SearchAgent::findFilesWithKeywords(const QVariantMap& args)
{
    QVariantMap result;
    QString dir = args.value("dir").toString();
    QString keywordsStr = args.value("keywords").toString();
    bool recursive = args.value("recursive", true).toBool();

    QDir directory(dir);
    if (!directory.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + dir;
        return result;
    }

    QStringList keywords = keywordsStr.split(",", Qt::SkipEmptyParts);
    for (QString& kw : keywords) {
        kw = kw.trimmed();
    }
    keywords.removeAll("");

    if (keywords.isEmpty()) {
        result["success"] = false;
        result["error"] = "请提供至少一个关键词";
        return result;
    }

    QStringList matches;

    QDirIterator it(dir, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        QString filePath = it.next();
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QString content = QTextStream(&file).readAll();
        file.close();

        bool hasAll = true;
        for (const QString& kw : keywords) {
            if (!content.contains(kw, Qt::CaseInsensitive)) {
                hasAll = false;
                break;
            }
        }

        if (hasAll) {
            matches << filePath;
        }
    }

    result["success"] = true;
    if (matches.isEmpty()) {
        result["result"] = "未找到包含所有关键词的文件";
    } else {
        result["result"] = QString("找到 %1 个包含所有关键词的文件:\n%2")
            .arg(matches.size()).arg(matches.join("\n"));
    }
    return result;
}
