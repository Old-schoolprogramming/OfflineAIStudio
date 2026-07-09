/**
 * @file codeagent.cpp
 * @brief 代码处理代理实现
 *
 * @details
 * CodeAgent是Agent基类的具体实现，负责代码相关的分析和处理。
 * 该代理注册了多个代码处理工具，包括代码行数统计、函数查找、
 * TODO标记查找、编码检测、注释比例统计等。
 * 所有工具执行结果以QVariantMap形式返回，统一包含success和result/error字段。
 */

#include "codeagent.h"
#include "tool.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTextCodec>
#include <QRegularExpression>

/**
 * @brief 构造函数
 * @param parent 父QObject对象
 *
 * 调用initializeTools()创建并注册所有代码处理工具。
 */
CodeAgent::CodeAgent(QObject *parent)
    : Agent(parent)
{
    initializeTools();
}

/**
 * @brief 析构函数
 *
 * 释放initializeTools中动态分配的所有Tool对象。
 */
CodeAgent::~CodeAgent()
{
    qDeleteAll(m_tools);
}

/**
 * @brief 获取代理名称
 * @return 固定返回"CodeAgent"
 */
QString CodeAgent::name() const
{
    return "CodeAgent";
}

/**
 * @brief 获取代理描述
 * @return 代理功能描述字符串
 */
QString CodeAgent::description() const
{
    return "代码处理代理，负责代码统计、分析、查找等操作";
}

/**
 * @brief 获取代理类型
 * @return 固定返回CodeAgentType
 */
Agent::AgentType CodeAgent::type() const
{
    return CodeAgentType;
}

/**
 * @brief 获取工具列表
 * @return 当前代理注册的所有Tool指针列表
 */
QList<Tool*> CodeAgent::tools() const
{
    return m_tools;
}

/**
 * @brief 初始化并注册所有代码处理工具
 *
 * @details
 * 注册的工具及参数说明：
 * - countLines: 统计代码行数，必填参数path，可选recursive
 * - findFunctions: 查找函数定义，必填参数path
 * - findTodos: 查找TODO/FIXME标记，必填参数path，可选recursive
 * - detectEncoding: 检测文件编码，必填参数path
 * - commentRatio: 统计注释比例，必填参数path
 * - findClasses: 查找类定义，必填参数path
 * - removeBlankLines: 移除空白行，必填参数path，可选outputPath
 * - charStats: 字符统计，必填参数path
 */
void CodeAgent::initializeTools()
{
    Tool* countLinesTool = new Tool("countLines", "统计代码行数");
    countLinesTool->addParameter("path", "string", "文件或目录路径", true);
    countLinesTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(countLinesTool);

    Tool* findFunctionsTool = new Tool("findFunctions", "查找代码中的函数定义");
    findFunctionsTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(findFunctionsTool);

    Tool* findTodosTool = new Tool("findTodos", "查找代码中的TODO/FIXME标记");
    findTodosTool->addParameter("path", "string", "文件或目录路径", true);
    findTodosTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(findTodosTool);

    Tool* detectEncodingTool = new Tool("detectEncoding", "检测文件编码格式");
    detectEncodingTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(detectEncodingTool);

    Tool* commentRatioTool = new Tool("commentRatio", "统计代码注释比例");
    commentRatioTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(commentRatioTool);

    Tool* findClassesTool = new Tool("findClasses", "查找代码中的类定义");
    findClassesTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(findClassesTool);

    Tool* removeBlankLinesTool = new Tool("removeBlankLines", "移除代码中的空白行");
    removeBlankLinesTool->addParameter("path", "string", "文件路径", true);
    removeBlankLinesTool->addParameter("outputPath", "string", "输出文件路径（可选，默认覆盖原文件）", false, "");
    m_tools.append(removeBlankLinesTool);

    Tool* charStatsTool = new Tool("charStats", "统计文件字符分布");
    charStatsTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(charStatsTool);
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
QVariantMap CodeAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "countLines") {
        return countLines(args);
    } else if (toolName == "findFunctions") {
        return findFunctions(args);
    } else if (toolName == "findTodos") {
        return findTodos(args);
    } else if (toolName == "detectEncoding") {
        return detectEncoding(args);
    } else if (toolName == "commentRatio") {
        return commentRatio(args);
    } else if (toolName == "findClasses") {
        return findClasses(args);
    } else if (toolName == "removeBlankLines") {
        return removeBlankLines(args);
    } else if (toolName == "charStats") {
        return charStats(args);
    }

    // 未知工具返回通用错误
    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

