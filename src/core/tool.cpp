/**
 * @file tool.cpp
 * @brief 工具类实现
 *
 * @details
 * Tool 类的具体实现，使用 std::function 作为执行器，
 * 避免为每个工具创建大量子类。
 */

#include "tool.h"  // 【引入】自己的头文件

/**
 * @brief 构造函数
 * @param name 工具名称
 * @param description 工具功能描述
 * @param executor 执行器
 */
Tool::Tool(const QString& name, const QString& description, Executor executor)
    : m_name(name)              // 【初始化】工具名称
    , m_description(description) // 【初始化】工具描述
    , m_executor(executor)       // 【初始化】执行器
{
    // 【说明】基类构造函数体为空
}

/**
 * @brief 析构函数
 */
Tool::~Tool()
{
    // 【说明】基类析构函数体为空
    // std::function 会自动释放其持有的资源
}

/**
 * @brief 获取工具名称
 */
QString Tool::name() const
{
    return m_name;  // 【返回】存储的工具名称
}

/**
 * @brief 获取工具描述
 */
QString Tool::description() const
{
    return m_description;  // 【返回】存储的工具描述
}

/**
 * @brief 获取工具的 Prompt 描述
 *
 * @implementation
 * 格式："name: description"
 * 如果有参数，则附加："参数：name1(type, 必填/可选, 默认值) - desc1; ..."
 */
QString Tool::promptDescription() const
{
    QString result = m_name + ": " + m_description;  // 【构造】基础描述

    if (!m_parameters.isEmpty()) {  // 【判断】如果存在参数
        result += "，参数：";  // 【添加】参数标识
        QStringList paramDescs;  // 【创建】参数描述列表
        for (const Parameter& param : m_parameters) {  // 【遍历】每个参数
            QString part = param.name + "(" + param.type;  // 【构造】name(type
            if (param.required) {  // 【判断】是否必填
                part += ", 必填";  // 【添加】必填标识
            } else {  // 【分支】可选参数
                part += ", 可选";  // 【添加】可选标识
                if (!param.defaultValue.isEmpty()) {  // 【判断】有默认值
                    part += ", 默认" + param.defaultValue;  // 【添加】默认值
                }
            }
            part += ") - " + param.description;  // 【添加】参数描述
            paramDescs.append(part);  // 【追加】到列表
        }
        result += paramDescs.join("; ");  // 【拼接】用分号分隔
    }

    return result;  // 【返回】完整 Prompt 描述
}

/**
 * @brief 执行工具
 * @param args 工具参数
 * @return 执行结果
 *
 * @implementation
 * 直接调用存储的 executor 函数对象。
 * 如果 executor 为空，返回错误信息。
 */
QVariantMap Tool::execute(const QVariantMap& args)
{
    if (!m_executor) {  // 【判断】执行器是否为空
        QVariantMap result;  // 【创建】结果字典
        result["success"] = false;  // 【标记】执行失败
        result["error"] = "Tool '" + m_name + "' has no executor";  // 【设置】错误信息
        return result;  // 【返回】错误结果
    }
    return m_executor(args);  // 【调用】存储的执行器
}

/**
 * @brief 添加参数
 */
void Tool::addParameter(const QString& name, const QString& type, const QString& desc,
                        bool required, const QString& defaultValue)
{
    Parameter param;  // 【创建】参数结构
    param.name = name;             // 【设置】参数名称
    param.type = type;             // 【设置】参数类型
    param.description = desc;      // 【设置】参数描述
    param.required = required;     // 【设置】是否必填
    param.defaultValue = defaultValue;  // 【设置】默认值
    m_parameters.append(param);    // 【追加】到参数列表
}

/**
 * @brief 构建参数描述字符串
 *
 * 生成格式："name(type) - desc"
 */
QString Tool::buildParamDesc(const QString& name, const QString& type, const QString& desc) const
{
    return QString("%1(%2) - %3").arg(name, type, desc);  // 【返回】格式化描述
}
