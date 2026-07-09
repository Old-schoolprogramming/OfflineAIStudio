/**
 * @file tool.cpp
 * @brief 工具类实现
 *
 * @details
 * Tool类封装了Agent可调用的具体工具/函数的描述信息。
 * 每个Tool包含名称、描述以及一组参数定义，
 * 并支持将自身格式化为LLM提示词中可用的文本描述。
 */

#include "tool.h"

/**
 * @brief 构造函数
 * @param name 工具名称，用于在提示词和JSON中标识该工具
 * @param description 工具功能描述，帮助LLM理解何时使用此工具
 *
 * 初始化工具的基本信息，参数列表初始为空。
 */
Tool::Tool(const QString& name, const QString& description)
    : m_name(name), m_description(description)
{
}

/**
 * @brief 析构函数
 */
Tool::~Tool()
{
}

/**
 * @brief 获取工具名称
 * @return 工具标识字符串
 */
QString Tool::name() const
{
    return m_name;
}

/**
 * @brief 获取工具描述
 * @return 工具功能描述字符串
 */
QString Tool::description() const
{
    return m_description;
}

/**
 * @brief 添加参数定义
 * @param name 参数名称
 * @param type 参数类型（如"string", "int", "bool"）
 * @param description 参数用途描述
 * @param required 是否为必填参数
 * @param defaultValue 可选参数的默认值（空字符串表示无默认值）
 *
 * @details
 * 构造Parameter结构体并追加到内部参数列表中。
 * 参数顺序决定了在提示词中的展示顺序。
 */
void Tool::addParameter(const QString& name, const QString& type, const QString& description, bool required, const QString& defaultValue)
{
    Parameter param;
    param.name = name;
    param.type = type;
    param.description = description;
    param.required = required;
    param.defaultValue = defaultValue;
    m_parameters.append(param);
}

/**
 * @brief 获取所有参数定义
 * @return 参数列表的副本
 */
QList<Parameter> Tool::parameters() const
{
    return m_parameters;
}

/**
 * @brief 将工具格式化为提示词文本
 * @return 格式化后的工具描述字符串
 *
 * @details
 * 生成适合嵌入LLM系统提示词的工具说明，格式如下：
 * @code
 * - toolName: toolDescription
 *   Parameters:
 *     - paramName (type) [required]: paramDescription (default: xxx)
 * @endcode
 *
 * 必填参数标记为[required]，可选参数若有默认值则一并显示。
 */
QString Tool::formatForPrompt() const
{
    QString result = "- " + m_name + ": " + m_description + "\n";
    result += "  Parameters:\n";
    
    // 遍历所有参数，生成带类型、必填标记和默认值的描述行
    for (const Parameter& param : m_parameters) {
        QString requiredStr = param.required ? "[required]" : "[optional]";
        QString defaultStr = !param.defaultValue.isEmpty() ? " (default: " + param.defaultValue + ")" : "";
        result += "    - " + param.name + " (" + param.type + ") " + requiredStr + ": " + param.description + defaultStr + "\n";
    }
    
    return result;
}
