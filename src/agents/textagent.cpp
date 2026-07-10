// 【文件说明】textagent.cpp - 文本处理Agent的实现文件
// 【作用】实现 TextAgent 类中声明的所有函数，提供Base64编解码、URL编解码、哈希计算、文本转换与格式化等功能
// 【注意事项】所有工具执行结果以 QVariantMap 形式返回，统一包含 success 和 result/error 字段

#include "textagent.h"          // 引入 TextAgent 自己的头文件，获取类声明
#include "tool.h"               // 引入 Tool 类头文件，用于创建和注册工具
#include <QByteArray>           // 引入 Qt 字节数组类，用于Base64和哈希计算时的二进制数据处理
#include <QCryptographicHash>   // 引入 Qt 加密哈希类，用于计算MD5和SHA256哈希值
#include <QUrl>                 // 引入 Qt URL类，用于URL编码和解码
#include <QTextStream>          // 引入 Qt 文本流类，用于读写文本内容
#include <QRegularExpression>   // 引入 Qt 正则表达式类，用于文本匹配、分割、邮箱和URL提取
#include <algorithm>            // 引入 C++ 标准算法库，用于sort排序和reverse反转
#include <QJsonDocument>        // 引入 Qt JSON文档类，用于JSON格式化时的解析和输出
#include <QJsonObject>          // 引入 Qt JSON对象类，用于处理JSON对象
#include <QJsonArray>           // 引入 Qt JSON数组类，用于处理JSON数组

// 【功能说明】TextAgent 构造函数，创建文本处理代理实例
// 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
// 【说明】调用父类 Agent 的构造函数，并执行 initializeTools() 注册所有文本处理工具
TextAgent::TextAgent(QObject *parent)
    : Agent(parent)  // 调用父类 Agent 的构造函数，传入父对象指针
{
    initializeTools();  // 初始化并注册所有文本处理工具到 m_tools 列表
}

// 【功能说明】TextAgent 析构函数，销毁文本处理代理实例
// 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象，防止内存泄漏
TextAgent::~TextAgent()
{
    qDeleteAll(m_tools);  // 遍历 m_tools 列表，删除每一个 Tool 对象指针指向的内存
}

// 【功能说明】获取代理名称
// 【返回值】固定返回 QString 字符串 "TextAgent"
// 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
QString TextAgent::name() const
{
    return "TextAgent";  // 返回此 Agent 的唯一标识名称
}

// 【功能说明】获取代理功能描述
// 【返回值】返回描述此 Agent 功能的 QString 字符串
// 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
QString TextAgent::description() const
{
    return "文本处理代理，负责Base64、URL编解码、哈希计算、文本转换、格式化等操作";  // 返回中文功能描述
}

// 【功能说明】获取代理类型
// 【返回值】固定返回枚举值 TextAgentType
// 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
Agent::AgentType TextAgent::type() const
{
    return TextAgentType;  // 返回文本处理代理类型枚举值
}

// 【功能说明】获取当前代理注册的所有工具列表
// 【返回值】返回 QList<Tool*> 类型的工具指针列表
// 【说明】重写父类虚函数，外部系统调用此函数获取可用工具清单
QList<Tool*> TextAgent::tools() const
{
    return m_tools;  // 返回内部存储的工具列表（m_tools 是成员变量）
}

