/**
 * @file promptbuilder.h
 * @brief Prompt 构建器 —— 构造发给大模型的提示词
 *
 * @details
 * PromptBuilder 负责将系统的运行时信息（Agent 列表、工具描述、用户输入）
 * 按照预设模板组装成大模型能理解的 Prompt 文本。
 *
 * 本系统使用两种 Prompt 模式：
 *
 * 模式一 —— 普通对话模式（v1.0 遗留，兼容保留）：
 *   用于简单问答场景，不需要工具调用。
 *   buildSystemPrompt() + buildUserPrompt() + buildToolResultPrompt()
 *
 * 模式二 —— 规划模式（v2.0 新增，核心模式）：
 *   用于生成 JSON 执行计划。
 *   buildPlanningSystemPrompt() + buildPlanningUserPrompt()
 *
 * 高质量的 Prompt 是大模型正确工作的关键。
 * 规划模式下的系统提示词需要精确描述 JSON 格式、字段含义、生成规则，
 * 使 LLM 能够输出可被 Planner 正确解析的结构化数据。
 */

#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QString>
#include <QList>

class Agent;

/**
 * @brief Prompt 构建器
 *
 * PromptBuilder 是无状态类，每次调用根据传入的参数动态构建 Prompt。
 * 所有方法都是纯函数，不依赖内部状态（除格式模板字符串外）。
 */
class PromptBuilder
{
public:
    /**
     * @brief 构造函数
     *
     * 初始化两种模式的格式模板字符串。
     */
    PromptBuilder();

    /**
     * @brief 析构函数
     */
    ~PromptBuilder();

    // ==================== 普通对话模式（兼容保留） ====================

    /**
     * @brief 构建普通对话的系统提示词
     * @param role AI 扮演的角色描述
     * @param agents 可用的 Agent 列表
     * @return 系统提示词文本
     *
     * @note 此模式用于 v1.0 的 ReAct 风格交互，v2.0 中主要使用规划模式。
     */
    QString buildSystemPrompt(const QString& role, const QList<Agent*>& agents);

    /**
     * @brief 构建普通对话的用户提示词
     * @param userQuery 用户的问题或指令
     * @return 用户提示词文本
     */
    QString buildUserPrompt(const QString& userQuery);

    /**
     * @brief 构建工具执行结果提示词
     * @param toolName 工具名称
     * @param result 工具执行结果
     * @return 结果提示词文本
     *
     * @note 用于 v1.0 的 ReAct 循环中，将工具结果反馈给 LLM 继续推理。
     */
    QString buildToolResultPrompt(const QString& toolName, const QString& result);

    /**
     * @brief 构建完整的普通对话 Prompt
     * @param userQuery 用户问题
     * @param role AI 角色
     * @param agents 可用 Agent 列表
     * @return 完整 Prompt
     */
    QString buildFullPrompt(const QString& userQuery, const QString& role, const QList<Agent*>& agents);

    // ==================== 规划模式（v2.0 核心） ====================

    /**
     * @brief 构建规划模式的系统提示词
     * @param agents 可用的 Agent 列表
     * @return 规划专用系统提示词
     *
     * @details
     * 系统提示词包含以下内容：
     * 1. 角色定义：告诉 LLM 它是一个任务规划专家
     * 2. Agent/工具清单：列出所有可用 Agent 及其工具的详细描述
     * 3. JSON 格式模板：给出精确的 JSON 输出格式示例
     * 4. 生成规则：分步、顺序、参数映射、中文描述等要求
     * 5. 降级说明：如果不需要工具，直接回答而非输出 JSON
     *
     * 这个提示词的质量直接决定了 Planner 能否成功解析 LLM 的输出。
     */
    QString buildPlanningSystemPrompt(const QList<Agent*>& agents);

    /**
     * @brief 构建规划模式的用户提示词
     * @param userQuery 用户的原始输入
     * @return 用户提示词文本
     *
     * @implementation
     * 简单的格式化："User request: [query]\n\nPlease create an execution plan for this task."
     */
    QString buildPlanningUserPrompt(const QString& userQuery);

private:
    QString m_toolCallFormat;  ///< v1.0 工具调用格式模板（如 <tool(args)>）
    QString m_jsonPlanFormat;  ///< v2.0 JSON 计划格式模板（包含 goal 和 steps 结构）
};

#endif // PROMPTBUILDER_H