/**
 * @brief 统计代码行数
 * @param args 参数映射，包含path、recursive
 * @return 行数统计结果
 *
 * @details
 * 统计单个文件或目录下所有代码文件的行数。
 * 分别统计总行数、代码行数、注释行数、空白行数。
 */
QVariantMap CodeAgent::countLines(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    bool recursive = args.value("recursive", true).toBool();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    int totalLines = 0;
    int codeLines = 0;
    int commentLines = 0;
    int blankLines = 0;
    int fileCount = 0;

    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        bool inBlockComment = false;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            totalLines++;

            if (line.isEmpty()) {
                blankLines++;
                continue;
            }

            bool isComment = false;
            if (inBlockComment) {
                isComment = true;
                if (line.contains("*/") || line.contains("*/")) {
                    inBlockComment = false;
                }
            } else if (line.startsWith("//") || line.startsWith("#")) {
                isComment = true;
            } else if (line.startsWith("/*") || line.startsWith("\"\"\"") || line.startsWith("'''")) {
                isComment = true;
                inBlockComment = true;
                if (line.endsWith("*/") || line.endsWith("\"\"\"") || line.endsWith("'''")) {
                    inBlockComment = false;
                }
            }

            if (isComment) {
                commentLines++;
            } else {
                codeLines++;
            }
        }
        file.close();
        fileCount++;
    };

    if (info.isFile()) {
        processFile(path);
    } else {
        QDirIterator it(path, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            processFile(it.next());
        }
    }

    QString resultStr = QString(
        "=== 代码行数统计 ===\n"
        "文件数量: %1\n"
        "总行数: %2\n"
        "代码行数: %3\n"
        "注释行数: %4\n"
        "空白行数: %5\n"
        "代码占比: %6%"
    ).arg(fileCount).arg(totalLines).arg(codeLines).arg(commentLines).arg(blankLines)
     .arg(totalLines > 0 ? codeLines * 100 / totalLines : 0);

    result["success"] = true;
    result["result"] = resultStr;
    return result;
}

/**
 * @brief 查找代码中的函数定义
 * @param args 参数映射，包含path
 * @return 找到的函数列表
 *
 * @details
 * 使用正则表达式匹配常见语言的函数定义：
 * C/C++/Java/C#: 返回类型 函数名(参数)
 * Python: def 函数名(参数):
 * JavaScript: function 函数名(参数)
 */
QVariantMap CodeAgent::findFunctions(const QVariantMap& args)
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
    QStringList functions;
    int lineNum = 0;

    // 多种语言的函数定义正则
    QList<QRegularExpression> patterns;
    patterns << QRegularExpression("^\\s*(?:public|private|protected|static|virtual|inline)?\\s*[\\w<>]+\\s+(\\w+)\\s*\\("); // C++/Java
    patterns << QRegularExpression("^\\s*def\\s+(\\w+)\\s*\\("); // Python
    patterns << QRegularExpression("^\\s*function\\s+(\\w+)\\s*\\("); // JavaScript
    patterns << QRegularExpression("^\\s*func\\s+(?:\\w+\\s*)?(\\w+)\\s*\\("); // Go
    patterns << QRegularExpression("^\\s*(?:async\\s+)?function\\s+(\\w+)\\s*\\("); // JS async

    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;

        for (const auto& regex : patterns) {
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                QString funcName = match.captured(1);
                functions << QString("第 %1 行: %2").arg(lineNum).arg(funcName);
                break;
            }
        }
    }
    file.close();

    result["success"] = true;
    if (functions.isEmpty()) {
        result["result"] = "未找到函数定义";
    } else {
        result["result"] = QString("找到 %1 个函数:\n%2").arg(functions.size()).arg(functions.join("\n"));
    }
    return result;
}

/**
 * @brief 查找代码中的TODO/FIXME标记
 * @param args 参数映射，包含path、recursive
 * @return 找到的TODO/FIXME列表
 *
 * @details
 * 递归搜索目录下所有文件中的TODO、FIXME、XXX、HACK等标记。
 */
QVariantMap CodeAgent::findTodos(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    bool recursive = args.value("recursive", true).toBool();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    QStringList todos;

    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        int lineNum = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            lineNum++;
            if (line.contains("TODO") || line.contains("FIXME") ||
                line.contains("XXX") || line.contains("HACK") ||
                line.contains("BUG") || line.contains("OPTIMIZE")) {
                todos << QString("%1:第%2行: %3").arg(QFileInfo(filePath).fileName()).arg(lineNum).arg(line.trimmed());
            }
        }
        file.close();
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
    if (todos.isEmpty()) {
        result["result"] = "未找到TODO/FIXME标记";
    } else {
        result["result"] = QString("找到 %1 个标记:\n\n%2").arg(todos.size()).arg(todos.join("\n"));
    }
    return result;
}