// 【功能说明】初始化并注册所有文本处理工具
// 【说明】在构造函数中调用一次，为每个功能创建一个 Tool 对象并添加到 m_tools 列表
void TextAgent::initializeTools()
{
    // ========== 1. 注册 base64Encode 工具 ==========
    // 【工具说明】将普通文本编码为 Base64 字符串
    // 【必填参数】text - 要编码的原始文本
    Tool* base64EncodeTool = new Tool("base64Encode", "Base64编码",
        [this](const QVariantMap& args) { return this->base64Encode(args); });
    base64EncodeTool->addParameter("text", "string", "要编码的文本", true);  // 添加必填参数：原始文本
    m_tools.append(base64EncodeTool);  // 将工具添加到列表

    // ========== 2. 注册 base64Decode 工具 ==========
    // 【工具说明】将 Base64 字符串解码为普通文本
    // 【必填参数】text - 要解码的 Base64 文本
    Tool* base64DecodeTool = new Tool("base64Decode", "Base64解码",
        [this](const QVariantMap& args) { return this->base64Decode(args); });
    base64DecodeTool->addParameter("text", "string", "要解码的Base64文本", true);  // 添加必填参数
    m_tools.append(base64DecodeTool);  // 将工具添加到列表

    // ========== 3. 注册 urlEncode 工具 ==========
    // 【工具说明】对文本进行 URL 编码（Percent Encoding），将特殊字符转为 %XX 格式
    // 【必填参数】text - 要编码的文本
    Tool* urlEncodeTool = new Tool("urlEncode", "URL编码",
        [this](const QVariantMap& args) { return this->urlEncode(args); });
    urlEncodeTool->addParameter("text", "string", "要编码的文本", true);  // 添加必填参数
    m_tools.append(urlEncodeTool);  // 将工具添加到列表

    // ========== 4. 注册 urlDecode 工具 ==========
    // 【工具说明】对 URL 编码的文本进行解码，将 %XX 格式还原为原始字符
    // 【必填参数】text - 要解码的 URL 文本
    Tool* urlDecodeTool = new Tool("urlDecode", "URL解码",
        [this](const QVariantMap& args) { return this->urlDecode(args); });
    urlDecodeTool->addParameter("text", "string", "要解码的URL文本", true);  // 添加必填参数
    m_tools.append(urlDecodeTool);  // 将工具添加到列表

    // ========== 5. 注册 md5Hash 工具 ==========
    // 【工具说明】计算文本的 MD5 哈希值，生成 32 位十六进制字符串
    // 【必填参数】text - 要计算哈希的文本
    Tool* md5HashTool = new Tool("md5Hash", "计算MD5哈希",
        [this](const QVariantMap& args) { return this->md5Hash(args); });
    md5HashTool->addParameter("text", "string", "要计算的文本", true);  // 添加必填参数
    m_tools.append(md5HashTool);  // 将工具添加到列表

    // ========== 6. 注册 sha256Hash 工具 ==========
    // 【工具说明】计算文本的 SHA256 哈希值，生成 64 位十六进制字符串
    // 【必填参数】text - 要计算哈希的文本
    Tool* sha256HashTool = new Tool("sha256Hash", "计算SHA256哈希",
        [this](const QVariantMap& args) { return this->sha256Hash(args); });
    sha256HashTool->addParameter("text", "string", "要计算的文本", true);  // 添加必填参数
    m_tools.append(sha256HashTool);  // 将工具添加到列表

    // ========== 7. 注册 charCount 工具 ==========
    // 【工具说明】统计文本中的字符详细信息（字母、数字、中文、标点、换行、单词等）
    // 【必填参数】text - 要统计的文本
    Tool* charCountTool = new Tool("charCount", "字符统计",
        [this](const QVariantMap& args) { return this->charCount(args); });
    charCountTool->addParameter("text", "string", "要统计的文本", true);  // 添加必填参数
    m_tools.append(charCountTool);  // 将工具添加到列表

    // ========== 8. 注册 toUpperCase 工具 ==========
    // 【工具说明】将文本中的所有字母转换为大写
    // 【必填参数】text - 要转换的文本
    Tool* toUpperCaseTool = new Tool("toUpperCase", "转换为大写",
        [this](const QVariantMap& args) { return this->toUpperCase(args); });
    toUpperCaseTool->addParameter("text", "string", "要转换的文本", true);  // 添加必填参数
    m_tools.append(toUpperCaseTool);  // 将工具添加到列表

    // ========== 9. 注册 toLowerCase 工具 ==========
    // 【工具说明】将文本中的所有字母转换为小写
    // 【必填参数】text - 要转换的文本
    Tool* toLowerCaseTool = new Tool("toLowerCase", "转换为小写",
        [this](const QVariantMap& args) { return this->toLowerCase(args); });
    toLowerCaseTool->addParameter("text", "string", "要转换的文本", true);  // 添加必填参数
    m_tools.append(toLowerCaseTool);  // 将工具添加到列表

    // ========== 10. 注册 trim 工具 ==========
    // 【工具说明】去除文本首尾的空白字符（空格、制表符、换行等）
    // 【必填参数】text - 要处理的文本
    Tool* trimTool = new Tool("trim", "去除首尾空白",
        [this](const QVariantMap& args) { return this->trim(args); });
    trimTool->addParameter("text", "string", "要处理的文本", true);  // 添加必填参数
    m_tools.append(trimTool);  // 将工具添加到列表

    // ========== 11. 注册 reverseText 工具 ==========
    // 【工具说明】反转文本字符顺序（最后一个字符变第一个）
    // 【必填参数】text - 要反转的文本
    Tool* reverseTextTool = new Tool("reverseText", "反转文本",
        [this](const QVariantMap& args) { return this->reverseText(args); });
    reverseTextTool->addParameter("text", "string", "要反转的文本", true);  // 添加必填参数
    m_tools.append(reverseTextTool);  // 将工具添加到列表

    // ========== 12. 注册 wordCount 工具 ==========
    // 【工具说明】统计文本的字符数、单词数和行数
    // 【必填参数】text - 要统计的文本
    Tool* wordCountTool = new Tool("wordCount", "字数统计",
        [this](const QVariantMap& args) { return this->wordCount(args); });
    wordCountTool->addParameter("text", "string", "要统计的文本", true);  // 添加必填参数
    m_tools.append(wordCountTool);  // 将工具添加到列表

    // ========== 13. 注册 sortLines 工具 ==========
    // 【工具说明】按行对文本进行排序
    // 【必填参数】text - 要排序的文本
    // 【可选参数】descending - 是否降序排列，默认 false（升序）
    Tool* sortLinesTool = new Tool("sortLines", "按行排序",
        [this](const QVariantMap& args) { return this->sortLines(args); });
    sortLinesTool->addParameter("text", "string", "要排序的文本", true);  // 添加必填参数
    sortLinesTool->addParameter("descending", "bool", "是否降序", false, "false");  // 添加可选参数
    m_tools.append(sortLinesTool);  // 将工具添加到列表

    // ========== 14. 注册 htmlEscape 工具 ==========
    // 【工具说明】将文本中的特殊 HTML 字符（< > " ' &）转为 HTML 实体，防止 XSS
    // 【必填参数】text - 要转义的文本
    Tool* htmlEscapeTool = new Tool("htmlEscape", "HTML转义",
        [this](const QVariantMap& args) { return this->htmlEscape(args); });
    htmlEscapeTool->addParameter("text", "string", "要转义的文本", true);  // 添加必填参数
    m_tools.append(htmlEscapeTool);  // 将工具添加到列表

    // ========== 15. 注册 htmlUnescape 工具 ==========
    // 【工具说明】将 HTML 实体还原为原始字符
    // 【必填参数】text - 要反转义的文本
    Tool* htmlUnescapeTool = new Tool("htmlUnescape", "HTML反转义",
        [this](const QVariantMap& args) { return this->htmlUnescape(args); });
    htmlUnescapeTool->addParameter("text", "string", "要反转义的文本", true);  // 添加必填参数
    m_tools.append(htmlUnescapeTool);  // 将工具添加到列表

    // ========== 16. 注册 deduplicateLines 工具 ==========
    // 【工具说明】按行去重，保留第一次出现的行，删除后续重复行
    // 【必填参数】text - 要去重的文本
    Tool* dedupTool = new Tool("deduplicateLines", "按行去重",
        [this](const QVariantMap& args) { return this->deduplicateLines(args); });
    dedupTool->addParameter("text", "string", "要去重的文本", true);  // 添加必填参数
    m_tools.append(dedupTool);  // 将工具添加到列表

    // ========== 17. 注册 jsonFormat 工具 ==========
    // 【工具说明】将压缩的 JSON 文本格式化为带缩进的可读形式
    // 【必填参数】text - JSON 文本
    Tool* jsonFormatTool = new Tool("jsonFormat", "JSON格式化",
        [this](const QVariantMap& args) { return this->jsonFormat(args); });
    jsonFormatTool->addParameter("text", "string", "JSON文本", true);  // 添加必填参数
    m_tools.append(jsonFormatTool);  // 将工具添加到列表

    // ========== 18. 注册 xmlFormat 工具 ==========
    // 【工具说明】将压缩的 XML 文本格式化为带缩进和换行的可读形式
    // 【必填参数】text - XML 文本
    Tool* xmlFormatTool = new Tool("xmlFormat", "XML格式化",
        [this](const QVariantMap& args) { return this->xmlFormat(args); });
    xmlFormatTool->addParameter("text", "string", "XML文本", true);  // 添加必填参数
    m_tools.append(xmlFormatTool);  // 将工具添加到列表

    // ========== 19. 注册 extractEmail 工具 ==========
    // 【工具说明】从文本中提取所有符合格式的邮箱地址
    // 【必填参数】text - 要搜索的文本
    Tool* extractEmailTool = new Tool("extractEmail", "提取邮箱地址",
        [this](const QVariantMap& args) { return this->extractEmail(args); });
    extractEmailTool->addParameter("text", "string", "要搜索的文本", true);  // 添加必填参数
    m_tools.append(extractEmailTool);  // 将工具添加到列表

    // ========== 20. 注册 extractUrl 工具 ==========
    // 【工具说明】从文本中提取所有 HTTP/HTTPS 链接
    // 【必填参数】text - 要搜索的文本
    Tool* extractUrlTool = new Tool("extractUrl", "提取URL链接",
        [this](const QVariantMap& args) { return this->extractUrl(args); });
    extractUrlTool->addParameter("text", "string", "要搜索的文本", true);  // 添加必填参数
    m_tools.append(extractUrlTool);  // 将工具添加到列表

    // ========== 21. 注册 replaceText 工具 ==========
    // 【工具说明】在文本中批量替换指定内容
    // 【必填参数】text - 原始文本；oldText - 要替换的内容；newText - 替换后的内容
    // 【可选参数】caseSensitive - 是否区分大小写，默认 true
    Tool* replaceTextTool = new Tool("replaceText", "文本替换",
        [this](const QVariantMap& args) { return this->replaceText(args); });
    replaceTextTool->addParameter("text", "string", "原始文本", true);  // 添加必填参数
    replaceTextTool->addParameter("oldText", "string", "要替换的文本", true);  // 添加必填参数
    replaceTextTool->addParameter("newText", "string", "替换后的文本", true);  // 添加必填参数
    replaceTextTool->addParameter("caseSensitive", "bool", "是否区分大小写", false, "true");  // 添加可选参数
    m_tools.append(replaceTextTool);  // 将工具添加到列表

    // ========== 22. 注册 splitText 工具 ==========
    // 【工具说明】按指定分隔符将文本分割为多行
    // 【必填参数】text - 要分割的文本
    // 【可选参数】delimiter - 分隔符，默认逗号 ","
    Tool* splitTextTool = new Tool("splitText", "文本分割",
        [this](const QVariantMap& args) { return this->splitText(args); });
    splitTextTool->addParameter("text", "string", "要分割的文本", true);  // 添加必填参数
    splitTextTool->addParameter("delimiter", "string", "分隔符", false, ",");  // 添加可选参数
    m_tools.append(splitTextTool);  // 将工具添加到列表

    // ========== 23. 注册 joinText 工具 ==========
    // 【工具说明】将多个文本用指定分隔符拼接为一个字符串
    // 【必填参数】texts - 文本数组
    // 【可选参数】separator - 拼接分隔符，默认逗号 ","
    Tool* joinTextTool = new Tool("joinText", "文本拼接",
        [this](const QVariantMap& args) { return this->joinText(args); });
    joinTextTool->addParameter("texts", "array", "要拼接的文本数组", true);  // 添加必填参数
    joinTextTool->addParameter("separator", "string", "拼接分隔符", false, ",");  // 添加可选参数
    m_tools.append(joinTextTool);  // 将工具添加到列表
}

