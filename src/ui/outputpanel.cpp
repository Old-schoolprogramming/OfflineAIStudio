/**
 * @file outputpanel.cpp
 * @brief 执行输出面板实现
 *
 * @details
 * OutputPanel 基于 QTextEdit（只读模式）实现，所有内容以 HTML 富文本形式追加。
 *
 * 核心设计要点：
 * 1. HTML 安全：所有用户输入和 Agent 输出都经过 toHtmlEscaped() 转义，防止 XSS
 * 2. 自动滚动：每次追加后移动 QTextCursor 到末尾，确保最新内容可见
 * 3. 分类展示：提供四种 append 方法，分别对应不同类型的输出内容
 * 4. 工具栏：提供复制和清空操作，方便用户管理输出内容
 *
 * HTML 样式设计：
 * - 步骤头：紫色渐变背景卡片，带边框和时间戳
 * - 步骤输出：等宽字体，浅灰色文字
 * - 成功结果：绿色半透明背景 + ✓ 标记
 * - 失败结果：红色半透明背景 + ✗ 标记
 * - 用户消息：蓝色头像标识
 * - AI 消息：绿色头像标识
 * - 错误信息：红色背景卡片
 */

#include "outputpanel.h"
#include <QDateTime>
#include <QApplication>
#include <QClipboard>

/**
 * @brief 构造函数
 * @param parent 父 QWidget
 *
 * @implementation
 * 调用 setupUI() 完成所有子控件的创建和布局。
 */
OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

/**
 * @brief 析构函数
 */
OutputPanel::~OutputPanel()
{
}

/**
 * @brief 创建和布局所有 UI 子控件
 *
 * @implementation
 * 控件层次结构：
 * m_mainLayout (QVBoxLayout)
 *   ├── headerWidget (QWidget, QHBoxLayout)
 *   │   ├── m_titleLabel — "执行输出"标题
 *   │   └── m_toolBar    — 工具栏（复制/清空按钮）
 *   └── m_outputDisplay  — 只读 QTextEdit（占据剩余空间）
 *
 * 样式设计：
 * - 输出区域：深色背景 #0f172a，等宽字体，圆角边框
 * - 工具栏按钮：透明背景 + 边框，悬停变色
 */
void OutputPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(8);

    // 头部区域：标题 + 工具栏
    QWidget* headerWidget = new QWidget(this);
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_titleLabel = new QLabel("执行输出", headerWidget);
    m_titleLabel->setStyleSheet(
        "color: #f1f5f9;"
        "font-size: 16px;"
        "font-weight: 600;"
    );
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();  // 将工具栏推到右侧

    // 工具栏
    m_toolBar = new QToolBar(headerWidget);
    m_toolBar->setIconSize(QSize(16, 16));
    m_toolBar->setStyleSheet(
        "QToolBar {"
        "   background: transparent;"
        "   spacing: 4px;"
        "}"
        "QToolButton {"
        "   background-color: #1e293b;"
        "   color: #94a3b8;"
        "   border: 1px solid #334155;"
        "   border-radius: 6px;"
        "   padding: 4px 10px;"
        "   font-size: 12px;"
        "}"
        "QToolButton:hover {"
        "   background-color: #334155;"
        "   color: #f1f5f9;"
        "   border-color: #475569;"
        "}"
        "QToolButton:pressed {"
        "   background-color: #475569;"
        "}"
    );

    m_copyAction = m_toolBar->addAction("复制");
    m_clearAction = m_toolBar->addAction("清空");

    connect(m_copyAction, &QAction::triggered, this, &OutputPanel::onCopyAction);
    connect(m_clearAction, &QAction::triggered, this, &OutputPanel::onClearAction);

    headerLayout->addWidget(m_toolBar);
    m_mainLayout->addWidget(headerWidget);

    // 输出显示区域
    m_outputDisplay = new QTextEdit(this);
    m_outputDisplay->setReadOnly(true);
    m_outputDisplay->setObjectName("outputDisplay");
    m_outputDisplay->setStyleSheet(
        "QTextEdit {"
        "   background-color: #0f172a;"
        "   color: #e2e8f0;"
        "   border: 1px solid #334155;"
        "   border-radius: 8px;"
        "   padding: 12px;"
        "   font-family: 'Consolas', 'Monaco', 'Courier New', monospace;"
        "   font-size: 13px;"
        "   line-height: 1.6;"
        "}"
    );
    m_mainLayout->addWidget(m_outputDisplay);
}

/**
 * @brief 追加普通文本输出
 * @param text 要追加的文本（支持 HTML）
 *
 * @implementation
 * 使用 QTextEdit::append() 追加文本，然后自动滚动到底部。
 * 调用者需要确保传入的 text 已经格式化为有效的 HTML。
 *
 * 自动滚动机制：
 * 1. append() 追加文本
 * 2. 获取 QTextCursor
 * 3. movePosition(QTextCursor::End) 移动到文档末尾
 * 4. setTextCursor(cursor) 设置光标位置，触发滚动
 */
void OutputPanel::appendOutput(const QString& text)
{
    m_outputDisplay->append(text);
    QTextCursor cursor = m_outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputDisplay->setTextCursor(cursor);
}