/**
 * @brief 检测文件编码格式
 * @param args 参数映射，包含path
 * @return 文件编码信息
 *
 * @details
 * 通过BOM检测和启发式方法判断文件编码。
 * 支持检测UTF-8、UTF-16、UTF-32等常见编码。
 */
QVariantMap CodeAgent::detectEncoding(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFile file(path);
    if (!file.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + path;
        return result;
    }

    QByteArray data = file.read(1024);
    file.close();

    QString encoding;
    QString confidence;

    // 检测BOM
    if (data.startsWith("\xEF\xBB\xBF")) {
        encoding = "UTF-8 (带BOM)";
        confidence = "高";
    } else if (data.startsWith("\xFF\xFE")) {
        encoding = "UTF-16 LE (带BOM)";
        confidence = "高";
    } else if (data.startsWith("\xFE\xFF")) {
        encoding = "UTF-16 BE (带BOM)";
        confidence = "高";
    } else if (data.startsWith("\x00\x00\xFE\xFF")) {
        encoding = "UTF-32 BE (带BOM)";
        confidence = "高";
    } else {
        // 无BOM，尝试UTF-8检测
        bool isUtf8 = true;
        int i = 0;
        while (i < data.size()) {
            unsigned char c = static_cast<unsigned char>(data[i]);
            if (c < 0x80) {
                i++;
            } else if ((c & 0xE0) == 0xC0) {
                if (i + 1 >= data.size() || (static_cast<unsigned char>(data[i+1]) & 0xC0) != 0x80) {
                    isUtf8 = false;
                    break;
                }
                i += 2;
            } else if ((c & 0xF0) == 0xE0) {
                if (i + 2 >= data.size() ||
                    (static_cast<unsigned char>(data[i+1]) & 0xC0) != 0x80 ||
                    (static_cast<unsigned char>(data[i+2]) & 0xC0) != 0x80) {
                    isUtf8 = false;
                    break;
                }
                i += 3;
            } else {
                isUtf8 = false;
                break;
            }
        }

        if (isUtf8) {
            encoding = "UTF-8 (无BOM)";
            confidence = "中";
        } else {
            encoding = "未知编码（可能是GBK/GB2312/ISO-8859-1等）";
            confidence = "低";
        }
    }

    QFileInfo fileInfo(path);
    QString resultStr = QString(
        "文件: %1\n"
        "大小: %2 字节\n"
        "检测编码: %3\n"
        "置信度: %4"
    ).arg(fileInfo.fileName()).arg(fileInfo.size()).arg(encoding).arg(confidence);

    result["success"] = true;
    result["result"] = resultStr;
    return result;
}

/**
 * @brief 统计代码注释比例
 * @param args 参数映射，包含path
 * @return 注释比例统计
 *
 * @details
 * 统计单个文件的注释占比，包括行注释和块注释。
 */
QVariantMap CodeAgent::commentRatio(const QVariantMap& args)
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
    int totalLines = 0;
    int commentLines = 0;
    int codeLines = 0;
    int blankLines = 0;
    bool inBlockComment = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        totalLines++;

        if (line.isEmpty()) {
            blankLines++;
            continue;
        }

        bool isComment = false;

        if (inBlockComment) {
            isComment = true;
            if (line.contains("*/")) {
                inBlockComment = false;
            }
        } else if (line.startsWith("//") || line.startsWith("#") || line.startsWith("--")) {
            isComment = true;
        } else if (line.startsWith("/*") || line.startsWith("\"\"\"") || line.startsWith("'''")) {
            isComment = true;
            inBlockComment = true;
            if (line.endsWith("*/") || (line.size() > 3 && (line.endsWith("\"\"\"") || line.endsWith("'''")))) {
                inBlockComment = false;
            }
        }

        if (isComment) {
            commentLines++;
        } else {
            codeLines++;
        }
    }
    file.close();

    int totalEffective = codeLines + commentLines;
    int ratio = totalEffective > 0 ? commentLines * 100 / totalEffective : 0;

    QString resultStr = QString(
        "=== 注释比例统计 ===\n"
        "文件: %1\n"
        "总行数: %2\n"
        "代码行: %3\n"
        "注释行: %4\n"
        "空白行: %5\n"
        "注释占比: %6%"
    ).arg(QFileInfo(path).fileName()).arg(totalLines).arg(codeLines).arg(commentLines).arg(blankLines).arg(ratio);

    result["success"] = true;
    result["result"] = resultStr;
    return result;
}