// 【功能说明】工具分发执行入口，根据工具名称调用对应的私有实现方法
// 【参数说明】toolName - 要执行的工具名称字符串
// 【参数说明】args - 工具参数字典，键为 QString，值为 QVariant
// 【返回值】QVariantMap 执行结果，包含 success（bool）、result（QString）或 error（QString）
QVariantMap TextAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    // 使用 if-else 链根据 toolName 将请求分派到对应的私有方法
    if (toolName == "base64Encode") {  // Base64 编码
        return base64Encode(args);
    } else if (toolName == "base64Decode") {  // Base64 解码
        return base64Decode(args);
    } else if (toolName == "urlEncode") {  // URL 编码
        return urlEncode(args);
    } else if (toolName == "urlDecode") {  // URL 解码
        return urlDecode(args);
    } else if (toolName == "md5Hash") {  // MD5 哈希
        return md5Hash(args);
    } else if (toolName == "sha256Hash") {  // SHA256 哈希
        return sha256Hash(args);
    } else if (toolName == "charCount") {  // 字符统计
        return charCount(args);
    } else if (toolName == "toUpperCase") {  // 转大写
        return toUpperCase(args);
    } else if (toolName == "toLowerCase") {  // 转小写
        return toLowerCase(args);
    } else if (toolName == "trim") {  // 去除空白
        return trim(args);
    } else if (toolName == "reverseText") {  // 反转文本
        return reverseText(args);
    } else if (toolName == "wordCount") {  // 字数统计
        return wordCount(args);
    } else if (toolName == "sortLines") {  // 按行排序
        return sortLines(args);
    } else if (toolName == "htmlEscape") {  // HTML 转义
        return htmlEscape(args);
    } else if (toolName == "htmlUnescape") {  // HTML 反转义
        return htmlUnescape(args);
    } else if (toolName == "deduplicateLines") {  // 按行去重
        return deduplicateLines(args);
    } else if (toolName == "jsonFormat") {  // JSON 格式化
        return jsonFormat(args);
    } else if (toolName == "xmlFormat") {  // XML 格式化
        return xmlFormat(args);
    } else if (toolName == "extractEmail") {  // 提取邮箱
        return extractEmail(args);
    } else if (toolName == "extractUrl") {  // 提取 URL
        return extractUrl(args);
    } else if (toolName == "replaceText") {  // 文本替换
        return replaceText(args);
    } else if (toolName == "splitText") {  // 文本分割
        return splitText(args);
    } else if (toolName == "joinText") {  // 文本拼接
        return joinText(args);
    }

    // 如果 toolName 不匹配任何已知工具，返回通用错误信息
    QVariantMap result;  // 创建结果字典
    result["success"] = false;  // 标记执行失败
    result["error"] = "Unknown tool: " + toolName;  // 返回错误提示，说明工具名称未知
    return result;  // 返回错误结果
}

