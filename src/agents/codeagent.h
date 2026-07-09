#ifndef CODEAGENT_H
#define CODEAGENT_H

#include "agent.h"
#include "../core/environmentdetector.h"
#include <QList>

class Tool;

/**
 * @brief 代码开发Agent - 专门负责代码生成、编译、调试和项目管理
 *
 * CodeAgent是一个专业的软件开发专家，它可以：
 * - 创建项目模板（CMake、Qt、Python等）
 * - 生成代码文件（根据用户需求生成完整代码）
 * - 编译项目（调用CMake、Make等）
 * - 运行测试和调试
 * - 管理项目结构
 *
 * 在离线环境中，CodeAgent会先检测系统中可用的工具：
 * - 如果cmake不可用，编译步骤会失败并提示用户
 * - 如果python不可用，Python项目会失败并提示用户
 * - 不会尝试任何网络操作（如pip install）
 */
class CodeAgent : public Agent
{
    Q_OBJECT

public:
    explicit CodeAgent(QObject *parent = nullptr);
    ~CodeAgent();

    QString name() const override;
    QString description() const override;
    AgentType type() const override;
    QList<Tool*> tools() const override;
    QVariantMap executeTool(const QString& toolName, const QVariantMap& args) override;

    void setEnvironmentDetector(EnvironmentDetector* detector);

private:
    void initializeTools();
    QList<Tool*> m_tools;
    EnvironmentDetector* m_envDetector;

    QVariantMap createProject(const QVariantMap& args);
    QVariantMap generateCode(const QVariantMap& args);
    QVariantMap compileProject(const QVariantMap& args);
    QVariantMap runProgram(const QVariantMap& args);
    QVariantMap debugCode(const QVariantMap& args);
    QVariantMap generateCMake(const QVariantMap& args);
    QVariantMap generateQtProject(const QVariantMap& args);
    QVariantMap checkEnvironment(const QString& requiredTool);

    QVariantMap analyzeCode(const QVariantMap& args);
    QVariantMap analyzeProject(const QVariantMap& args);
    QVariantMap checkDependencies(const QVariantMap& args);
    QVariantMap cleanProject(const QVariantMap& args);
    QVariantMap formatCode(const QVariantMap& args);
    QVariantMap countLines(const QVariantMap& args);
};

#endif