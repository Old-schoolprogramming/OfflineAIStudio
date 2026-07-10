/**
 * @file outputpanel.h
 * @brief 执行输出面板头文件 —— 展示 Multi-Agent 执行全过程的日志
 *
 * @details
 * OutputPanel 是系统的核心输出组件，展示从用户输入到最终结果的完整执行链路：
 * - 用户发送的消息
 * - AI 生成的计划信息
 * - 每个步骤的执行头信息（步骤编号、Agent、工具、参数）
 * - 每个步骤的实际输出
 * - 每个步骤的成功/失败结果
 * - 执行总结
 * - 错误信息
 *
 * 输出使用富文本（HTML）渲染，支持：
 * - 步骤头的渐变色卡片
 * - 成功/失败的状态标记（带颜色背景）
 * - 等宽字体代码块风格
 * - 时间戳
 *
 * 工具栏提供两个操作：
 * - 复制：将当前所有输出复制到剪贴板
 * - 清空：清除所有输出内容
 *
 * 与 Orchestrator 的交互：
 *   Orchestrator 发射 messageReceived / stepOutput / stepStarted 等信号
 *   → MainWindow 接收 → 调用 OutputPanel 的 append 方法
 */

#ifndef OUTPUTPANEL_H  // 【条件编译保护】防止头文件被重复包含；若未定义OUTPUTPANEL_H宏才编译以下内容
#define OUTPUTPANEL_H  // 【定义保护宏】标记此文件已被包含过一次，避免重定义错误

#include <QWidget>    // 【引入Qt基础控件类】QWidget是所有UI控件的基类，提供显示、布局、事件处理等基础能力
#include <QTextEdit>  // 【引入富文本编辑器类】QTextEdit用于显示和编辑多行富文本（HTML格式），这里是日志展示区
#include <QVBoxLayout>// 【引入垂直布局类】QVBoxLayout将子控件沿垂直方向排列；用于标题工具栏在上、输出区在下的布局结构
#include <QLabel>     // 【引入标签类】QLabel用于显示静态文本；用作"执行输出"标题
#include <QToolBar>   // 【引入工具栏类】QToolBar提供可放置多个操作按钮的横条容器；用于放置"复制"和"清空"按钮
#include <QAction>    // 【引入动作类】QAction代表一个用户操作（如菜单项、工具栏按钮），可关联图标、快捷键和槽函数

/**
 * @brief 执行输出面板类 - 展示Multi-Agent系统执行过程的日志输出
 *
 * @note OutputPanel 内部使用 QTextEdit（只读模式）展示内容。
 *       所有追加操作都会自动滚动到底部，确保最新内容可见。
 *
 * 设计特点：
 * 1. 采用深色主题风格（背景#0f172a，文字#e2e8f0），与主窗口协调
 * 2. 使用等宽字体（Consolas/Monaco）展示输出，便于阅读代码和命令行结果
 * 3. 提供工具栏快速操作（复制、清空），提升用户体验
 * 4. 支持HTML富文本渲染，可显示带颜色的状态标记和格式化文本
 */
class OutputPanel : public QWidget  // 【类声明】OutputPanel继承QWidget，成为一个独立的日志展示面板
{
    Q_OBJECT  // 【Qt元对象宏】启用信号与槽、运行时类型信息等Qt核心机制；必须放在类声明的私有区域首行

public:  // 【公有接口区】外部可访问的构造函数、析构函数和日志追加方法

    /**
     * @brief 构造函数
     * @param parent 父 QWidget；决定面板的嵌入位置和生命周期
     *
     * 内部调用 setupUI() 创建和布局所有子控件（标题标签、工具栏、输出显示区），
     * 并应用自定义QSS样式表（深色主题、等宽字体、圆角边框）。
     */
    explicit OutputPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * Qt父子对象机制自动释放所有子控件，无需手动delete。
     */
    ~OutputPanel();

    /**
     * @brief 追加普通文本输出
     * @param text 要追加的文本（支持 HTML 标签进行富文本渲染）
     *
     * @details
     * 使用 QTextEdit::append() 将文本追加到输出区末尾，然后自动滚动到底部。
     * 文本可预格式化为 HTML，以支持颜色、字体、加粗等富文本效果。
     *
     * 此方法用于追加用户消息、AI 回复、错误提示等通用内容。
     * 调用后光标自动跳转到文档末尾，确保用户始终看到最新输出。
     */
    void appendOutput(const QString& text);

    /**
     * @brief 追加步骤执行头信息
     * @param stepId 步骤编号（如1、2、3...），标识当前执行到第几步
     * @param description 步骤描述文本（如"读取文件内容"、"执行代码分析"）
     *
     * @details
     * 生成一个带时间戳的步骤头行，格式为"[HH:mm:ss] 步骤 N — 描述"。
     * 用于标识一个Agent步骤的开始，在输出区形成清晰的段落分隔。
     * 与appendStepOutput和appendStepResult配合使用，形成完整的步骤日志记录。
     */
    void appendStepHeader(int stepId, const QString& description);