// 【功能说明】Base64 编码
// 【参数说明】args - 参数字典，必须包含 "text" 键（要编码的原始文本）
// 【返回值】若成功，result 字段为编码后的 Base64 字符串
QVariantMap TextAgent::base64Encode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取 "text"，转为 QString 类型
    QByteArray encoded = text.toUtf8().toBase64();  // 先将文本转为 UTF-8 字节数组，再编码为 Base64
    result["success"] = true;  // 标记成功
    result["result"] = QString::fromUtf8(encoded);  // 将编码后的字节数组转回 QString 放入结果
    return result;  // 返回成功结果
}

// 【功能说明】Base64 解码
// 【参数说明】args - 参数字典，必须包含 "text" 键（要解码的 Base64 文本）
// 【返回值】若成功，result 字段为解码后的原始字符串；若输入非法，返回错误
QVariantMap TextAgent::base64Decode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要解码的 Base64 文本
    QByteArray decoded = QByteArray::fromBase64(text.toUtf8());  // 将文本转为字节数组后进行 Base64 解码
    // 检查解码结果：如果解码结果为空且输入不为空，说明输入不是合法的 Base64
    if (decoded.isEmpty() && !text.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "Base64解码失败，请检查输入";  // 设置错误信息
        return result;  // 返回错误结果
    }
    result["success"] = true;  // 标记成功
    result["result"] = QString::fromUtf8(decoded);  // 将解码后的字节数组转为 QString
    return result;  // 返回成功结果
}

// 【功能说明】URL 编码（Percent Encoding）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要编码的文本）
// 【返回值】若成功，result 字段为编码后的 URL 字符串（特殊字符转为 %XX 格式）
QVariantMap TextAgent::urlEncode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要编码的文本
    QByteArray encoded = QUrl::toPercentEncoding(text);  // 使用 Qt 的 URL 编码函数将特殊字符编码
    result["success"] = true;  // 标记成功
    result["result"] = QString::fromUtf8(encoded);  // 将编码后的字节数组转为 QString
    return result;  // 返回成功结果
}

