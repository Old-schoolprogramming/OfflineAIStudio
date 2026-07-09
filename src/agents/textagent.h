#ifndef TEXTAGENT_H
#define TEXTAGENT_H

#include "agent.h"
#include <QList>

class Tool;

/**
 * @brief 文本处理Agent - 专门负责各种文本处理操作
 *
 * TextAgent是一个文本处理专家，它可以：
 * - Base64编码/解码
 * - URL编码/解码
 * - MD5/SHA256哈希计算
 * - 文本字符统计
 * - 大小写转换
 * - 去除空白字符
 * - 文本反转
 * - 字数统计
 * - 行排序
 * - JSON格式化
 * - HTML转义/反转义
 *
 * 当大模型判断用户的问题涉及文本处理时，
 * 就会调用TextAgent的工具来完成任务。
 */
class TextAgent : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit TextAgent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TextAgent();

    /**
     * @brief 获取Agent名称
     * @return "TextAgent"
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 文本处理Agent的功能描述
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return TextAgentType
     */
    AgentType type() const override;

    /**
     * @brief 获取所有工具
     * @return 工具列表
     */
    QList<Tool*> tools() const override;

    /**
     * @brief 执行指定工具
     * @param toolName 工具名称
     * @param args 工具参数
     * @return 执行结果
     */
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    /**
     * @brief 初始化所有工具
     */
    void initializeTools();

    QList<Tool*> m_tools; ///< 工具列表

    /**
     * @brief Base64编码
     * @param args 包含text参数
     * @return Base64编码结果
     */
    QVariantMap base64Encode(const QVariantMap& args);

    /**
     * @brief Base64解码
     * @param args 包含text参数
     * @return Base64解码结果
     */
    QVariantMap base64Decode(const QVariantMap& args);

    /**
     * @brief URL编码
     * @param args 包含text参数
     * @return URL编码结果
     */
    QVariantMap urlEncode(const QVariantMap& args);

    /**
     * @brief URL解码
     * @param args 包含text参数
     * @return URL解码结果
     */
    QVariantMap urlDecode(const QVariantMap& args);

    /**
     * @brief 计算MD5哈希
     * @param args 包含text参数
     * @return MD5哈希值
     */
    QVariantMap md5Hash(const QVariantMap& args);

    /**
     * @brief 计算SHA256哈希
     * @param args 包含text参数
     * @return SHA256哈希值
     */
    QVariantMap sha256Hash(const QVariantMap& args);

    /**
     * @brief 文本字符统计
     * @param args 包含text参数
     * @return 字符统计结果
     */
    QVariantMap charCount(const QVariantMap& args);

    /**
     * @brief 转换为大写
     * @param args 包含text参数
     * @return 大写文本
     */
    QVariantMap toUpperCase(const QVariantMap& args);

    /**
     * @brief 转换为小写
     * @param args 包含text参数
     * @return 小写文本
     */
    QVariantMap toLowerCase(const QVariantMap& args);

    /**
     * @brief 去除首尾空白
     * @param args 包含text参数
     * @return 去除空白后的文本
     */
    QVariantMap trim(const QVariantMap& args);

    /**
     * @brief 反转文本
     * @param args 包含text参数
     * @return 反转后的文本
     */
    QVariantMap reverseText(const QVariantMap& args);

    /**
     * @brief 字数统计
     * @param args 包含text参数
     * @return 字数统计结果
     */
    QVariantMap wordCount(const QVariantMap& args);

    /**
     * @brief 行排序
     * @param args 包含text、descending参数
     * @return 排序后的文本
     */
    QVariantMap sortLines(const QVariantMap& args);

    /**
     * @brief HTML转义
     * @param args 包含text参数
     * @return 转义后的HTML文本
     */
    QVariantMap htmlEscape(const QVariantMap& args);

    /**
     * @brief HTML反转义
     * @param args 包含text参数
     * @return 反转义后的文本
     */
    QVariantMap htmlUnescape(const QVariantMap& args);

    /**
     * @brief 文本去重（按行）
     * @param args 包含text参数
     * @return 去重后的文本
     */
    QVariantMap deduplicateLines(const QVariantMap& args);
};

#endif
