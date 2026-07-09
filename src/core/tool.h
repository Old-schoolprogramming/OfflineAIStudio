#ifndef TOOL_H
#define TOOL_H

#include <QString>
#include <QVariantMap>
#include <QList>

/**
 * @brief 工具参数结构体 - 描述一个工具的参数信息
 */
struct Parameter {
    QString name;           ///< 参数名称
    QString type;           ///< 参数类型（如 string, int, bool 等）
    QString description;    ///< 参数描述说明
    bool required;          ///< 是否为必填参数
    QString defaultValue;   ///< 参数默认值（可选）
};

/**
 * @brief 工具类 - 代表Agent可以调用的一个具体功能
 *
 * 每个工具都有自己的名称、描述和参数列表。
 * 大模型通过理解工具的描述来决定何时调用哪个工具。
 */
class Tool
{
public:
    /**
     * @brief 构造函数
     * @param name 工具名称
     * @param description 工具功能描述
     */
    Tool(const QString& name, const QString& description);

    /**
     * @brief 析构函数
     */
    ~Tool();

    /**
     * @brief 获取工具名称
     * @return 工具名称
     */
    QString name() const;

    /**
     * @brief 获取工具描述
     * @return 工具描述
     */
    QString description() const;

    /**
     * @brief 添加一个参数到工具
     * @param name 参数名称
     * @param type 参数类型
     * @param description 参数描述
     * @param required 是否必填
     * @param defaultValue 默认值
     */
    void addParameter(const QString& name, const QString& type, const QString& description, bool required = true, const QString& defaultValue = "");

    /**
     * @brief 获取所有参数
     * @return 参数列表
     */
    QList<Parameter> parameters() const;

    /**
     * @brief 将工具信息格式化为Prompt文本
     * @return 格式化后的工具描述，用于输入给大模型
     */
    QString formatForPrompt() const;

private:
    QString m_name;             ///< 工具名称
    QString m_description;      ///< 工具描述
    QList<Parameter> m_parameters; ///< 参数列表
};

#endif
