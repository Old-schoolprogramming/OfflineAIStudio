#ifndef CODEAGENT_H
#define CODEAGENT_H

#include "agent.h"
#include <QList>

class Tool;

/**
 * @brief 代码处理Agent - 专门负责代码相关的操作
 *
 * CodeAgent是一个代码处理专家，它可以：
 * - 统计代码行数（支持多种语言）
 * - 查找代码中的符号（函数、类、变量等）
 * - 检测文件编码格式
 * - 转换文件编码
 * - 统计代码注释比例
 * - 格式化代码（简单格式化）
 * - 查找代码中的TODO/FIXME标记
 * - 比较代码差异
 *
 * 当大模型判断用户的问题涉及代码处理时，
 * 就会调用CodeAgent的工具来完成任务。
 */
class CodeAgent : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CodeAgent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CodeAgent();

    /**
     * @brief 获取Agent名称
     * @return "CodeAgent"
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 代码处理Agent的功能描述
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return CodeAgentType
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
     *
     * 在构造函数中调用，创建所有这个Agent拥有的工具。
     */
    void initializeTools();

    QList<Tool*> m_tools; ///< 工具列表

    /**
     * @brief 统计代码行数
     * @param args 包含path、recursive参数
     * @return 代码行数统计结果
     */
    QVariantMap countLines(const QVariantMap& args);

    /**
     * @brief 查找代码中的函数定义
     * @param args 包含path、pattern参数
     * @return 找到的函数列表
     */
    QVariantMap findFunctions(const QVariantMap& args);

    /**
     * @brief 查找代码中的TODO/FIXME标记
     * @param args 包含path、recursive参数
     * @return 找到的TODO/FIXME列表
     */
    QVariantMap findTodos(const QVariantMap& args);

    /**
     * @brief 检测文件编码
     * @param args 包含path参数
     * @return 文件编码信息
     */
    QVariantMap detectEncoding(const QVariantMap& args);

    /**
     * @brief 统计代码注释比例
     * @param args 包含path参数
     * @return 注释比例统计
     */
    QVariantMap commentRatio(const QVariantMap& args);

    /**
     * @brief 查找代码中的类定义
     * @param args 包含path参数
     * @return 找到的类列表
     */
    QVariantMap findClasses(const QVariantMap& args);

    /**
     * @brief 移除代码中的空白行
     * @param args 包含path、outputPath参数
     * @return 处理结果
     */
    QVariantMap removeBlankLines(const QVariantMap& args);

    /**
     * @brief 统计文件中各种字符出现次数
     * @param args 包含path参数
     * @return 字符统计结果
     */
    QVariantMap charStats(const QVariantMap& args);
};

#endif
