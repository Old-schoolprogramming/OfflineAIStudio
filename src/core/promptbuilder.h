/**
 * @file promptbuilder.h
 * @brief 系统提示词构建器 —— 控制大模型的"使用说明书"
 *
 * @details
 * PromptBuilder 负责为不同阶段构建适合大模型的提示词（Prompt）。
 * 它是连接"人类需求"与"机器指令"的桥梁，提示词的质量直接影响大模型的输出质量。
 *
 * 核心功能：
 * 1. buildPlanningSystemPrompt() —— 规划阶段系统提示词
 *    向大模型解释：你是谁、你有啥工具、怎么返回 JSON 计划
 * 2. buildPlanningUserPrompt() —— 规划阶段用户提示词
 *    告诉大模型：用户的请求是啥、基于什么上下文
 * 3. buildToolDefinitionPrompt() —— 工具定义 Prompt（当前未使用）
 *    为每个工具生成包含名称、描述、参数 Schema 的文本描述
 *
 * 设计原则：
 * - 提示词越清晰、越结构化，大模型的输出就越可控
 * - 强制 JSON 格式输出，方便程序解析
 * - 包含"所有可用工具"的完整描述，大模型才能做出正确的工具选择
 * - 大量示例（few-shot learning）降低 7B 模型的理解难度
 */

#ifndef PROMPTBUILDER_H  // 【条件编译】防止头文件被重复包含
#define PROMPTBUILDER_H  // 【宏定义】标记该文件已被包含

#include <QString>     // 【引入】Qt字符串类
#include <QList>       // 【引入】Qt列表容器

// 前向声明：避免在头文件中包含 agent.h 的完整定义，减少编译依赖
class Agent;           // 【前向声明】Agent类，此处只需要指针类型
class Tool;            // 【前向声明】Tool类

/**
 * @brief 系统提示词构建器
 *
 * PromptBuilder 是纯工具类（无 QObject 继承），不管理任何状态，
 * 所有方法都是 const 的，可多线程安全使用。
 */
class PromptBuilder  // 【类声明】纯工具类，无需继承QObject
{
public:
    /**
     * @brief 构造函数
     *
     * 无参构造函数，PromptBuilder 不需要初始化任何状态。
     */
    PromptBuilder();  // 【构造函数】创建PromptBuilder实例

    /**
     * @brief 构建规划阶段的系统提示词（System Prompt）
     * @param agents 所有可用的 Agent 列表
     * @return 完整的系统提示词字符串
     *
     * @details
     * 这是最关键的系统提示词，它告诉大模型：
     * 1. 你是谁（Multi-Agent System Orchestrator）
     * 2. 你能做什么（各 Agent 的工具能力）
     * 3. 怎么做事（严格按 plan → tool 的顺序）
     * 4. 输出格式要求（JSON 数组）
     * 5. 大量示例（降低 7B 模型的理解门槛）
     *
     * @note 生成 Prompt 的逻辑是"原样"复制的，所以文档内也保留了这份 Prompt 全文，
     *       方便开发者和 LLM 理解其工作原理。
     */
    QString buildPlanningSystemPrompt(const QList<Agent*>& agents) const;  // 【方法】构建规划系统提示词

    /**
     * @brief 构建规划阶段的用户提示词（User Prompt）
     * @param userInput 用户的自然语言输入
     * @return 包装后的用户提示词
     *
     * @implementation
     * 简单包装用户输入，添加格式说明。
     * 未来可以在此添加上下文信息、历史记录等。
     */
    QString buildPlanningUserPrompt(const QString& userInput) const;  // 【方法】构建规划用户提示词

    /**
     * @brief 构建工具定义 Prompt
     * @param agents 所有可用的 Agent 列表
     * @return 所有工具的定义文本
     *
     * @implementation
     * 遍历所有 Agent 的所有工具，为每个工具调用 tool->promptDescription()，
     * 用换行符连接所有描述。
     *
     * @note 当前该方法在 buildPlanningSystemPrompt() 中未直接使用，
     *       而是内联了类似的工具描述生成逻辑。保留此方法为未来重构预留。
     */
    QString buildToolDefinitionPrompt(const QList<Agent*>& agents) const;  // 【方法】构建工具定义提示词
};

#endif // PROMPTBUILDER_H  // 【条件编译结束】
