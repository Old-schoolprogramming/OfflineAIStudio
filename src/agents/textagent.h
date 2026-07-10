// 【文件说明】textagent.h - 文本处理Agent的头文件
// 【作用】声明 TextAgent 类，定义文本编解码、哈希计算、格式化等功能的接口
// 【注意事项】头文件只声明类和函数，具体实现在 textagent.cpp 中

#ifndef TEXTAGENT_H  // 防止重复包含的宏定义开始
#define TEXTAGENT_H  // 定义 TEXTAGENT_H 宏

#include "agent.h"  // 引入 Agent 基类头文件，TextAgent 继承自 Agent
#include <QList>    // 引入 Qt 列表容器，用于存储工具对象列表

class Tool;  // 前向声明 Tool 类，告诉编译器有这个类，减少编译依赖

/**
 * @brief 文本处理Agent - 负责文本编解码、哈希计算、格式化等操作
 *
 * TextAgent 提供丰富的文本处理工具，包括：
 * - Base64 编码/解码
 * - URL 编码/解码
 * - MD5 / SHA256 哈希计算
 * - 大小写转换、去除空白、反转文本
 * - 字符统计、字数统计、行排序、去重
 * - HTML 转义/反转义
 * - JSON / XML 格式化
 * - 邮箱和 URL 提取
 * - 文本替换、分割、拼接
 */
class TextAgent : public Agent  // TextAgent 继承自 Agent 基类
{
    Q_OBJECT  // Qt 宏，启用信号与槽、元对象系统等 Qt 特性

public:
    // 【功能说明】构造函数，创建文本处理代理实例
    // 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
    explicit TextAgent(QObject *parent = nullptr);

    // 【功能说明】析构函数，销毁文本处理代理实例
    // 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象
    ~TextAgent();

    // 【功能说明】获取代理名称
    // 【返回值】固定返回 QString 字符串 "TextAgent"
    // 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
    QString name() const override;

    // 【功能说明】获取代理描述
    // 【返回值】返回描述此 Agent 功能的 QString 字符串
    // 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
    QString description() const override;

    // 【功能说明】获取代理类型
    // 【返回值】返回枚举值 TextAgentType
    // 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
    AgentType type() const override;

    // 【功能说明】获取所有工具
    // 【返回值】返回 QList<Tool*> 类型的工具指针列表
    // 【说明】重写父类虚函数，返回此 Agent 注册的所有可用工具
    QList<Tool*> tools() const override;

    // 【功能说明】执行指定工具
    // 【参数说明】toolName - 要执行的工具名称字符串
    // 【参数说明】args - 工具参数字典，键值对形式
    // 【返回值】执行结果字典，包含 success（是否成功）和 result/error（结果或错误信息）
    // 【说明】重写父类虚函数，根据工具名称分发到对应的私有实现方法
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    // 【功能说明】初始化所有文本处理工具
    // 【说明】在构造函数中调用，创建所有这个Agent拥有的工具并注册到 m_tools 列表中
    void initializeTools();

    QList<Tool*> m_tools;  ///< 工具列表成员变量，存储此 Agent 拥有的所有 Tool 对象指针

    // 【功能说明】Base64 编码
    // 【参数说明】args - 包含 text（要编码的文本）
    // 【返回值】编码后的 Base64 字符串
    QVariantMap base64Encode(const QVariantMap& args);

    // 【功能说明】Base64 解码
    // 【参数说明】args - 包含 text（要解码的 Base64 文本）
    // 【返回值】解码后的原始字符串
    QVariantMap base64Decode(const QVariantMap& args);

    // 【功能说明】URL 编码（Percent Encoding）
    // 【参数说明】args - 包含 text（要编码的文本）
    // 【返回值】编码后的 URL 字符串
    QVariantMap urlEncode(const QVariantMap& args);

    // 【功能说明】URL 解码（Percent Decoding）
    // 【参数说明】args - 包含 text（要解码的 URL 文本）
    // 【返回值】解码后的原始字符串
    QVariantMap urlDecode(const QVariantMap& args);

    // 【功能说明】计算 MD5 哈希值
    // 【参数说明】args - 包含 text（要计算的文本）
    // 【返回值】32 位十六进制 MD5 字符串
    QVariantMap md5Hash(const QVariantMap& args);

