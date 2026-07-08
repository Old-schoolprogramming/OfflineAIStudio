/**
 * @file outputpanel.h
 * @brief 执行输出面板 —— 展示 Multi-Agent 执行全过程的日志
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

#ifndef OUTPUTPANEL_H
#define OUTPUTPANEL_H

#include <QWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolBar>
#include <QAction>

/**
 * @brief 执行输出面板
 *
 * @note OutputPanel 内部使用 QTextEdit（只读模式）展示内容。
 *       所有追加操作都会自动滚动到底部，确保最新内容可见。
 */
class OutputPanel : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父 QWidget
     *
     * 调用 setupUI() 创建和布局所有子控件，应用自定义样式表。
     */
    explicit OutputPanel(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~OutputPanel();

    /**
     * @brief 追加普通文本输出
     * @param text 要追加的文本（支持 HTML）
     *
     * @details
     * 使用 QTextEdit::append() 追加文本，然后自动滚动到底部。
     * 文本应预格式化为 HTML，以支持富文本渲染。
     *
     * 此方法用于追加用户消息、AI 回复、错误提示等通用内容。
     */
    void appendOutput(const QString& text);

    /**
     * @brief 追加步骤执行头信息
     * @param stepId 步骤编号
     * @param description 步骤描述
     *
     * @details
     * 生成一个带渐变色背景和边框的步骤头卡片，包含：
     * - 步骤编号和描述
     * - 当前时间戳
     *
     * 样式：紫色渐变背景 + 靛蓝色边框 + 圆角
     */
    void appendStepHeader(int stepId, const QString& description);

    /**
     * @brief 追加步骤执行输出
     * @param stepId 步骤编号（当前未使用，保留供未来扩展）
     * @param output Agent 返回的输出文本
     *
     * @details
     * 将输出文本转义为 HTML 安全字符串（防止 XSS），
     * 将换行符替换为 `<br>`，然后追加到输出面板。
     *
     * 使用等宽字体（Consolas / Monaco）展示，便于阅读代码和命令输出。
     */
    void appendStepOutput(int stepId, const QString& output);

    /**
     * @brief 追加步骤执行结果
     * @param stepId 步骤编号（当前未使用，保留供未来扩展）
     * @param success true 表示成功，false 表示失败
     * @param message 结果描述（成功时的输出摘要或失败时的错误信息）
     *
     * @details
     * 生成一个状态标记卡片：
     * - 成功：绿色背景 + ✓ 标记 + "成功"
     * - 失败：红色背景 + ✗ 标记 + "失败"
     */
    void appendStepResult(int stepId, bool success, const QString& message);

    /**
     * @brief 清空所有输出内容
     */
    void clearOutput();

signals:
    /**
     * @brief 用户请求清空输出
     *
     * @note 当前未连接此信号，保留供未来扩展。
     */
    void clearRequested();

    /**
     * @brief 用户请求复制输出
     *
     * @note 当前未连接此信号，保留供未来扩展。
     */
    void copyRequested();

private slots:
    /**
     * @brief 清空操作处理
     *
     * @implementation
     * 调用 clearOutput() 清除内容，发射 clearRequested 信号。
     */
    void onClearAction();

    /**
     * @brief 复制操作处理
     *
     * @implementation
     * 将 QTextEdit 的纯文本内容复制到系统剪贴板，发射 copyRequested 信号。
     */
    void onCopyAction();

private:
    /**
     * @brief 创建和布局所有 UI 子控件
     *
     * @implementation
     * 创建以下子控件：
     * 1. 头部区域（headerWidget）：包含标题标签和工具栏
     *    - m_titleLabel — "执行输出"标题
     *    - m_toolBar    — 工具栏（复制/清空按钮）
     * 2. m_outputDisplay — 只读 QTextEdit，展示所有输出内容
     *
     * 布局结构：垂直布局，头部在上，输出区域占据剩余空间。
     */
    void setupUI();

    QTextEdit*  m_outputDisplay;  ///< 输出显示区域（只读 QTextEdit）
    QLabel*     m_titleLabel;     ///< "执行输出"标题标签
    QToolBar*   m_toolBar;        ///< 操作工具栏
    QAction*    m_clearAction;    ///< 清空动作
    QAction*    m_copyAction;     ///< 复制动作
    QVBoxLayout* m_mainLayout;    ///< 主垂直布局
};

#endif // OUTPUTPANEL_H
