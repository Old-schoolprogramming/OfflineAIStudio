// 【文件说明】codeagent.h - 代码操作Agent的头文件
// 【作用】声明 CodeAgent 类，定义代码生成、编译、分析等功能的接口
// 【注意事项】头文件只声明类和函数，具体实现在 codeagent.cpp 中

#ifndef CODEAGENT_H  // 防止重复包含的宏定义开始
#define CODEAGENT_H  // 定义 CODEAGENT_H 宏

#include "agent.h"                       // 引入 Agent 基类头文件，CodeAgent 继承自 Agent
#include "../core/environmentdetector.h" // 引入环境检测器头文件，用于检测编译工具是否可用
#include <QList>                         // 引入 Qt 列表容器，用于存储工具对象列表

class Tool;  // 前向声明 Tool 类，告诉编译器有这个类，减少编译依赖

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
class CodeAgent : public Agent  // CodeAgent 继承自 Agent 基类
{
    Q_OBJECT  // Qt 宏，启用信号与槽、元对象系统等 Qt 特性

public:
    // 【功能说明】构造函数，创建代码操作代理实例
    // 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
    explicit CodeAgent(QObject *parent = nullptr);

    // 【功能说明】析构函数，销毁代码操作代理实例
    // 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象
    ~CodeAgent();

    // 【功能说明】获取代理名称
    // 【返回值】固定返回 QString 字符串 "CodeAgent"
    // 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
    QString name() const override;

    // 【功能说明】获取代理描述
    // 【返回值】返回描述此 Agent 功能的 QString 字符串
    // 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
    QString description() const override;

    // 【功能说明】获取代理类型
    // 【返回值】返回枚举值 CodeAgentType
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

    // 【功能说明】设置环境检测器
    // 【参数说明】detector - EnvironmentDetector 指针，用于检测系统工具（如 cmake、python）
    // 【说明】外部系统通过此方法注入环境检测器，CodeAgent 在编译前会检测工具可用性
    void setEnvironmentDetector(EnvironmentDetector* detector);

private:
    // 【功能说明】初始化所有代码开发相关工具
    // 【说明】在构造函数中调用，创建所有这个Agent拥有的工具并注册到 m_tools 列表中
    void initializeTools();

    QList<Tool*> m_tools;  ///< 工具列表成员变量，存储此 Agent 拥有的所有 Tool 对象指针
    EnvironmentDetector* m_envDetector;  ///< 环境检测器指针，用于检测编译工具是否已安装

    // 【功能说明】创建项目模板（c/cpp/cmake/qt/python）
    // 【参数说明】args - 包含 path（项目路径）、type（项目类型）、name（项目名称）
    // 【返回值】创建结果字典
    QVariantMap createProject(const QVariantMap& args);

    // 【功能说明】生成代码文件
    // 【参数说明】args - 包含 path（文件路径）、content（代码内容）、language（编程语言）
    // 【返回值】生成结果字典
    QVariantMap generateCode(const QVariantMap& args);

    // 【功能说明】编译项目
    // 【参数说明】args - 包含 path（项目目录）、buildType（Debug/Release）、generator（构建工具）
    // 【返回值】编译结果字典，包含标准输出和错误信息
    QVariantMap compileProject(const QVariantMap& args);

    // 【功能说明】运行程序
    // 【参数说明】args - 包含 path（程序路径）、args（命令行参数）、workingDir（工作目录）
    // 【返回值】运行结果字典，包含程序输出
    QVariantMap runProgram(const QVariantMap& args);

    // 【功能说明】调试代码（目前仅支持 Python）
    // 【参数说明】args - 包含 path（代码文件路径）、breakpoints（断点列表）
    // 【返回值】调试结果字典
    QVariantMap debugCode(const QVariantMap& args);

    // 【功能说明】生成 CMakeLists.txt 文件
    // 【参数说明】args - 包含 path、projectName、sources、targetName、requiresQt
    // 【返回值】生成结果字典
    QVariantMap generateCMake(const QVariantMap& args);

    // 【功能说明】生成 Qt 项目文件（.pro 和 CMakeLists.txt）
    // 【参数说明】args - 包含 path、projectName、sources、forms、resources
    // 【返回值】生成结果字典
    QVariantMap generateQtProject(const QVariantMap& args);

