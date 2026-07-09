/**
 * @file textagent.cpp
 * @brief 文本处理代理实现
 *
 * @details
 * TextAgent是Agent基类的具体实现，负责各种文本处理操作。
 * 该代理注册了多个文本处理工具：Base64编解码、URL编解码、
 * MD5/SHA256哈希、字符统计、大小写转换、文本反转等。
 * 所有工具执行结果以QVariantMap形式返回，统一包含success和result/error字段。
 */

#include "textagent.h"
#include "tool.h"
#include <QByteArray>
#include <QCryptographicHash>
#include <QUrl>
#include <QTextStream>
#include <algorithm>

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 调用initializeTools()创建并注册所有文本处理工具。
 */
TextAgent::TextAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

/**
 * @brief 析构函数
 *
 * 释放initializeTools中动态分配的所有Tool对象。
 */
TextAgent::~TextAgent()
{
    qDeleteAll(m_tools);
}

/**
 * @brief 获取代理名称
 * @return 固定返回"TextAgent"
 */
QString TextAgent::name() const
{
    return "TextAgent";
}

/**
 * @brief 获取代理描述
 * @return 代理功能描述字符串
 */
QString TextAgent::description() const
{
    return "文本处理代理，负责Base64、URL编解码、哈希计算、文本转换等操作";
}

/**
 * @brief 获取代理类型
 * @return 固定返回TextAgentType
 */
Agent::AgentType TextAgent::type() const
{
    return TextAgentType;
}

/**
 * @brief 获取工具列表
 * @return 当前代理注册的所有Tool指针列表
 */
QList<Tool*> TextAgent::tools() const
{
    return m_tools;
}

/**
 * @brief 初始化并注册所有文本处理工具
 */
void TextAgent::initializeTools()
{
    Tool* base64EncodeTool = new Tool("base64Encode", "Base64编码");
    base64EncodeTool->addParameter("text", "string", "要编码的文本", true);
    m_tools.append(base64EncodeTool);

    Tool* base64DecodeTool = new Tool("base64Decode", "Base64解码");
    base64DecodeTool->addParameter("text", "string", "要解码的Base64文本", true);
    m_tools.append(base64DecodeTool);

    Tool* urlEncodeTool = new Tool("urlEncode", "URL编码");
    urlEncodeTool->addParameter("text", "string", "要编码的文本", true);
    m_tools.append(urlEncodeTool);

    Tool* urlDecodeTool = new Tool("urlDecode", "URL解码");
    urlDecodeTool->addParameter("text", "string", "要解码的URL文本", true);
    m_tools.append(urlDecodeTool);

    Tool* md5HashTool = new Tool("md5Hash", "计算MD5哈希");
    md5HashTool->addParameter("text", "string", "要计算的文本", true);
    m_tools.append(md5HashTool);

    Tool* sha256HashTool = new Tool("sha256Hash", "计算SHA256哈希");
    sha256HashTool->addParameter("text", "string", "要计算的文本", true);
    m_tools.append(sha256HashTool);

    Tool* charCountTool = new Tool("charCount", "字符统计");
    charCountTool->addParameter("text", "string", "要统计的文本", true);
    m_tools.append(charCountTool);

    Tool* toUpperCaseTool = new Tool("toUpperCase", "转换为大写");
    toUpperCaseTool->addParameter("text", "string", "要转换的文本", true);
    m_tools.append(toUpperCaseTool);

    Tool* toLowerCaseTool = new Tool("toLowerCase", "转换为小写");
    toLowerCaseTool->addParameter("text", "string", "要转换的文本", true);
    m_tools.append(toLowerCaseTool);

    Tool* trimTool = new Tool("trim", "去除首尾空白");
    trimTool->addParameter("text", "string", "要处理的文本", true);
    m_tools.append(trimTool);

    Tool* reverseTextTool = new Tool("reverseText", "反转文本");
    reverseTextTool->addParameter("text", "string", "要反转的文本", true);
    m_tools.append(reverseTextTool);

    Tool* wordCountTool = new Tool("wordCount", "字数统计");
    wordCountTool->addParameter("text", "string", "要统计的文本", true);
    m_tools.append(wordCountTool);

    Tool* sortLinesTool = new Tool("sortLines", "按行排序");
    sortLinesTool->addParameter("text", "string", "要排序的文本", true);
    sortLinesTool->addParameter("descending", "bool", "是否降序", false, "false");
    m_tools.append(sortLinesTool);

    Tool* htmlEscapeTool = new Tool("htmlEscape", "HTML转义");
    htmlEscapeTool->addParameter("text", "string", "要转义的文本", true);
    m_tools.append(htmlEscapeTool);

    Tool* htmlUnescapeTool = new Tool("htmlUnescape", "HTML反转义");
    htmlUnescapeTool->addParameter("text", "string", "要反转义的文本", true);
    m_tools.append(htmlUnescapeTool);

    Tool* dedupTool = new Tool("deduplicateLines", "按行去重");
    dedupTool->addParameter("text", "string", "要去重的文本", true);
    m_tools.append(dedupTool);
}

