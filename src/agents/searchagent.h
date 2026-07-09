#ifndef SEARCHAGENT_H
#define SEARCHAGENT_H

#include "agent.h"
#include <QList>

class Tool;

/**
 * @brief 搜索Agent - 专门负责文件内容和文件名的搜索替换
 *
 * SearchAgent是一个搜索专家，它可以：
 * - 在文件内容中搜索指定文本
 * - 使用正则表达式进行高级搜索
 * - 按文件名模式搜索文件
 * - 批量替换文件中的文本
 * - 统计搜索结果出现次数
 * - 在指定目录下递归搜索
 *
 * 当大模型判断用户的问题涉及搜索替换时，
 * 就会调用SearchAgent的工具来完成任务。
 */
class SearchAgent : public Agent
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit SearchAgent(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SearchAgent();

    /**
     * @brief 获取Agent名称
     * @return "SearchAgent"
     */
    QString name() const override;

    /**
     * @brief 获取Agent描述
     * @return 搜索Agent的功能描述
     */
    QString description() const override;

    /**
     * @brief 获取Agent类型
     * @return SearchAgentType
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
     * @brief 在文件内容中搜索文本
     * @param args 包含path、pattern、recursive、caseSensitive参数
     * @return 搜索结果
     */
    QVariantMap searchInFiles(const QVariantMap& args);

    /**
     * @brief 使用正则表达式搜索
     * @param args 包含path、pattern、recursive参数
     * @return 搜索结果
     */
    QVariantMap regexSearch(const QVariantMap& args);

    /**
     * @brief 替换文件中的文本
     * @param args 包含path、search、replace、recursive参数
     * @return 替换结果
     */
    QVariantMap replaceInFiles(const QVariantMap& args);

    /**
     * @brief 按文件名搜索文件
     * @param args 包含dir、pattern、recursive参数
     * @return 匹配的文件列表
     */
    QVariantMap searchByName(const QVariantMap& args);

    /**
     * @brief 统计文本在文件中出现的次数
     * @param args 包含path、pattern、recursive参数
     * @return 统计结果
     */
    QVariantMap countOccurrences(const QVariantMap& args);

    /**
     * @brief 查找包含所有指定关键词的文件
     * @param args 包含dir、keywords、recursive参数
     * @return 匹配的文件列表
     */
    QVariantMap findFilesWithKeywords(const QVariantMap& args);
};

#endif