/**
 * @brief 追加步骤执行头信息
 * @param stepId 步骤编号
 * @param description 步骤描述
 *
 * @implementation
 * 生成一个带紫色渐变背景和边框的步骤头卡片，包含：
 * - ▶ 步骤编号和描述（大字体，靛蓝色）
 * - 当前时间戳（小字体，灰色）
 *
 * HTML 样式：
 * - 背景：从 rgba(99,102,241,0.2) 到 rgba(139,92,246,0.15) 的渐变
 * - 边框：1px solid #4f46e5
 * - 圆角：8px
 * - 内边距：10px 12px
 */
void OutputPanel::appendStepHeader(int stepId, const QString& description)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString header = QString(
        "<div style='margin: 12px 0 8px 0; padding: 10px 12px; "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "   stop:0 rgba(99, 102, 241, 0.2), stop:1 rgba(139, 92, 246, 0.15));"
        "border: 1px solid #4f46e5; border-radius: 8px;'>"
        "  <div style='color: #818cf8; font-weight: 600; font-size: 14px;'>"
        "    <span style='font-size: 16px;'>▶</span> 步骤 %1 — %2"
        "  </div>"
        "  <div style='color: #64748b; font-size: 11px; margin-top: 4px;'>%3</div>"
        "</div>"
    ).arg(stepId).arg(description.toHtmlEscaped()).arg(time);

    m_outputDisplay->append(header);
    // 自动滚动到底部
    QTextCursor cursor = m_outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputDisplay->setTextCursor(cursor);
}

/**
 * @brief 追加步骤执行输出
 * @param stepId 步骤编号（当前未使用）
 * @param output Agent 返回的输出文本
 *
 * @implementation
 * 1. 使用 toHtmlEscaped() 转义输出文本，防止恶意内容注入 HTML
 * 2. 将换行符 \n 替换为 HTML 的 <br> 标签
 * 3. 使用等宽字体包裹，便于阅读代码和命令输出
 * 4. 追加到输出面板并自动滚动
 */
void OutputPanel::appendStepOutput(int stepId, const QString& output)
{
    QString formatted = output.toHtmlEscaped();
    formatted.replace("\n", "<br>");

    QString html = QString(
        "<div style='padding: 4px 12px; color: #cbd5e1; font-family: Consolas, monospace;'>"
        "%1"
        "</div>"
    ).arg(formatted);

    m_outputDisplay->append(html);
    QTextCursor cursor = m_outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputDisplay->setTextCursor(cursor);
}

/**
 * @brief 追加步骤执行结果
 * @param stepId 步骤编号（当前未使用）
 * @param success true=成功, false=失败
 * @param message 结果描述
 *
 * @implementation
 * 生成一个状态标记卡片：
 *
 * 成功时：
 * - 图标：✓
 * - 文字颜色：#22c55e (绿色)
 * - 背景：rgba(34, 197, 94, 0.1) (半透明绿色)
 * - 边框：#16a34a (深绿)
 *
 * 失败时：
 * - 图标：✗
 * - 文字颜色：#ef4444 (红色)
 * - 背景：rgba(239, 68, 68, 0.1) (半透明红色)
 * - 边框：#dc2626 (深红)
 */
void OutputPanel::appendStepResult(int stepId, bool success, const QString& message)
{
    QString icon = success ? "✓" : "✗";
    QString color = success ? "#22c55e" : "#ef4444";
    QString bgColor = success ? "rgba(34, 197, 94, 0.1)" : "rgba(239, 68, 68, 0.1)";
    QString borderColor = success ? "#16a34a" : "#dc2626";
    QString statusText = success ? "成功" : "失败";

    QString html = QString(
        "<div style='margin: 8px 0 12px 0; padding: 8px 12px; "
        "background-color: %1; border: 1px solid %2; border-radius: 6px;'>"
        "  <span style='color: %3; font-weight: 600;'>%4 %5</span>"
        "  <span style='color: #94a3b8; margin-left: 8px;'>%6</span>"
        "</div>"
    ).arg(bgColor, borderColor, color, icon, statusText, message.toHtmlEscaped());

    m_outputDisplay->append(html);
    QTextCursor cursor = m_outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputDisplay->setTextCursor(cursor);
}

/**
 * @brief 清空所有输出内容
 *
 * @implementation
 * 调用 QTextEdit::clear() 清除所有内容。
 */
void OutputPanel::clearOutput()
{
    m_outputDisplay->clear();
}

/**
 * @brief 清空操作处理
 *
 * @implementation
 * 1. 调用 clearOutput() 清除输出面板内容
 * 2. 发射 clearRequested 信号（供外部监听）
 */
void OutputPanel::onClearAction()
{
    clearOutput();
    emit clearRequested();
}

/**
 * @brief 复制操作处理
 *
 * @implementation
 * 1. 获取 QTextEdit 的纯文本内容（toPlainText()）
 * 2. 使用 QApplication::clipboard()->setText() 复制到系统剪贴板
 * 3. 发射 copyRequested 信号（供外部监听）
 */
void OutputPanel::onCopyAction()
{
    QApplication::clipboard()->setText(m_outputDisplay->toPlainText());
    emit copyRequested();
}