/**
 * @brief 工具分发执行入口
 * @param toolName 要执行的工具名称
 * @param args 工具参数字典（键值对）
 * @return 执行结果，包含success字段和result或error字段
 */
QVariantMap TextAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "base64Encode") {
        return base64Encode(args);
    } else if (toolName == "base64Decode") {
        return base64Decode(args);
    } else if (toolName == "urlEncode") {
        return urlEncode(args);
    } else if (toolName == "urlDecode") {
        return urlDecode(args);
    } else if (toolName == "md5Hash") {
        return md5Hash(args);
    } else if (toolName == "sha256Hash") {
        return sha256Hash(args);
    } else if (toolName == "charCount") {
        return charCount(args);
    } else if (toolName == "toUpperCase") {
        return toUpperCase(args);
    } else if (toolName == "toLowerCase") {
        return toLowerCase(args);
    } else if (toolName == "trim") {
        return trim(args);
    } else if (toolName == "reverseText") {
        return reverseText(args);
    } else if (toolName == "wordCount") {
        return wordCount(args);
    } else if (toolName == "sortLines") {
        return sortLines(args);
    } else if (toolName == "htmlEscape") {
        return htmlEscape(args);
    } else if (toolName == "htmlUnescape") {
        return htmlUnescape(args);
    } else if (toolName == "deduplicateLines") {
        return deduplicateLines(args);
    }

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

/**
 * @brief Base64编码
 * @param args 参数映射，包含text
 * @return Base64编码结果
 */
QVariantMap TextAgent::base64Encode(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QByteArray encoded = text.toUtf8().toBase64();
    result["success"] = true;
    result["result"] = QString::fromUtf8(encoded);
    return result;
}

/**
 * @brief Base64解码
 * @param args 参数映射，包含text
 * @return Base64解码结果
 */
QVariantMap TextAgent::base64Decode(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QByteArray decoded = QByteArray::fromBase64(text.toUtf8());
    if (decoded.isEmpty() && !text.isEmpty()) {
        result["success"] = false;
        result["error"] = "Base64解码失败，请检查输入";
        return result;
    }

    result["success"] = true;
    result["result"] = QString::fromUtf8(decoded);
    return result;
}

/**
 * @brief URL编码
 * @param args 参数映射，包含text
 * @return URL编码结果
 */
QVariantMap TextAgent::urlEncode(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QByteArray encoded = QUrl::toPercentEncoding(text);
    result["success"] = true;
    result["result"] = QString::fromUtf8(encoded);
    return result;
}

/**
 * @brief URL解码
 * @param args 参数映射，包含text
 * @return URL解码结果
 */
QVariantMap TextAgent::urlDecode(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QString decoded = QUrl::fromPercentEncoding(text.toUtf8());
    result["success"] = true;
    result["result"] = decoded;
    return result;
}

/**
 * @brief 计算MD5哈希
 * @param args 参数映射，包含text
 * @return MD5哈希值
 */
QVariantMap TextAgent::md5Hash(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5);
    result["success"] = true;
    result["result"] = QString::fromUtf8(hash.toHex());
    return result;
}

/**
 * @brief 计算SHA256哈希
 * @param args 参数映射，包含text
 * @return SHA256哈希值
 */
QVariantMap TextAgent::sha256Hash(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    result["success"] = true;
    result["result"] = QString::fromUtf8(hash.toHex());
    return result;
}

/**
 * @brief 字符统计
 * @param args 参数映射，包含text
 * @return 字符统计结果
 *
 * @details
 * 统计总字符数、字母数、数字数、空白符数、行数、中文字符数等。
 */