// 【功能说明】URL 解码（Percent Decoding）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要解码的 URL 文本）
// 【返回值】若成功，result 字段为解码后的原始字符串
QVariantMap TextAgent::urlDecode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要解码的 URL 文本
    QString decoded = QUrl::fromPercentEncoding(text.toUtf8());  // 使用 Qt 的 URL 解码函数还原 %XX 格式
    result["success"] = true;  // 标记成功
    result["result"] = decoded;  // 将解码后的字符串放入结果
    return result;  // 返回成功结果
}

// 【功能说明】计算 MD5 哈希值
// 【参数说明】args - 参数字典，必须包含 "text" 键（要计算的文本）
// 【返回值】若成功，result 字段为 32 位十六进制 MD5 字符串
QVariantMap TextAgent::md5Hash(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要计算的文本
    // 使用 Qt 加密哈希类计算 MD5：先转 UTF-8 字节数组，再计算哈希，最后转为十六进制字符串
    QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5);
    result["success"] = true;  // 标记成功
    result["result"] = QString::fromUtf8(hash.toHex());  // 将哈希字节数组转为十六进制字符串
    return result;  // 返回成功结果
}

// 【功能说明】计算 SHA256 哈希值
// 【参数说明】args - 参数字典，必须包含 "text" 键（要计算的文本）
// 【返回值】若成功，result 字段为 64 位十六进制 SHA256 字符串
QVariantMap TextAgent::sha256Hash(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要计算的文本
    // 使用 Qt 加密哈希类计算 SHA256：先转 UTF-8 字节数组，再计算哈希，最后转为十六进制字符串
    QByteArray hash = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    result["success"] = true;  // 标记成功
    result["result"] = QString::fromUtf8(hash.toHex());  // 将哈希字节数组转为十六进制字符串
    return result;  // 返回成功结果
}

// 【功能说明】统计字符详细信息（字母、数字、中文、标点、换行、单词等）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要统计的文本）
// 【返回值】若成功，result 字段为包含各类字符数量的统计报告字符串
QVariantMap TextAgent::charCount(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要统计的文本

    int total = text.size();  // 总字符数（QString 的 size() 返回字符数）
    int letters = 0;  // 字母计数器
    int digits = 0;  // 数字计数器
    int spaces = 0;  // 空白符计数器
    int punctuations = 0;  // 标点符号计数器
    int newlines = 0;  // 换行符计数器
    int chinese = 0;  // 中文字符计数器
    int words = 0;  // 单词计数器

    bool inWord = false;  // 标记当前是否处于单词内部（用于单词计数）
    // 遍历文本中的每一个字符
    for (QChar c : text) {
        if (c == '\n') {  // 如果是换行符
            newlines++;  // 换行数加 1
        }
        if (c.isLetter()) {  // 如果是字母（包括中文、英文等）
            letters++;  // 字母数加 1
            // 判断是否为中文汉字（Unicode 脚本为汉文）
            if (c.script() == QChar::Script_Han) {
                chinese++;  // 中文字符数加 1
            }
            // 如果当前不在单词内且字符是字母或数字，表示新单词开始
            if (!inWord && c.isLetterOrNumber()) {
                inWord = true;  // 标记进入单词
                words++;  // 单词数加 1
            }
        } else if (c.isDigit()) {  // 如果是数字
            digits++;  // 数字数加 1
            if (!inWord) {  // 如果当前不在单词内
                inWord = true;  // 标记进入单词（数字也算单词的一部分）
                words++;  // 单词数加 1
            }
        } else if (c.isSpace()) {  // 如果是空白字符（空格、制表符等）
            spaces++;  // 空白符数加 1
            inWord = false;  // 标记离开单词
        } else if (c.isPunct()) {  // 如果是标点符号
            punctuations++;  // 标点符号数加 1
            inWord = false;  // 标记离开单词
        } else {  // 其他字符（如特殊符号）
            inWord = false;  // 标记离开单词
        }
    }

    // 总行数 = 换行符数 + 1（如果文本非空）。例如 "a\nb" 有 1 个换行符，2 行
    int lines = newlines + (text.isEmpty() ? 0 : 1);

    // 使用 QString::arg 格式化统计报告字符串
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

    result["success"] = true;  // 标记成功
    result["result"] = stats;  // 将统计报告放入结果
    return result;  // 返回成功结果
}

// 【功能说明】将文本转换为大写
// 【参数说明】args - 参数字典，必须包含 "text" 键（要转换的文本）
// 【返回值】若成功，result 字段为大写文本
QVariantMap TextAgent::toUpperCase(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要转换的文本
    result["success"] = true;  // 标记成功
    result["result"] = text.toUpper();  // 调用 QString::toUpper() 将所有字母转为大写
    return result;  // 返回成功结果
}