/**
 * @brief 查找代码中的类定义
 * @param args 参数映射，包含path
 * @return 找到的类列表
 *
 * @details
 * 使用正则表达式匹配常见语言的类定义。
 */
QVariantMap CodeAgent::findClasses(const QVariantMap& args)
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
    QStringList classes;
    int lineNum = 0;

    QList<QRegularExpression> patterns;
    patterns << QRegularExpression("^\\s*(?:public|private|protected|abstract|sealed|final)?\\s*class\\s+(\\w+)"); // C++/Java/C#
    patterns << QRegularExpression("^\\s*class\\s+(\\w+)\\s*:"); // Python
    patterns << QRegularExpression("^\\s*interface\\s+(\\w+)"); // interface
    patterns << QRegularExpression("^\\s*struct\\s+(\\w+)"); // struct
    patterns << QRegularExpression("^\\s*type\\s+(\\w+)\\s+struct"); // Go struct

    while (!in.atEnd()) {
        QString line = in.readLine();
        lineNum++;

        for (const auto& regex : patterns) {
            QRegularExpressionMatch match = regex.match(line);
            if (match.hasMatch()) {
                QString className = match.captured(1);
                classes << QString("第 %1 行: class %2").arg(lineNum).arg(className);
                break;
            }
        }
    }
    file.close();

    result["success"] = true;
    if (classes.isEmpty()) {
        result["result"] = "未找到类定义";
    } else {
        result["result"] = QString("找到 %1 个类/结构:\n%2").arg(classes.size()).arg(classes.join("\n"));
    }
    return result;
}

/**
 * @brief 移除代码中的空白行
 * @param args 参数映射，包含path、outputPath
 * @return 处理结果
 *
 * @details
 * 移除文件中的所有空白行。如果指定了outputPath则写入新文件，
 * 否则覆盖原文件。
 */
QVariantMap CodeAgent::removeBlankLines(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString outputPath = args.value("outputPath", "").toString();

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
    int removedCount = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) {
            removedCount++;
        } else {
            lines << line;
        }
    }
    file.close();

    QString targetPath = outputPath.isEmpty() ? path : outputPath;
    QFile outFile(targetPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法写入文件: " + targetPath;
        return result;
    }

    QTextStream out(&outFile);
    for (const QString& line : lines) {
        out << line << "\n";
    }
    outFile.close();

    result["success"] = true;
    result["result"] = QString("已移除 %1 个空白行，结果保存在: %2").arg(removedCount).arg(targetPath);
    return result;
}

/**
 * @brief 统计文件字符分布
 * @param args 参数映射，包含path
 * @return 字符统计结果
 *
 * @details
 * 统计文件中的字符总数、字母数、数字数、空白符数等。
 */
QVariantMap CodeAgent::charStats(const QVariantMap& args)
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

    QString content = QTextStream(&file).readAll();
    file.close();

    int totalChars = content.size();
    int letterCount = 0;
    int digitCount = 0;
    int spaceCount = 0;
    int punctuationCount = 0;
    int chineseCount = 0;
    int newlineCount = 0;

    for (QChar c : content) {
        if (c == '\n' || c == '\r') {
            newlineCount++;
        } else if (c.isLetter()) {
            letterCount++;
            if (c.script() == QChar::Script_Han) {
                chineseCount++;
            }
        } else if (c.isDigit()) {
            digitCount++;
        } else if (c.isSpace()) {
            spaceCount++;
        } else if (c.isPunct()) {
            punctuationCount++;
        }
    }

    QString resultStr = QString(
        "=== 字符统计 ===\n"
        "文件: %1\n"
        "总字符数: %2\n"
        "字母数: %3\n"
        "  其中中文字符: %4\n"
        "数字数: %5\n"
        "空白符数: %6\n"
        "标点符号数: %7\n"
        "换行符数: %8"
    ).arg(QFileInfo(path).fileName())
     .arg(totalChars).arg(letterCount).arg(chineseCount)
     .arg(digitCount).arg(spaceCount).arg(punctuationCount).arg(newlineCount);

    result["success"] = true;
    result["result"] = resultStr;
    return result;
}
