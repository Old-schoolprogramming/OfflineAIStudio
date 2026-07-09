#ifndef SEARCHAGENT_H
#define SEARCHAGENT_H

#include "agent.h"
#include <QList>

class Tool;

/**
 * @brief 搜索 Agent - 支持本地文件搜索和离线模式
 *
 * SearchAgent 提供以下工具：
 * - webSearch: 网页搜索（离线模式下返回友好提示）
 * - webFetch: 抓取网页内容（离线模式下返回友好提示）
 * - searchFiles: 在目录中搜索文件（支持通配符）
 * - searchText: 在文件中搜索文本内容（支持正则表达式）
 * - searchCode: 搜索代码中的函数、类、变量等
 */
class SearchAgent : public Agent
{
    Q_OBJECT

public:
    explicit SearchAgent(QObject *parent = nullptr);
    ~SearchAgent();

    QString name() const override;
    QString description() const override;
    AgentType type() const override;
    QList<Tool*> tools() const override;
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    void initializeTools();
    QVariantMap webSearch(const QVariantMap& args);
    QVariantMap webFetch(const QVariantMap& args);
    QVariantMap searchFiles(const QVariantMap& args);
    QVariantMap searchText(const QVariantMap& args);
    QVariantMap searchCode(const QVariantMap& args);

    QList<Tool*> m_tools;
};

#endif