// 【功能说明】将文本转换为小写
// 【参数说明】args - 参数字典，必须包含 "text" 键（要转换的文本）
// 【返回值】若成功，result 字段为小写文本
QVariantMap TextAgent::toLowerCase(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要转换的文本
    result["success"] = true;  // 标记成功
    result["result"] = text.toLower();  // 调用 QString::toLower() 将所有字母转为小写
    return result;  // 返回成功结果
}

// 【功能说明】去除文本首尾的空白字符（空格、制表符、换行等）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要处理的文本）
// 【返回值】若成功，result 字段为去除首尾空白后的文本
QVariantMap TextAgent::trim(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要处理的文本
    result["success"] = true;  // 标记成功
    result["result"] = text.trimmed();  // 调用 QString::trimmed() 去除首尾空白
    return result;  // 返回成功结果
}

// 【功能说明】反转文本（最后一个字符变第一个）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要反转的文本）
// 【返回值】若成功，result 字段为反转后的文本
QVariantMap TextAgent::reverseText(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要反转的文本
    QString reversed;  // 创建空字符串用于存储反转结果
    // 从最后一个字符开始遍历到第一个字符
    for (int i = text.size() - 1; i >= 0; i--) {
        reversed.append(text.at(i));  // 将当前字符追加到结果字符串末尾
    }
    result["success"] = true;  // 标记成功
    result["result"] = reversed;  // 将反转后的文本放入结果
    return result;  // 返回成功结果
}

// 【功能说明】字数统计（按空白分隔计算单词数）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要统计的文本）
// 【返回值】若成功，result 字段为包含字符数、单词数、行数的统计报告
QVariantMap TextAgent::wordCount(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要统计的文本
    int charCount = text.size();  // 统计总字符数
    // 统计行数：空文本为 0 行，否则为换行符数 + 1
    int lineCount = text.isEmpty() ? 0 : text.count('\n') + 1;
    // 使用正则表达式按一个或多个空白字符分割，统计非空片段数即为单词数
    int wordCount = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();

    // 格式化统计报告
    QString stats = QString(
        "字符数: %1\n"
        "单词数: %2\n"
        "行数: %3"
    ).arg(charCount).arg(wordCount).arg(lineCount);

    result["success"] = true;  // 标记成功
    result["result"] = stats;  // 将统计报告放入结果
    return result;  // 返回成功结果
}

// 【功能说明】按行排序文本
// 【参数说明】args - 参数字典，必须包含 "text" 键（要排序的文本）
// 【参数说明】可选 "descending"（bool），true 为降序，false 为升序（默认）
// 【返回值】若成功，result 字段为排序后的文本
QVariantMap TextAgent::sortLines(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要排序的文本
    bool descending = args.value("descending", false).toBool();  // 提取是否降序参数，默认 false

    QStringList lines = text.split('\n');  // 按换行符分割为字符串列表
    std::sort(lines.begin(), lines.end());  // 使用标准库的 sort 对行进行升序排序（按字典序）
    if (descending) {  // 如果要求降序
        std::reverse(lines.begin(), lines.end());  // 反转列表，实现降序
    }

    result["success"] = true;  // 标记成功
    result["result"] = lines.join("\n");  // 用换行符将排序后的行重新拼接为文本
    return result;  // 返回成功结果
}

// 【功能说明】HTML 转义（将 < > " ' & 转为实体），防止 HTML 注入攻击
// 【参数说明】args - 参数字典，必须包含 "text" 键（要转义的文本）
// 【返回值】若成功，result 字段为转义后的 HTML 安全文本
QVariantMap TextAgent::htmlEscape(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要转义的文本
    QString escaped = text;  // 复制原始文本到临时变量
    // 按照 HTML 实体规则逐个替换特殊字符（注意：必须先替换 &，避免重复替换）
    escaped.replace("&", "&amp;");   // 将 & 替换为 &amp;（必须在最前面，防止影响后续替换）
    escaped.replace("<", "&lt;");    // 将 < 替换为 &lt;
    escaped.replace(">", "&gt;");    // 将 > 替换为 &gt;
    escaped.replace("\"", "&quot;"); // 将 " 替换为 &quot;
    escaped.replace("'", "&#39;");   // 将 ' 替换为 &#39;
    result["success"] = true;  // 标记成功
    result["result"] = escaped;  // 将转义后的文本放入结果
    return result;  // 返回成功结果
}

// 【功能说明】HTML 反转义（将实体转回原始字符）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要反转义的文本）
// 【返回值】若成功，result 字段为原始文本
QVariantMap TextAgent::htmlUnescape(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要反转义的文本
    QString unescaped = text;  // 复制原始文本到临时变量
    // 按照 HTML 实体规则逐个还原（注意：必须先还原其他实体，最后还原 &amp;）
    unescaped.replace("&lt;", "<");    // 将 &lt; 还原为 <
    unescaped.replace("&gt;", ">");    // 将 &gt; 还原为 >
    unescaped.replace("&quot;", "\""); // 将 &quot; 还原为 "
    unescaped.replace("&#39;", "'");   // 将 &#39; 还原为 '
    unescaped.replace("&amp;", "&");   // 将 &amp; 还原为 &（必须在最后，避免影响前面的替换）
    result["success"] = true;  // 标记成功
    result["result"] = unescaped;  // 将反转义后的文本放入结果
    return result;  // 返回成功结果
}