    // 【功能说明】检查环境工具是否可用
    // 【参数说明】requiredTool - 需要的工具名称（如 "cmake"、"python"）
    // 【返回值】检测结果字典，若不可用则返回错误信息
    QVariantMap checkEnvironment(const QString& requiredTool);

    // 【功能说明】分析单个代码文件（统计行数、查找类/函数/变量）
    // 【参数说明】args - 包含 path（文件路径）、language（编程语言）
    // 【返回值】JSON 格式的分析结果
    QVariantMap analyzeCode(const QVariantMap& args);

    // 【功能说明】分析项目结构（统计文件类型、数量等）
    // 【参数说明】args - 包含 path（项目目录）
    // 【返回值】JSON 格式的项目信息
    QVariantMap analyzeProject(const QVariantMap& args);

    // 【功能说明】检查项目依赖（解析 CMakeLists.txt、requirements.txt、package.json 等）
    // 【参数说明】args - 包含 path（项目目录）、type（项目类型）
    // 【返回值】JSON 格式的依赖列表
    QVariantMap checkDependencies(const QVariantMap& args);

    // 【功能说明】清理项目构建文件（删除 build 目录、中间文件等）
    // 【参数说明】args - 包含 path（项目目录）
    // 【返回值】清理结果字典，列出已删除的文件
    QVariantMap cleanProject(const QVariantMap& args);

    // 【功能说明】格式化代码（调用 clang-format、black、prettier 等）
    // 【参数说明】args - 包含 path（文件路径）、language（编程语言）
    // 【返回值】格式化结果字典
    QVariantMap formatCode(const QVariantMap& args);

    // 【功能说明】统计代码行数
    // 【参数说明】args - 包含 path（目录或文件路径）、language（语言过滤）
    // 【返回值】JSON 格式的行数统计结果
    QVariantMap countLines(const QVariantMap& args);

    // 【功能说明】查找代码中的函数定义
    // 【参数说明】args - 包含 path（文件路径）
    // 【返回值】函数列表字符串
    QVariantMap findFunctions(const QVariantMap& args);

    // 【功能说明】查找代码中的类定义
    // 【参数说明】args - 包含 path（文件路径）
    // 【返回值】类/结构列表字符串
    QVariantMap findClasses(const QVariantMap& args);

    // 【功能说明】查找代码中的 TODO/FIXME 等标记
    // 【参数说明】args - 包含 path（文件或目录路径）、recursive（是否递归）
    // 【返回值】标记列表字符串
    QVariantMap findTodos(const QVariantMap& args);

    // 【功能说明】检测文件编码格式（UTF-8、UTF-16、GBK 等）
    // 【参数说明】args - 包含 path（文件路径）
    // 【返回值】编码检测结果字符串
    QVariantMap detectEncoding(const QVariantMap& args);

    // 【功能说明】统计代码注释比例
    // 【参数说明】args - 包含 path（文件路径）
    // 【返回值】注释统计结果字符串
    QVariantMap commentRatio(const QVariantMap& args);

    // 【功能说明】移除代码中的空白行
    // 【参数说明】args - 包含 path（文件路径）、outputPath（输出路径，可选）
    // 【返回值】处理结果字典
    QVariantMap removeBlankLines(const QVariantMap& args);

    // 【功能说明】生成单元测试代码模板
    // 【参数说明】args - 包含 path（源文件路径）、language（编程语言）
    // 【返回值】测试结果文件路径
    QVariantMap generateTests(const QVariantMap& args);

    // 【功能说明】重构代码（批量替换名称）
    // 【参数说明】args - 包含 path（文件路径）、oldName（旧名称）、newName（新名称）
    // 【返回值】重构结果字典，报告替换次数
    QVariantMap refactorCode(const QVariantMap& args);

    // 【功能说明】提取类接口头文件
    // 【参数说明】args - 包含 path（源文件路径）、outputPath（输出头文件路径）
    // 【返回值】提取结果字典
    QVariantMap extractInterface(const QVariantMap& args);
};

#endif  // CODEAGENT_H  // 宏定义结束，与开头的 #ifndef 配对