    /**
     * @brief 追加步骤执行输出
     * @param stepId 步骤编号（当前未在输出中使用，保留供未来扩展如步骤关联过滤）
     * @param output Agent 返回的输出文本（可能包含多行、代码、JSON等内容）
     *
     * @details
     * 将输出文本进行以下处理后追加到面板：
     * 1. 将换行符替换为带缩进的换行，使多行输出在视觉上形成缩进块
     * 2. 整体添加4空格缩进，与步骤头形成层级关系
     *
     * 使用等宽字体（Consolas / Monaco）展示，便于阅读代码和命令输出。
     */
    void appendStepOutput(int stepId, const QString& output);

    /**
     * @brief 追加步骤执行结果
     * @param stepId 步骤编号（当前未在输出中使用，保留供未来扩展）
     * @param success true 表示步骤执行成功，false 表示执行失败
     * @param message 结果描述文本（成功时的输出摘要或失败时的错误信息）
     *
     * @details
     * 生成一个状态标记行：
     * - 成功时：[成功] + 消息内容
     * - 失败时：[失败] + 消息内容
     * 然后追加一个空行，与下一个步骤形成视觉分隔。
     */
    void appendStepResult(int stepId, bool success, const QString& message);

    /**
     * @brief 清空所有输出内容
     *
     * 调用QTextEdit::clear()删除输出显示区中的所有文本。
     * 效果：面板瞬间变为空白状态，适用于开始新任务前清理旧日志。
     */
    void clearOutput();

signals:  // 【信号声明区】用户操作通知，实现面板与外部逻辑的解耦

    /**
     * @brief 用户请求清空输出
     *
     * @note 当前未连接此信号，保留供未来扩展（如主窗口记录清空操作到日志文件）。
     */
    void clearRequested();

    /**
     * @brief 用户请求复制输出
     *
     * @note 当前未连接此信号，保留供未来扩展（如复制成功后显示提示消息）。
     */
    void copyRequested();

private slots:  // 【私有槽函数区】处理工具栏按钮点击事件

    /**
     * @brief 清空操作处理槽函数
     *
     * @implementation
     * 1. 调用 clearOutput() 清除输出显示区的所有内容
     * 2. 发射 clearRequested() 信号，通知外部（如主窗口）清空操作已执行
     */
    void onClearAction();

    /**
     * @brief 复制操作处理槽函数
     *
     * @implementation
     * 1. 调用m_outputDisplay->toPlainText()获取输出区的纯文本内容
     * 2. 使用QApplication::clipboard()->setText()将内容写入系统剪贴板
     * 3. 发射 copyRequested() 信号，通知外部复制操作已执行
     */
    void onCopyAction();

private:  // 【私有成员区】内部UI控件和辅助方法

    /**
     * @brief 创建和布局所有 UI 子控件
     *
     * @implementation
     * 创建以下子控件并组织布局：
     * 1. 头部区域（headerWidget）：水平布局，包含标题标签和工具栏
     *    - m_titleLabel — "执行输出"标题，14px白色加粗文字
     *    - m_toolBar    — 操作工具栏，放置"复制"和"清空"两个QAction按钮
     * 2. m_outputDisplay — 只读 QTextEdit，占据剩余空间，展示所有输出内容
     *
     * 整体布局：垂直布局，头部区域在上（固定高度），输出区域在下（拉伸填充）。
     */
    void setupUI();

    QTextEdit*  m_outputDisplay;  // 【输出显示区指针】只读QTextEdit实例；使用等宽字体展示日志内容；支持HTML富文本渲染；自动滚动到底部
    QLabel*     m_titleLabel;     // 【标题标签指针】显示"执行输出"文字；14px字号、白色(#f1f5f9)、font-weight:600（半加粗）
    QToolBar*   m_toolBar;        // 【工具栏指针】容纳复制和清空两个操作按钮；背景透明，按钮带悬停效果
    QAction*    m_clearAction;    // 【清空动作指针】代表"清空"操作；点击后触发onClearAction槽函数；在工具栏上显示为文本按钮"清空"
    QAction*    m_copyAction;     // 【复制动作指针】代表"复制"操作；点击后触发onCopyAction槽函数；在工具栏上显示为文本按钮"复制"
    QVBoxLayout* m_mainLayout;    // 【主布局指针】QVBoxLayout实例，垂直排列headerWidget和m_outputDisplay；边距12px，间距8px
};

#endif // OUTPUTPANEL_H  // 【结束条件编译】对应开头的#ifndef
