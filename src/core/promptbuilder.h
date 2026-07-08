#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QString>
#include <QList>

class Agent;

/**
 * @brief Prompt构建器 - 负责构建发给大模型的提示词
 *
 * 这个类的作用是把用户的问题、Agent的信息、工具的描述等
 * 按照一定的格式组装成大模型能理解的Prompt文本。
 *
 * 高质量的Prompt是大模型正确工作的关键，就像给人布置任务时，
 * 任务描述得越清楚，完成质量就越高。
 */
class PromptBuilder
{
public:
    /**
     * @brief 构造函数
     */
    PromptBuilder();

    /**
     * @brief 析构函数
     */
    ~PromptBuilder();

    /**
     * @brief 构建系统提示词
     *
     * 系统提示词用于设定AI的角色和行为规则，
     * 告诉它有哪些工具可用，以及如何使用这些工具。
     *
     * @param role AI扮演的角色描述
     * @param agents 可用的Agent列表
     * @return 格式化后的系统提示词
     */
    QString buildSystemPrompt(const QString& role, const QList<Agent*>& agents);

    /**
     * @brief 构建用户提示词
     * @param userQuery 用户的问题或指令
     * @return 格式化后的用户提示词
     */
    QString buildUserPrompt(const QString& userQuery);

    /**
     * @brief 构建工具执行结果提示词
     *
     * 当工具执行完成后，把结果告诉大模型，
     * 让它根据结果继续思考下一步该做什么。
     *
     * @param toolName 工具名称
     * @param result 工具执行结果
     * @return 格式化后的工具结果提示词
     */
    QString buildToolResultPrompt(const QString& toolName, const QString& result);

    /**
     * @brief 构建完整的初始提示词
     * @param userQuery 用户的问题
     * @param role AI角色
     * @param agents 可用Agent列表
     * @return 完整的Prompt文本
     */
    QString buildFullPrompt(const QString& userQuery, const QString& role, const QList<Agent*>& agents);

private:
    QString m_toolCallFormat; ///< 工具调用的格式说明
};

#endif
