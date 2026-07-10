/**
 * @file agent.h
 * @brief Agent 抽象基类 —— 所有智能体的统一接口
 *
 * @details
 * Agent 是 Multi-Agent 系统的核心抽象，定义了所有智能体必须实现的接口。
 * 每个 Agent 代表一个具有特定能力的"智能助手"，例如：
 * - ComputerAgent：文件操作、命令执行
 * - CodeAgent：代码生成、代码分析
 * - Orchestrator：总控协调（也继承自 Agent）
 *
 * Agent 基类的设计遵循"接口隔离"原则：
 * - name() / description() / type()：身份标识
 * - tools()：能力清单（每个 Agent 可以拥有多个 Tool）
 * - executeTool()：执行指定工具
 *
 * 通过继承 Agent 并实现这些纯虚函数，可以无限扩展系统的能力，
 * 添加新的 Agent 类型（如 DatabaseAgent、WebAgent 等）。
 *
 * AgentType 枚举区分不同类型的 Agent，用于 UI 展示和逻辑分支。
 */

#ifndef AGENT_H  // 【条件编译】防止头文件被重复包含
#define AGENT_H  // 【宏定义】标记该文件已被包含

#include <QObject>     // 【引入】Qt对象基类
#include <QString>     // 【引入】Qt字符串类
#include <QList>       // 【引入】Qt列表容器
#include <QVariantMap> // 【引入】Qt变体映射类，用于键值对参数和结果

// 前向声明：避免循环包含
class Tool;  // 【前向声明】Tool类

/**
 * @brief Agent 类型枚举
 *
 * 用于区分不同角色的 Agent，便于 UI 层展示不同的图标、颜色等。
 * 未来可扩展更多类型（如 DatabaseAgent、WebAgent、ApiAgent 等）。
 */
enum AgentType {  // 【枚举】Agent类型标识
    OrchestratorType,  // 【枚举值】总控协调器，负责生成计划和调度
    ComputerType,      // 【枚举值】通用计算机Agent（保留兼容）
    CodeType,          // 【枚举值】通用代码Agent（保留兼容）
    FileAgentType,     // 【枚举值】文件操作Agent
    ComputerAgentType, // 【枚举值】系统命令Agent
    CodeAgentType,     // 【枚举值】代码开发Agent
    SearchAgentType,   // 【枚举值】搜索Agent
    TextAgentType      // 【枚举值】文本处理Agent
};

/**
 * @brief Agent 抽象基类
 *
 * Agent 继承自 QObject，因此：
 * - 可以使用 Qt 的信号槽机制
 * - 可以加入 Qt 对象树进行自动内存管理
 * - 可以使用 Qt 的元对象反射功能
 *
 * @note 这是一个抽象类（含纯虚函数），不能直接实例化。
 *       必须通过子类化并实现所有纯虚函数才能使用。
 */
class Agent : public QObject  // 【类声明】Agent抽象基类，继承QObject
{
    Q_OBJECT  // 【Qt宏】启用元对象系统，支持信号槽

public:
    /**
     * @brief 构造函数
     * @param parent 父 QObject，用于 Qt 内存管理
     */
    explicit Agent(QObject *parent = nullptr);  // 【构造函数】explicit防止隐式转换

    /**
     * @brief 析构函数
     *
     * @note 声明为 virtual，确保通过基类指针删除派生类对象时能正确析构。
     */
    virtual ~Agent();  // 【虚析构函数】支持多态析构

    /**
     * @brief 获取 Agent 名称
     * @return Agent 的唯一标识名称
     *
     * @note 名称用于：
     * - UI 展示（任务列表中的 Agent 列）
     * - PromptBuilder 生成工具描述
     * - 调试日志中的标识
     */
    virtual QString name() const = 0;  // 【纯虚方法】返回Agent名称，子类必须实现

    /**
     * @brief 获取 Agent 描述
     * @return Agent 的功能描述文本
     *
     * @note 描述用于：
     * - UI 展示（鼠标悬停提示）
     * - PromptBuilder 生成系统提示词中的 Agent 说明
     */
    virtual QString description() const = 0;  // 【纯虚方法】返回Agent描述，子类必须实现

    /**
     * @brief 获取 Agent 类型
     * @return AgentType 枚举值
     *
     * @note 类型用于：
     * - UI 层选择不同的图标和颜色
     * - 逻辑分支判断（如 OrchestratorType 有特殊处理）
     */
    virtual AgentType type() const = 0;  // 【纯虚方法】返回Agent类型，子类必须实现

    /**
     * @brief 获取 Agent 拥有的所有工具
     * @return Tool 指针列表
     *
     * @note 每个 Agent 可以拥有多个 Tool，代表它的各种能力。
     *       Orchestrator 通过汇总所有 Agent 的 tools() 来构建完整的工具清单。
     */
    virtual QList<Tool*> tools() const = 0;  // 【纯虚方法】返回工具列表，子类必须实现

    /**
     * @brief 执行指定工具
     * @param toolName 要执行的工具名称
     * @param args 工具参数（键值对）
     * @return 执行结果（至少包含 "success" 键）
     *
     * @implementation
     * 子类实现此方法时，通常使用 if-else 或 switch 根据 toolName 分发到具体的工具实现。
     *
     * @note 返回值必须包含 "success" 键（bool 类型）：
     *       - true 表示执行成功，可选包含 "result" 键存放结果
     *       - false 表示执行失败，必须包含 "error" 键存放错误信息
     */
    virtual QVariantMap executeTool(const QString& toolName, const QVariantMap& args) = 0;  // 【纯虚方法】执行工具，子类必须实现

signals:  // 【Qt关键字】信号声明
    /**
     * @brief Agent 产生输出
     * @param output 输出文本
     *
     * 用于长时间运行的工具实时反馈执行进度。
     * 例如：编译大项目时的逐行输出、文件复制进度等。
     */
    void output(const QString& output);  // 【信号】实时输出执行内容

    /**
     * @brief Agent 执行出错
     * @param error 错误描述
     */
    void error(const QString& error);  // 【信号】通知执行出错
};

#endif // AGENT_H  // 【条件编译结束】