// 【功能说明】按行去重（保留第一次出现的行，删除后续重复行）
// 【参数说明】args - 参数字典，必须包含 "text" 键（要去重的文本）
// 【返回值】若成功，result 字段为去重后的文本，并附带统计信息
QVariantMap TextAgent::deduplicateLines(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要去重的文本
    QStringList lines = text.split('\n');  // 按换行符分割为字符串列表
    QSet<QString> seen;  // 创建集合用于记录已经出现过的行（集合自动去重）
    QStringList uniqueLines;  // 创建列表存储去重后的行（保持原始顺序）

    // 遍历每一行
    for (const QString& line : lines) {
        if (!seen.contains(line)) {  // 如果该行还未出现过
            seen.insert(line);  // 将行加入集合，标记为已出现
            uniqueLines.append(line);  // 将行加入去重后的列表
        }
    }

    int removed = lines.size() - uniqueLines.size();  // 计算被移除的重复行数
    result["success"] = true;  // 标记成功
    // 格式化结果字符串，包含去重统计和去重后的文本内容
    result["result"] = QString("去除了 %1 行重复，剩余 %2 行：\n\n%3")
        .arg(removed).arg(uniqueLines.size()).arg(uniqueLines.join("\n"));
    return result;  // 返回成功结果
}

// 【功能说明】JSON 格式化（将压缩的 JSON 转为可读格式，带缩进和换行）
// 【参数说明】args - 参数字典，必须包含 "text" 键（JSON 文本）
// 【返回值】若成功，result 字段为格式化后的 JSON 字符串；若解析失败，返回错误
QVariantMap TextAgent::jsonFormat(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取 JSON 文本
    QJsonParseError error;  // 创建 JSON 解析错误对象，用于接收解析错误信息
    // 将文本转为 UTF-8 字节数组并解析为 JSON 文档
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &error);

    // 检查解析是否出错
    if (error.error != QJsonParseError::NoError) {
        result["success"] = false;  // 标记失败
        result["error"] = "JSON解析失败: " + error.errorString();  // 返回具体的解析错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    // 将 JSON 文档以缩进格式（Indented）输出为 UTF-8 字节数组，再转为 QString
    result["result"] = QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    return result;  // 返回成功结果
}

// 【功能说明】XML 格式化（添加缩进和换行，使压缩的 XML 变得可读）
// 【参数说明】args - 参数字典，必须包含 "text" 键（XML 文本）
// 【返回值】若成功，result 字段为格式化后的 XML 字符串
QVariantMap TextAgent::xmlFormat(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取 XML 文本
    QString formatted;  // 存储格式化后的结果
    int indent = 0;  // 当前缩进层级计数器
    bool inComment = false;  // 标记当前是否在 XML 注释（<!-- ... -->）内部

    // 逐个字符遍历 XML 文本
    for (int i = 0; i < text.size(); i++) {
        QChar c = text.at(i);  // 获取当前字符

        if (c == '<') {  // 如果遇到左尖括号（标签开始）
            // 检查是否是注释开始 <!--
            if (i + 3 < text.size() && text.mid(i, 4) == "<!--") {
                inComment = true;  // 标记进入注释
            } else if (i + 2 < text.size() && text.mid(i, 3) == "-->") {
                inComment = false;  // 标记退出注释
            }

            if (!inComment) {  // 如果不在注释内
                // 如果前一个字符不是空白，先换行并添加当前层级的缩进
                if (i > 0 && !text.at(i-1).isSpace()) {
                    formatted += "\n";  // 添加换行
                    formatted += QString("    ").repeated(indent);  // 添加 4 空格 × 缩进层级
                }
                // 如果下一个是 /，说明是闭合标签，减少缩进层级
                if (i + 1 < text.size() && text.at(i+1) == '/') {
                    indent--;
                }
            }
            formatted += c;  // 添加左尖括号
        } else if (c == '>') {  // 如果遇到右尖括号（标签结束）
            formatted += c;  // 添加右尖括号
            // 如果不在注释内，且下一个是普通文本（非标签开头、非空白），先换行缩进
            if (!inComment && i + 1 < text.size() && text.at(i+1) != '<' && !text.at(i+1).isSpace()) {
                formatted += "\n";  // 添加换行
                formatted += QString("    ").repeated(indent);  // 添加缩进
            }
            // 如果不在注释内，且前一个字符不是 /（自闭合标签），且不是 />，则增加缩进
            if (!inComment && i > 1 && text.at(i-1) != '/' && text.mid(i-2, 2) != "/>") {
                bool isClosing = false;  // 标记是否为闭合标签
                // 向前查找对应的左尖括号，判断是否为 </xxx> 格式
                for (int j = i - 1; j >= 0; j--) {
                    if (text.at(j) == '<') {  // 找到左尖括号
                        if (j + 1 < text.size() && text.at(j+1) == '/') {
                            isClosing = true;  // 是闭合标签
                        }
                        break;  // 跳出查找循环
                    }
                }
                if (!isClosing) {  // 如果不是闭合标签（即开标签）
                    indent++;  // 增加缩进层级
                }
            }
        } else {
            formatted += c;  // 普通字符直接添加
        }
    }

    result["success"] = true;  // 标记成功
    result["result"] = formatted.trimmed();  // 去除首尾空白后返回格式化结果
    return result;  // 返回成功结果
}