QVariantMap TextAgent::charCount(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    int total = text.size();
    int letters = 0;
    int digits = 0;
    int spaces = 0;
    int punctuations = 0;
    int newlines = 0;
    int chinese = 0;
    int words = 0;

    bool inWord = false;
    for (QChar c : text) {
        if (c == '\n') {
            newlines++;
        }
        if (c.isLetter()) {
            letters++;
            if (c.script() == QChar::Script_Han) {
                chinese++;
            }
            if (!inWord && c.isLetterOrNumber()) {
                inWord = true;
                words++;
            }
        } else if (c.isDigit()) {
            digits++;
            if (!inWord) {
                inWord = true;
                words++;
            }
        } else if (c.isSpace()) {
            spaces++;
            inWord = false;
        } else if (c.isPunct()) {
            punctuations++;
            inWord = false;
        } else {
            inWord = false;
        }
    }

    int lines = newlines + (text.isEmpty() ? 0 : 1);

    QString stats = QString(
        "总字符数: %1\n"
        "字母数: %2\n"
        "  中文字符: %3\n"
        "数字数: %4\n"
        "空白符: %5\n"
        "标点符号: %6\n"
        "换行数: %7\n"
        "总行数: %8\n"
        "单词数: %9"
    ).arg(total).arg(letters).arg(chinese).arg(digits).arg(spaces)
     .arg(punctuations).arg(newlines).arg(lines).arg(words);

    result["success"] = true;
    result["result"] = stats;
    return result;
}

/**
 * @brief 转换为大写
 * @param args 参数映射，包含text
 * @return 大写文本
 */
QVariantMap TextAgent::toUpperCase(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    result["success"] = true;
    result["result"] = text.toUpper();
    return result;
}

/**
 * @brief 转换为小写
 * @param args 参数映射，包含text
 * @return 小写文本
 */
QVariantMap TextAgent::toLowerCase(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    result["success"] = true;
    result["result"] = text.toLower();
    return result;
}

/**
 * @brief 去除首尾空白
 * @param args 参数映射，包含text
 * @return 去除空白后的文本
 */
QVariantMap TextAgent::trim(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    result["success"] = true;
    result["result"] = text.trimmed();
    return result;
}

/**
 * @brief 反转文本
 * @param args 参数映射，包含text
 * @return 反转后的文本
 */
QVariantMap TextAgent::reverseText(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QString reversed;
    for (int i = text.size() - 1; i >= 0; i--) {
        reversed.append(text.at(i));
    }

    result["success"] = true;
    result["result"] = reversed;
    return result;
}

/**
 * @brief 字数统计
 * @param args 参数映射，包含text
 * @return 字数统计结果
 */
QVariantMap TextAgent::wordCount(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    int charCount = text.size();
    int lineCount = text.isEmpty() ? 0 : text.count('\n') + 1;
    int wordCount = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();

    QString stats = QString(
        "字符数: %1\n"
        "单词数: %2\n"
        "行数: %3"
    ).arg(charCount).arg(wordCount).arg(lineCount);

    result["success"] = true;
    result["result"] = stats;
    return result;
}

/**
 * @brief 按行排序
 * @param args 参数映射，包含text、descending
 * @return 排序后的文本
 */
QVariantMap TextAgent::sortLines(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();
    bool descending = args.value("descending", false).toBool();

    QStringList lines = text.split('\n');
    std::sort(lines.begin(), lines.end());
    if (descending) {
        std::reverse(lines.begin(), lines.end());
    }

    result["success"] = true;
    result["result"] = lines.join("\n");
    return result;
}

/**
 * @brief HTML转义
 * @param args 参数映射，包含text
 * @return 转义后的HTML文本
 */
QVariantMap TextAgent::htmlEscape(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QString escaped = text;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&#39;");

    result["success"] = true;
    result["result"] = escaped;
    return result;
}

/**
 * @brief HTML反转义
 * @param args 参数映射，包含text
 * @return 反转义后的文本
 */
QVariantMap TextAgent::htmlUnescape(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QString unescaped = text;
    unescaped.replace("&lt;", "<");
    unescaped.replace("&gt;", ">");
    unescaped.replace("&quot;", "\"");
    unescaped.replace("&#39;", "'");
    unescaped.replace("&amp;", "&");

    result["success"] = true;
    result["result"] = unescaped;
    return result;
}

/**
 * @brief 按行去重
 * @param args 参数映射，包含text
 * @return 去重后的文本
 *
 * @details
 * 去除重复的行，保留第一次出现的顺序。
 */
QVariantMap TextAgent::deduplicateLines(const QVariantMap& args)
{
    QVariantMap result;
    QString text = args.value("text").toString();

    QStringList lines = text.split('\n');
    QSet<QString> seen;
    QStringList uniqueLines;

    for (const QString& line : lines) {
        if (!seen.contains(line)) {
            seen.insert(line);
            uniqueLines.append(line);
        }
    }

    int removed = lines.size() - uniqueLines.size();
    result["success"] = true;
    result["result"] = QString("去除了 %1 行重复，剩余 %2 行：\n\n%3")
        .arg(removed).arg(uniqueLines.size()).arg(uniqueLines.join("\n"));
    return result;
}