    // 【功能说明】计算 SHA256 哈希值
    // 【参数说明】args - 包含 text（要计算的文本）
    // 【返回值】64 位十六进制 SHA256 字符串
    QVariantMap sha256Hash(const QVariantMap& args);

    // 【功能说明】统计字符详细信息（字母、数字、中文、标点、换行等）
    // 【参数说明】args - 包含 text（要统计的文本）
    // 【返回值】字符统计结果字符串
    QVariantMap charCount(const QVariantMap& args);

    // 【功能说明】将文本转换为大写
    // 【参数说明】args - 包含 text（要转换的文本）
    // 【返回值】大写文本
    QVariantMap toUpperCase(const QVariantMap& args);

    // 【功能说明】将文本转换为小写
    // 【参数说明】args - 包含 text（要转换的文本）
    // 【返回值】小写文本
    QVariantMap toLowerCase(const QVariantMap& args);

    // 【功能说明】去除文本首尾的空白字符（空格、制表符、换行等）
    // 【参数说明】args - 包含 text（要处理的文本）
    // 【返回值】去除空白后的文本
    QVariantMap trim(const QVariantMap& args);

    // 【功能说明】反转文本（最后一个字符变第一个）
    // 【参数说明】args - 包含 text（要反转的文本）
    // 【返回值】反转后的文本
    QVariantMap reverseText(const QVariantMap& args);

    // 【功能说明】字数统计（按空白分隔计算单词数）
    // 【参数说明】args - 包含 text（要统计的文本）
    // 【返回值】字数统计结果字符串
    QVariantMap wordCount(const QVariantMap& args);

    // 【功能说明】按行排序文本
    // 【参数说明】args - 包含 text（要排序的文本）、descending（是否降序，默认 false）
    // 【返回值】排序后的文本
    QVariantMap sortLines(const QVariantMap& args);

    // 【功能说明】HTML 转义（将 < > " ' & 转为实体）
    // 【参数说明】args - 包含 text（要转义的文本）
    // 【返回值】转义后的 HTML 安全文本
    QVariantMap htmlEscape(const QVariantMap& args);

    // 【功能说明】HTML 反转义（将实体转回原始字符）
    // 【参数说明】args - 包含 text（要反转义的文本）
    // 【返回值】原始文本
    QVariantMap htmlUnescape(const QVariantMap& args);

    // 【功能说明】按行去重（保留第一次出现的行）
    // 【参数说明】args - 包含 text（要去重的文本）
    // 【返回值】去重后的文本，附带统计信息
    QVariantMap deduplicateLines(const QVariantMap& args);

    // 【功能说明】JSON 格式化（将压缩的 JSON 转为可读格式）
    // 【参数说明】args - 包含 text（JSON 文本）
    // 【返回值】格式化后的 JSON 字符串
    QVariantMap jsonFormat(const QVariantMap& args);

    // 【功能说明】XML 格式化（添加缩进和换行）
    // 【参数说明】args - 包含 text（XML 文本）
    // 【返回值】格式化后的 XML 字符串
    QVariantMap xmlFormat(const QVariantMap& args);

    // 【功能说明】从文本中提取邮箱地址
    // 【参数说明】args - 包含 text（要搜索的文本）
    // 【返回值】提取到的邮箱列表字符串
    QVariantMap extractEmail(const QVariantMap& args);

    // 【功能说明】从文本中提取 URL 链接
    // 【参数说明】args - 包含 text（要搜索的文本）
    // 【返回值】提取到的 URL 列表字符串
    QVariantMap extractUrl(const QVariantMap& args);

    // 【功能说明】文本替换
    // 【参数说明】args - 包含 text（原始文本）、oldText（要替换的内容）、newText（替换后的内容）、caseSensitive（是否区分大小写，默认 true）
    // 【返回值】替换后的文本
    QVariantMap replaceText(const QVariantMap& args);

    // 【功能说明】文本分割（按分隔符拆分为多行）
    // 【参数说明】args - 包含 text（要分割的文本）、delimiter（分隔符，默认逗号）
    // 【返回值】分割后的多行文本
    QVariantMap splitText(const QVariantMap& args);

    // 【功能说明】文本拼接（将数组用分隔符连接）
    // 【参数说明】args - 包含 texts（文本数组）、separator（拼接分隔符，默认逗号）
    // 【返回值】拼接后的字符串
    QVariantMap joinText(const QVariantMap& args);
};

#endif  // TEXTAGENT_H  // 宏定义结束，与开头的 #ifndef 配对
