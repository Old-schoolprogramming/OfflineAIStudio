// 【文件说明】searchagent.h - 搜索Agent的头文件
// 【作用】声明 SearchAgent 类，定义网页搜索、本地文件搜索、文本搜索等功能的接口
// 【注意事项】头文件只声明类和函数，具体实现在 searchagent.cpp 中

#ifndef SEARCHAGENT_H  // 防止重复包含的宏定义开始
#define SEARCHAGENT_H  // 定义 SEARCHAGENT_H 宏

#include "agent.h"  // 引入 Agent 基类头文件，SearchAgent 继承自 Agent
#include <QList>    // 引入 Qt 列表容器，用于存储工具对象列表

class Tool;  // 前向声明 Tool 类，告诉编译器有这个类，减少编译依赖

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
class SearchAgent : public Agent  // SearchAgent 继承自 Agent 基类
{
    Q_OBJECT  // Qt 宏，启用信号与槽、元对象系统等 Qt 特性

public:
    // 【功能说明】构造函数，创建搜索代理实例
    // 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
    explicit SearchAgent(QObject *parent = nullptr);

    // 【功能说明】析构函数，销毁搜索代理实例
    // 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象
    ~SearchAgent();

    // 【功能说明】获取代理名称
    // 【返回值】固定返回 QString 字符串 "SearchAgent"
    // 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
    QString name() const override;

    // 【功能说明】获取代理描述
    // 【返回值】返回描述此 Agent 功能的 QString 字符串
    // 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
    QString description() const override;

    // 【功能说明】获取代理类型
    // 【返回值】返回枚举值 SearchAgentType
    // 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
    AgentType type() const override;

    // 【功能说明】获取所有工具
    // 【返回值】返回 QList<Tool*> 类型的工具指针列表
    // 【说明】重写父类虚函数，返回此 Agent 注册的所有可用工具
    QList<Tool*> tools() const override;

    // 【功能说明】执行指定工具
    // 【参数说明】toolName - 要执行的工具名称字符串
    // 【参数说明】args - 工具参数字典，键值对形式
    // 【返回值】执行结果字典，包含 success（是否成功）和 result/error（结果或错误信息）
    // 【说明】重写父类虚函数，根据工具名称分发到对应的私有实现方法
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

private:
    // 【功能说明】初始化所有搜索相关工具
    // 【说明】在构造函数中调用，创建所有这个Agent拥有的工具并注册到 m_tools 列表中
    void initializeTools();

    // 【功能说明】网页搜索（当前离线模式下直接返回不可用提示）
    // 【参数说明】args - 包含 query（搜索关键词）、maxResults（最大结果数）
    // 【返回值】搜索结果或离线提示
    QVariantMap webSearch(const QVariantMap& args);

    // 【功能说明】抓取指定 URL 的网页内容（当前离线模式下直接返回不可用提示）
    // 【参数说明】args - 包含 url（目标网页地址）
    // 【返回值】网页内容或离线提示
    QVariantMap webFetch(const QVariantMap& args);

    // 【功能说明】在目录中搜索匹配的文件名
    // 【参数说明】args - 包含 dir（搜索目录）、pattern（匹配模式）、maxDepth（最大深度）
    // 【返回值】JSON 格式的文件列表
    QVariantMap searchFiles(const QVariantMap& args);

    // 【功能说明】在文件中搜索文本内容（支持正则表达式）
    // 【参数说明】args - 包含 dir（搜索目录）、pattern（关键词/正则）、filePattern（文件过滤）、caseSensitive（大小写敏感）
    // 【返回值】JSON 格式的搜索结果
    QVariantMap searchText(const QVariantMap& args);

    // 【功能说明】在代码中搜索函数、类、变量等符号
    // 【参数说明】args - 包含 dir（搜索目录）、symbol（符号名称）、language（语言过滤）
    // 【返回值】JSON 格式的代码搜索结果
    QVariantMap searchCode(const QVariantMap& args);

    QList<Tool*> m_tools;  ///< 工具列表成员变量，存储此 Agent 拥有的所有 Tool 对象指针
};

#endif  // SEARCHAGENT_H  // 宏定义结束，与开头的 #ifndef 配对