// 【功能说明】从文本中提取邮箱地址
// 【参数说明】args - 参数字典，必须包含 "text" 键（要搜索的文本）
// 【返回值】若成功，result 字段为提取到的邮箱列表；若未找到，返回提示
QVariantMap TextAgent::extractEmail(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要搜索的文本
    // 定义邮箱正则表达式：用户名@域名.顶级域名
    QRegularExpression re("[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}");
    QRegularExpressionMatchIterator it = re.globalMatch(text);  // 在文本中全局搜索所有匹配
    QStringList emails;  // 存储找到的邮箱地址

    // 遍历所有匹配结果
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();  // 获取下一个匹配
        emails.append(match.captured());  // 将匹配的完整邮箱字符串添加到列表
    }

    result["success"] = true;  // 标记成功
    // 如果未找到邮箱，返回提示；否则返回邮箱数量和列表
    result["result"] = emails.isEmpty() ? "未找到邮箱地址" : QString("找到 %1 个邮箱:\n%2").arg(emails.size()).arg(emails.join("\n"));
    return result;  // 返回成功结果
}

// 【功能说明】从文本中提取 URL 链接
// 【参数说明】args - 参数字典，必须包含 "text" 键（要搜索的文本）
// 【返回值】若成功，result 字段为提取到的 URL 列表；若未找到，返回提示
QVariantMap TextAgent::extractUrl(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 从参数中提取要搜索的文本
    // 定义 URL 正则表达式：http:// 或 https:// 开头，后跟域名和可选路径
    QRegularExpression re("https?://[a-zA-Z0-9.-]+(?:/[^\\s]*)?");
    QRegularExpressionMatchIterator it = re.globalMatch(text);  // 在文本中全局搜索所有匹配
    QStringList urls;  // 存储找到的 URL

    // 遍历所有匹配结果
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();  // 获取下一个匹配
        urls.append(match.captured());  // 将匹配的完整 URL 字符串添加到列表
    }

    result["success"] = true;  // 标记成功
    // 如果未找到 URL，返回提示；否则返回 URL 数量和列表
    result["result"] = urls.isEmpty() ? "未找到URL链接" : QString("找到 %1 个URL:\n%2").arg(urls.size()).arg(urls.join("\n"));
    return result;  // 返回成功结果
}

// 【功能说明】文本替换（批量替换指定内容）
// 【参数说明】args - 参数字典，包含 text（原始文本）、oldText（要替换的内容）、newText（替换后的内容）
// 【参数说明】可选 "caseSensitive"（bool），true 区分大小写（默认），false 不区分
// 【返回值】若成功，result 字段为替换后的文本
QVariantMap TextAgent::replaceText(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 提取原始文本
    QString oldText = args.value("oldText").toString();  // 提取要替换的内容
    QString newText = args.value("newText").toString();  // 提取替换后的内容
    bool caseSensitive = args.value("caseSensitive", true).toBool();  // 提取大小写敏感选项，默认 true

    QString replaced = text;  // 复制原始文本到临时变量
    // 根据 caseSensitive 选择大小写敏感模式
    Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    replaced.replace(oldText, newText, cs);  // 执行批量替换

    result["success"] = true;  // 标记成功
    result["result"] = replaced;  // 将替换后的文本放入结果
    return result;  // 返回成功结果
}

// 【功能说明】文本分割（按分隔符拆分为多行）
// 【参数说明】args - 参数字典，包含 text（要分割的文本）
// 【参数说明】可选 "delimiter"（string），分隔符，默认逗号 ","
// 【返回值】若成功，result 字段为分割后的多行文本，附带分割数量
QVariantMap TextAgent::splitText(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString text = args.value("text").toString();  // 提取要分割的文本
    QString delimiter = args.value("delimiter", ",").toString();  // 提取分隔符，默认逗号

    // 按分隔符分割文本，保留空片段（KeepEmptyParts）
    QStringList parts = text.split(delimiter, Qt::KeepEmptyParts);

    result["success"] = true;  // 标记成功
    // 返回分割数量和各部分列表
    result["result"] = QString("分割为 %1 部分:\n%2").arg(parts.size()).arg(parts.join("\n"));
    return result;  // 返回成功结果
}

// 【功能说明】文本拼接（将数组用分隔符连接为一个字符串）
// 【参数说明】args - 参数字典，包含 texts（文本数组）
// 【参数说明】可选 "separator"（string），拼接分隔符，默认逗号 ","
// 【返回值】若成功，result 字段为拼接后的字符串
QVariantMap TextAgent::joinText(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QVariantList texts = args.value("texts").toList();  // 提取要拼接的文本数组（QVariantList 类型）
    QString separator = args.value("separator", ",").toString();  // 提取拼接分隔符，默认逗号

    QStringList textList;  // 创建 QStringList 用于存储转换后的字符串
    // 遍历 QVariantList，将每个元素转为 QString
    for (const QVariant& v : texts) {
        textList.append(v.toString());  // 将 QVariant 转为 QString 并添加到列表
    }

    result["success"] = true;  // 标记成功
    result["result"] = textList.join(separator);  // 使用指定分隔符拼接列表中的字符串
    return result;  // 返回成功结果
}
