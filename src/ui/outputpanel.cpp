/**
 * @file outputpanel.cpp
 * @brief 执行输出面板实现 —— 对话式纯文本输出
 *
 * 改为类似聊天对话的纯文本输出形式，不再使用彩色HTML卡片。
 * 所有内容以简洁的文本列表形式展示，便于阅读。
 */

#include "outputpanel.h"
#include <QDateTime>
#include <QApplication>
#include <QClipboard>

OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

OutputPanel::~OutputPanel()
{
}

void OutputPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(8);

    QWidget* headerWidget = new QWidget(this);
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_titleLabel = new QLabel("执行输出", headerWidget);
    m_titleLabel->setStyleSheet(
        "color: #f1f5f9;"
        "font-size: 14px;"
        "font-weight: 600;"
    );
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_toolBar = new QToolBar(headerWidget);
    m_toolBar->setIconSize(QSize(16, 16));
    m_toolBar->setStyleSheet(
        "QToolBar { background: transparent; spacing: 4px; }"
        "QToolButton {"
        "   background-color: transparent;"
        "   color: #94a3b8;"
        "   border: 1px solid #334155;"
        "   border-radius: 6px;"
        "   padding: 4px 10px;"
        "   font-size: 12px;"
        "}"
        "QToolButton:hover {"
        "   background-color: #1e293b;"
        "   color: #f1f5f9;"
        "}"
    );

    m_copyAction = m_toolBar->addAction("复制");
    m_clearAction = m_toolBar->addAction("清空");

    connect(m_copyAction, &QAction::triggered, this, &OutputPanel::onCopyAction);
    connect(m_clearAction, &QAction::triggered, this, &OutputPanel::onClearAction);

    headerLayout->addWidget(m_toolBar);
    m_mainLayout->addWidget(headerWidget);

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

void OutputPanel::appendOutput(const QString& text)
{
    m_outputDisplay->append(text);
    QTextCursor cursor = m_outputDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputDisplay->setTextCursor(cursor);
}

void OutputPanel::appendStepHeader(int stepId, const QString& description)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString header = QString("[%1] 步骤 %2 — %3").arg(time, QString::number(stepId), description);
    appendOutput(header);
}

void OutputPanel::appendStepOutput(int stepId, const QString& output)
{
    Q_UNUSED(stepId)
    QString formatted = output;
    formatted.replace("\n", "\n    ");
    appendOutput("    " + formatted);
}

void OutputPanel::appendStepResult(int stepId, bool success, const QString& message)
{
    Q_UNUSED(stepId)
    QString status = success ? "[成功]" : "[失败]";
    appendOutput(QString("%1 %2").arg(status, message));
    appendOutput("");
}

void OutputPanel::clearOutput()
{
    m_outputDisplay->clear();
}

void OutputPanel::onClearAction()
{
    clearOutput();
    emit clearRequested();
}

void OutputPanel::onCopyAction()
{
    QApplication::clipboard()->setText(m_outputDisplay->toPlainText());
    emit copyRequested();
}