/**
 * @file tool.h
 * @brief 工具类 —— Agent 能力的原子单元
 *
 * @details
 * Tool 是 Agent 能力的原子化表示。每个 Tool 代表一个具体的、可执行的功能，例如：
 * - readFile：读取文件内容
 * - writeFile：写入文件
 * - runCommand：执行系统命令
 * - generateCode：生成代码
 *
 * Tool 的设计遵循"单一职责原则"：每个 Tool 只做一件事，做好一件事。
 * Agent 通过组合多个 Tool 来构建复杂的能力。
 *
 * Tool 的核心价值在于 promptDescription() 方法：
 * 它生成一段自然语言描述，告诉大模型这个工具的名称、功能、参数要求。
 * 这些描述被 PromptBuilder 收集并插入到系统提示词中，
 * 使大模型能够"了解"每个工具的用途，从而在规划时做出正确选择。
 */

#ifndef TOOL_H  // 【条件编译】防止头文件被重复包含
#define TOOL_H  // 【宏定义】标记该文件已被包含

#include <QString>      // 【引入】Qt字符串类
#include <QVariantMap>  // 【引入】Qt变体映射类
#include <QJsonObject>  // 【引入】Qt JSON对象类
#include <QList>        // 【引入】Qt列表容器
#include <functional>   // 【引入】std::function 支持

/**
 * @brief 工具类
 *
 * Tool 是一个具体类（可直接 new），封装了工具的元数据和执行逻辑。
 * 每个 Tool 实例包含：
 * - name：工具唯一标识
 * - description：工具功能说明
 * - parameters：参数列表（用于生成 Prompt 描述和参数校验）
 * - executor：实际执行的函数对象（std::function）
 *
 * @note Tool 的生命周期由创建它的 Agent 管理。
 */
class Tool  // 【类声明】工具类
{
public:
    /**
     * @brief 工具参数描述结构
     *
     * 描述一个工具参数的元数据：名称、类型、描述、是否必填、默认值。
     */
    struct Parameter {
        QString name;          ///< 参数名称
        QString type;          ///< 参数类型（如 string、int、bool、array）
        QString description;   ///< 参数功能描述
        bool required;         ///< 是否必填参数
        QString defaultValue;  ///< 默认值（仅当 required=false 时有效）
    };

    /**
     * @brief 执行器类型定义
     *
     * 一个可调用对象，接收 QVariantMap 参数，返回 QVariantMap 结果。
     * 通过 std::function 绑定到具体 Agent 的成员方法。
     */
    using Executor = std::function<QVariantMap(const QVariantMap&)>;

    /**
     * @brief 构造函数
     * @param name 工具名称
     * @param description 工具功能描述
     * @param executor 执行器（lambda 绑定到 Agent 的成员方法）
     */
    Tool(const QString& name, const QString& description, Executor executor);

    /**
     * @brief 析构函数
     */
    virtual ~Tool();

    /**
     * @brief 获取工具名称
     * @return 工具的唯一标识名称
     */
    QString name() const;

    /**
     * @brief 获取工具描述
     * @return 工具的功能描述
     */
    QString description() const;

    /**
     * @brief 获取工具的 Prompt 描述
     * @return 适合插入系统提示词的文本描述
     *
     * @implementation
     * 生成格式：
     *   "name: description, 参数：name1(type, 必填) - desc1; name2(type, 可选, 默认default) - desc2"
     */
    QString promptDescription() const;

    /**
     * @brief 执行工具
     * @param args 工具参数（键值对）
     * @return 执行结果（至少包含 "success" 键）
     */
    QVariantMap execute(const QVariantMap& args);

    /**
     * @brief 添加参数
     * @param name 参数名称
     * @param type 参数类型
     * @param desc 参数功能描述
     * @param required 是否必填，默认为 true
     * @param defaultValue 默认值（仅 required=false 时有效），默认为空
     */
    void addParameter(const QString& name, const QString& type, const QString& desc,
                      bool required = true, const QString& defaultValue = QString());

protected:  // 【访问修饰符】受保护成员，子类可访问
    /**
     * @brief 构建参数描述的辅助方法
     * @param name 参数名称
     * @param type 参数类型
     * @param desc 参数功能描述
     * @return 格式化的参数描述字符串
     */
    QString buildParamDesc(const QString& name, const QString& type, const QString& desc) const;

private:  // 【访问修饰符】私有成员
    QString m_name;            ///< 工具名称
    QString m_description;     ///< 工具描述
    QList<Parameter> m_parameters;  ///< 参数列表
    Executor m_executor;       ///< 执行器
};

#endif // TOOL_H  // 【条件编译结束】
