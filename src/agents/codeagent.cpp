// 【文件说明】codeagent.cpp - 代码操作代理的实现文件
// 【作用】实现 CodeAgent 类中声明的所有函数，提供代码生成、编译、分析等功能
// 【注意事项】所有工具执行结果以 QVariantMap 形式返回，统一包含 success 和 result/error 字段

#include "codeagent.h"    // 引入 CodeAgent 自己的头文件，获取类声明
#include "tool.h"         // 引入 Tool 类头文件，用于创建和注册工具
#include <QFile>          // 引入 Qt 文件类，用于代码文件的读写
#include <QDir>           // 引入 Qt 目录类，用于项目目录管理
#include <QDirIterator>   // 引入 Qt 目录迭代器，用于遍历项目文件
#include <QTextStream>    // 引入 Qt 文本流类，用于方便地读写文本内容
#include <QProcess>       // 引入 Qt 进程类，用于调用外部编译工具（cmake、make 等）
#include <QJsonArray>     // 引入 Qt JSON 数组类，用于构建 JSON 格式的返回数据
#include <QJsonObject>    // 引入 Qt JSON 对象类，用于构建 JSON 键值对
#include <QJsonDocument>  // 引入 Qt JSON 文档类，用于将 JSON 对象序列化为字符串
#include <QDebug>         // 引入 Qt 调试输出类，用于输出警告信息

// 【功能说明】CodeAgent 构造函数，创建代码操作代理实例
// 【参数说明】parent - 父 QObject 对象指针，用于 Qt 对象树内存管理，默认为 nullptr
// 【说明】调用父类 Agent 的构造函数，初始化环境检测器为 nullptr，并注册所有工具
CodeAgent::CodeAgent(QObject *parent)
    : Agent(parent),       // 调用父类 Agent 的构造函数，传入父对象指针
      m_envDetector(nullptr)  // 初始化环境检测器指针为 nullptr，表示尚未设置
{
    initializeTools();  // 初始化并注册所有代码开发工具到 m_tools 列表
}

// 【功能说明】CodeAgent 析构函数，销毁代码操作代理实例
// 【说明】自动释放 initializeTools() 中动态分配的所有 Tool 对象，防止内存泄漏
CodeAgent::~CodeAgent()
{
    qDeleteAll(m_tools);  // 遍历 m_tools 列表，删除每一个 Tool 对象指针指向的内存
}

// 【功能说明】设置环境检测器
// 【参数说明】detector - EnvironmentDetector 指针，指向外部创建的环境检测器对象
// 【说明】外部系统通过此方法注入环境检测器，CodeAgent 在编译前会用它来检测工具是否已安装
void CodeAgent::setEnvironmentDetector(EnvironmentDetector* detector)
{
    m_envDetector = detector;  // 保存环境检测器指针到成员变量
}

// 【功能说明】获取代理名称
// 【返回值】固定返回 QString 字符串 "CodeAgent"
// 【说明】重写父类虚函数，系统通过此名称识别和查找该 Agent
QString CodeAgent::name() const
{
    return "CodeAgent";  // 返回此 Agent 的唯一标识名称
}

// 【功能说明】获取代理功能描述
// 【返回值】返回描述此 Agent 功能的 QString 字符串
// 【说明】重写父类虚函数，向用户或系统说明此 Agent 的用途
QString CodeAgent::description() const
{
    return "代码开发Agent，负责项目创建、代码生成、编译和运行";  // 返回中文功能描述
}

// 【功能说明】获取代理类型
// 【返回值】固定返回枚举值 CodeAgentType
// 【说明】重写父类虚函数，用于在系统中区分不同类型的 Agent
AgentType CodeAgent::type() const
{
    return CodeAgentType;  // 返回代码代理类型枚举值
}

// 【功能说明】获取当前代理注册的所有工具列表
// 【返回值】返回 QList<Tool*> 类型的工具指针列表
// 【说明】重写父类虚函数，外部系统调用此函数获取可用工具清单
QList<Tool*> CodeAgent::tools() const
{
    return m_tools;  // 返回内部存储的工具列表（m_tools 是成员变量）
}

// 【功能说明】初始化并注册所有代码开发相关工具
// 【说明】在构造函数中调用一次，为每个功能创建一个 Tool 对象并添加到 m_tools 列表
void CodeAgent::initializeTools()
{
    // ========== 1. 注册 createProject 工具 ==========
    // 【工具说明】创建项目模板（c/cpp/cmake/qt/python）
    // 【必填参数】path - 项目路径；type - 项目类型；name - 项目名称
    Tool* createProjectTool = new Tool("createProject", "创建项目模板",
        [this](const QVariantMap& args) { return this->createProject(args); });
    createProjectTool->addParameter("path", "string", "项目路径", true);
    createProjectTool->addParameter("type", "string", "项目类型(c/cpp/cmake/qt/python)", true);
    createProjectTool->addParameter("name", "string", "项目名称", true);
    m_tools.append(createProjectTool);

    // ========== 2. 注册 generateCode 工具 ==========
    // 【工具说明】生成代码文件，将完整代码内容写入指定路径
    // 【必填参数】path - 文件路径；content - 完整代码内容
    // 【可选参数】language - 编程语言，默认 "cpp"
    Tool* generateCodeTool = new Tool("generateCode", "生成代码文件（content参数必须包含完整可运行的代码，不能只写注释或框架）",
        [this](const QVariantMap& args) { return this->generateCode(args); });
    generateCodeTool->addParameter("path", "string", "文件路径", true);
    generateCodeTool->addParameter("content", "string", "完整的代码内容（必须是完整可运行的代码，包含所有函数实现）", true);
    generateCodeTool->addParameter("language", "string", "编程语言(c/cpp/python/java/js)", false, "cpp");
    m_tools.append(generateCodeTool);

    // ========== 3. 注册 generateCMake 工具 ==========
    // 【工具说明】生成 CMakeLists.txt 文件
    // 【必填参数】path - CMakeLists.txt 路径；projectName - 项目名称；sources - 源文件列表
    // 【可选参数】targetName - 目标可执行文件名；requiresQt - 是否需要 Qt，默认 false
    Tool* generateCMakeTool = new Tool("generateCMake", "生成CMakeLists.txt",
        [this](const QVariantMap& args) { return this->generateCMake(args); });
    generateCMakeTool->addParameter("path", "string", "CMakeLists.txt路径", true);
    generateCMakeTool->addParameter("projectName", "string", "项目名称", true);
    generateCMakeTool->addParameter("sources", "array", "源文件列表", true);
    generateCMakeTool->addParameter("targetName", "string", "目标可执行文件名称", false);
    generateCMakeTool->addParameter("requiresQt", "bool", "是否需要Qt", false, "false");
    m_tools.append(generateCMakeTool);

    // ========== 4. 注册 generateQtProject 工具 ==========
    // 【工具说明】生成 Qt 项目文件（.pro 和 CMakeLists.txt）
    // 【必填参数】path - 项目目录；projectName - 项目名称；sources - 源文件列表
    // 【可选参数】forms - UI 文件列表；resources - 资源文件列表
    Tool* generateQtProjectTool = new Tool("generateQtProject", "生成Qt项目文件",
        [this](const QVariantMap& args) { return this->generateQtProject(args); });
    generateQtProjectTool->addParameter("path", "string", "项目目录", true);
    generateQtProjectTool->addParameter("projectName", "string", "项目名称", true);
    generateQtProjectTool->addParameter("sources", "array", "源文件列表", true);
    generateQtProjectTool->addParameter("forms", "array", "UI文件列表", false);
    generateQtProjectTool->addParameter("resources", "array", "资源文件列表", false);
    m_tools.append(generateQtProjectTool);

    // ========== 5. 注册 compileProject 工具 ==========
    // 【工具说明】编译项目（调用 cmake 和构建工具）
    // 【必填参数】path - 项目目录
    // 【可选参数】buildType - Debug/Release，默认 "Release"；generator - Ninja/Makefiles，默认 "Ninja"
    Tool* compileProjectTool = new Tool("compileProject", "编译项目",
        [this](const QVariantMap& args) { return this->compileProject(args); });
    compileProjectTool->addParameter("path", "string", "项目目录", true);
    compileProjectTool->addParameter("buildType", "string", "构建类型(Debug/Release)", false, "Release");
    compileProjectTool->addParameter("generator", "string", "构建生成器(Ninja/Makefiles)", false, "Ninja");
    m_tools.append(compileProjectTool);

    // ========== 6. 注册 runProgram 工具 ==========
    // 【工具说明】运行可执行程序
    // 【必填参数】path - 程序路径
    // 【可选参数】args - 命令行参数数组；workingDir - 工作目录
    Tool* runProgramTool = new Tool("runProgram", "运行程序",
        [this](const QVariantMap& args) { return this->runProgram(args); });
    runProgramTool->addParameter("path", "string", "程序路径", true);
    runProgramTool->addParameter("args", "array", "命令行参数", false);
    runProgramTool->addParameter("workingDir", "string", "工作目录", false);
    m_tools.append(runProgramTool);

    // ========== 7. 注册 debugCode 工具 ==========
    // 【工具说明】调试代码（目前仅支持 Python 的 pdb）
    // 【必填参数】path - 代码文件路径
    // 【可选参数】breakpoints - 断点列表
    Tool* debugCodeTool = new Tool("debugCode", "调试代码",
        [this](const QVariantMap& args) { return this->debugCode(args); });
    debugCodeTool->addParameter("path", "string", "代码文件路径", true);
    debugCodeTool->addParameter("breakpoints", "array", "断点列表", false);
    m_tools.append(debugCodeTool);

    // ========== 8. 注册 analyzeCode 工具 ==========
    // 【工具说明】分析单个代码文件（统计行数、查找类/函数/变量）
    // 【必填参数】path - 代码文件路径
    // 【可选参数】language - 编程语言，默认 "cpp"
    Tool* analyzeCodeTool = new Tool("analyzeCode", "分析代码文件",
        [this](const QVariantMap& args) { return this->analyzeCode(args); });
    analyzeCodeTool->addParameter("path", "string", "代码文件路径", true);
    analyzeCodeTool->addParameter("language", "string", "编程语言(cpp/python/js)", false, "cpp");
    m_tools.append(analyzeCodeTool);

    // ========== 9. 注册 analyzeProject 工具 ==========
    // 【工具说明】分析项目结构（统计文件类型、数量等）
    // 【必填参数】path - 项目目录
    Tool* analyzeProjectTool = new Tool("analyzeProject", "分析项目结构",
        [this](const QVariantMap& args) { return this->analyzeProject(args); });
    analyzeProjectTool->addParameter("path", "string", "项目目录", true);
    m_tools.append(analyzeProjectTool);

    // ========== 10. 注册 checkDependencies 工具 ==========
    // 【工具说明】检查项目依赖（解析 CMakeLists.txt、requirements.txt、package.json 等）
    // 【必填参数】path - 项目目录
    // 【可选参数】type - 项目类型，默认 "cpp"
    Tool* checkDependenciesTool = new Tool("checkDependencies", "检查项目依赖",
        [this](const QVariantMap& args) { return this->checkDependencies(args); });
    checkDependenciesTool->addParameter("path", "string", "项目目录", true);
    checkDependenciesTool->addParameter("type", "string", "项目类型(cpp/python/npm/go)", false, "cpp");
    m_tools.append(checkDependenciesTool);

    // ========== 11. 注册 cleanProject 工具 ==========
    // 【工具说明】清理项目构建文件（删除 build 目录、中间文件等）
    // 【必填参数】path - 项目目录
    Tool* cleanProjectTool = new Tool("cleanProject", "清理项目构建文件",
        [this](const QVariantMap& args) { return this->cleanProject(args); });
    cleanProjectTool->addParameter("path", "string", "项目目录", true);
    m_tools.append(cleanProjectTool);

    // ========== 12. 注册 formatCode 工具 ==========
    // 【工具说明】格式化代码（调用 clang-format、black、prettier 等外部工具）
    // 【必填参数】path - 代码文件路径
    // 【可选参数】language - 编程语言，默认 "cpp"
    Tool* formatCodeTool = new Tool("formatCode", "格式化代码",
        [this](const QVariantMap& args) { return this->formatCode(args); });
    formatCodeTool->addParameter("path", "string", "代码文件路径", true);
    formatCodeTool->addParameter("language", "string", "编程语言(cpp/python/js)", false, "cpp");
    m_tools.append(formatCodeTool);

    // ========== 13. 注册 countLines 工具 ==========
    // 【工具说明】统计代码行数
    // 【必填参数】path - 项目目录或文件路径
    // 【可选参数】language - 语言过滤，默认 ""（不过滤）
    Tool* countLinesTool = new Tool("countLines", "统计代码行数",
        [this](const QVariantMap& args) { return this->countLines(args); });
    countLinesTool->addParameter("path", "string", "项目目录或文件路径", true);
    countLinesTool->addParameter("language", "string", "语言过滤(cpp/python/js)", false, "");
    m_tools.append(countLinesTool);

    // ========== 14. 注册 findFunctions 工具 ==========
    // 【工具说明】查找代码中的函数定义
    // 【必填参数】path - 文件路径
    Tool* findFunctionsTool = new Tool("findFunctions", "查找代码中的函数定义",
        [this](const QVariantMap& args) { return this->findFunctions(args); });
    findFunctionsTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(findFunctionsTool);

    // ========== 15. 注册 findClasses 工具 ==========
    // 【工具说明】查找代码中的类定义
    // 【必填参数】path - 文件路径
    Tool* findClassesTool = new Tool("findClasses", "查找代码中的类定义",
        [this](const QVariantMap& args) { return this->findClasses(args); });
    findClassesTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(findClassesTool);

    // ========== 16. 注册 findTodos 工具 ==========
    // 【工具说明】查找代码中的 TODO/FIXME 等标记
    // 【必填参数】path - 文件或目录路径
    // 【可选参数】recursive - 是否递归子目录，默认 "true"
    Tool* findTodosTool = new Tool("findTodos", "查找代码中的TODO/FIXME标记",
        [this](const QVariantMap& args) { return this->findTodos(args); });
    findTodosTool->addParameter("path", "string", "文件或目录路径", true);
    findTodosTool->addParameter("recursive", "bool", "是否递归子目录", false, "true");
    m_tools.append(findTodosTool);

    // ========== 17. 注册 detectEncoding 工具 ==========
    // 【工具说明】检测文件编码格式（UTF-8、UTF-16、GBK 等）
    // 【必填参数】path - 文件路径
    Tool* detectEncodingTool = new Tool("detectEncoding", "检测文件编码格式",
        [this](const QVariantMap& args) { return this->detectEncoding(args); });
    detectEncodingTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(detectEncodingTool);

    // ========== 18. 注册 commentRatio 工具 ==========
    // 【工具说明】统计代码注释比例
    // 【必填参数】path - 文件路径
    Tool* commentRatioTool = new Tool("commentRatio", "统计代码注释比例",
        [this](const QVariantMap& args) { return this->commentRatio(args); });
    commentRatioTool->addParameter("path", "string", "文件路径", true);
    m_tools.append(commentRatioTool);

    // ========== 19. 注册 removeBlankLines 工具 ==========
    // 【工具说明】移除代码中的空白行
    // 【必填参数】path - 文件路径
    // 【可选参数】outputPath - 输出文件路径，默认 ""（覆盖原文件）
    Tool* removeBlankLinesTool = new Tool("removeBlankLines", "移除代码中的空白行",
        [this](const QVariantMap& args) { return this->removeBlankLines(args); });
    removeBlankLinesTool->addParameter("path", "string", "文件路径", true);
    removeBlankLinesTool->addParameter("outputPath", "string", "输出文件路径", false, "");
    m_tools.append(removeBlankLinesTool);

    // ========== 20. 注册 generateTests 工具 ==========
    // 【工具说明】生成单元测试代码模板
    // 【必填参数】path - 源文件路径
    // 【可选参数】language - 编程语言，默认 "cpp"
    Tool* generateTestsTool = new Tool("generateTests", "生成单元测试代码",
        [this](const QVariantMap& args) { return this->generateTests(args); });
    generateTestsTool->addParameter("path", "string", "源文件路径", true);
    generateTestsTool->addParameter("language", "string", "编程语言(cpp/python)", false, "cpp");
    m_tools.append(generateTestsTool);

    // ========== 21. 注册 refactorCode 工具 ==========
    // 【工具说明】重构代码（批量替换名称）
    // 【必填参数】path - 文件路径；oldName - 旧名称；newName - 新名称
    Tool* refactorCodeTool = new Tool("refactorCode", "重构代码",
        [this](const QVariantMap& args) { return this->refactorCode(args); });
    refactorCodeTool->addParameter("path", "string", "文件路径", true);
    refactorCodeTool->addParameter("oldName", "string", "旧名称", true);
    refactorCodeTool->addParameter("newName", "string", "新名称", true);
    m_tools.append(refactorCodeTool);

    // ========== 22. 注册 extractInterface 工具 ==========
    // 【工具说明】提取类接口头文件（生成纯虚接口）
    // 【必填参数】path - 源文件路径；outputPath - 输出头文件路径
    Tool* extractInterfaceTool = new Tool("extractInterface", "提取类接口头文件",
        [this](const QVariantMap& args) { return this->extractInterface(args); });
    extractInterfaceTool->addParameter("path", "string", "源文件路径", true);
    extractInterfaceTool->addParameter("outputPath", "string", "输出头文件路径", true);
    m_tools.append(extractInterfaceTool);
}

// 【功能说明】检查环境工具是否可用
// 【参数说明】requiredTool - 需要的工具名称字符串（如 "cmake"、"python"）
// 【返回值】若工具不可用，返回包含 success=false 和 error 的结果；若可用，返回 success=true
// 【说明】如果 m_envDetector 未设置，直接返回空结果（不阻止操作）
QVariantMap CodeAgent::checkEnvironment(const QString& requiredTool)
{
    QVariantMap result;  // 创建结果字典
    if (!m_envDetector) {  // 如果环境检测器未设置
        return result;  // 返回空结果，不阻止后续操作
    }

    // 使用环境检测器检查工具是否可用
    if (!m_envDetector->isToolAvailable(requiredTool)) {  // 如果工具不可用
        result["success"] = false;  // 标记失败
        result["error"] = QString("工具 %1 不可用，请确保已安装并添加到系统PATH中").arg(requiredTool);  // 设置错误提示
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功（工具可用）
    return result;  // 返回成功结果
}

// 【功能说明】工具分发执行入口，根据工具名称调用对应的私有实现方法
// 【参数说明】toolName - 要执行的工具名称字符串
// 【参数说明】args - 工具参数字典，键为 QString，值为 QVariant
// 【返回值】QVariantMap 执行结果，包含 success（bool）、result（QString）或 error（QString）
QVariantMap CodeAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    // 使用 if-else 链根据 toolName 将请求分派到对应的私有方法
    if (toolName == "createProject") {  // 创建项目
        return createProject(args);
    } else if (toolName == "generateCode") {  // 生成代码
        return generateCode(args);
    } else if (toolName == "generateCMake") {  // 生成 CMakeLists.txt
        return generateCMake(args);
    } else if (toolName == "generateQtProject") {  // 生成 Qt 项目
        return generateQtProject(args);
    } else if (toolName == "compileProject") {  // 编译项目
        return compileProject(args);
    } else if (toolName == "runProgram") {  // 运行程序
        return runProgram(args);
    } else if (toolName == "debugCode") {  // 调试代码
        return debugCode(args);
    } else if (toolName == "analyzeCode") {  // 分析代码
        return analyzeCode(args);
    } else if (toolName == "analyzeProject") {  // 分析项目
        return analyzeProject(args);
    } else if (toolName == "checkDependencies") {  // 检查依赖
        return checkDependencies(args);
    } else if (toolName == "cleanProject") {  // 清理项目
        return cleanProject(args);
    } else if (toolName == "formatCode") {  // 格式化代码
        return formatCode(args);
    } else if (toolName == "countLines") {  // 统计行数
        return countLines(args);
    } else if (toolName == "findFunctions") {  // 查找函数
        return findFunctions(args);
    } else if (toolName == "findClasses") {  // 查找类
        return findClasses(args);
    } else if (toolName == "findTodos") {  // 查找 TODO
        return findTodos(args);
    } else if (toolName == "detectEncoding") {  // 检测编码
        return detectEncoding(args);
    } else if (toolName == "commentRatio") {  // 注释比例
        return commentRatio(args);
    } else if (toolName == "removeBlankLines") {  // 移除空白行
        return removeBlankLines(args);
    } else if (toolName == "generateTests") {  // 生成测试
        return generateTests(args);
    } else if (toolName == "refactorCode") {  // 重构代码
        return refactorCode(args);
    } else if (toolName == "extractInterface") {  // 提取接口
        return extractInterface(args);
    }

    // 未知工具返回通用错误
    QVariantMap result;  // 创建结果字典
    result["success"] = false;  // 标记失败
    result["error"] = "Unknown tool: " + toolName;  // 返回错误提示
    return result;  // 返回错误结果
}

// 【功能说明】创建项目模板
// 【参数说明】args - 包含 path（项目路径）、type（项目类型：c/cpp/cmake/qt/python）、name（项目名称）
// 【返回值】若成功，result 字段包含项目创建成功提示；若失败，error 字段包含原因
// 【说明】根据 type 创建不同的项目结构和初始文件
QVariantMap CodeAgent::createProject(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取项目路径
    QString type = args.value("type").toString().toLower();  // 提取项目类型并转为小写，便于统一判断
    QString name = args.value("name").toString();  // 提取项目名称

    QDir dir(path);  // 创建 QDir 对象
    if (!dir.exists()) {  // 如果目录不存在
        if (!dir.mkpath(".")) {  // 尝试创建目录
            result["success"] = false;  // 创建失败
            result["error"] = "无法创建目录: " + path;  // 设置错误信息
            return result;  // 返回错误结果
        }
    }

    QString fullPath = dir.absolutePath();  // 获取目录的绝对路径

    // 根据项目类型创建对应的初始文件
    if (type == "c" || type == "cpp") {  // C/C++ 项目
        // 创建 main.cpp 文件
        QFile mainFile(fullPath + "/main.cpp");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 以只写文本模式打开
            QTextStream out(&mainFile);  // 创建文本输出流
            out << "#include <iostream>\n\n";  // 写入头文件包含
            out << "int main() {\n";  // 写入 main 函数定义
            out << "    std::cout << \"Hello, \" << " << name << " << \"!\" << std::endl;\n";  // 写入输出语句
            out << "    return 0;\n";  // 写入返回语句
            out << "}\n";  // 写入函数结束
            mainFile.close();  // 关闭文件
        }

        // 创建 CMakeLists.txt 文件
        QFile cmakeFile(fullPath + "/CMakeLists.txt");
        if (cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cmakeFile);
            out << "cmake_minimum_required(VERSION 3.16)\n";  // 写入 CMake 最低版本要求
            out << "project(" << name << ")\n\n";  // 写入项目名称
            out << "set(CMAKE_CXX_STANDARD 17)\n";  // 设置 C++ 标准
            out << "add_executable(" << name << " main.cpp)\n";  // 添加可执行文件目标
            cmakeFile.close();  // 关闭文件
        }
    } else if (type == "python") {  // Python 项目
        // 创建 main.py 文件
        QFile mainFile(fullPath + "/main.py");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << "print(\"Hello, " << name << "!\")\n";  // 写入打印语句
            mainFile.close();  // 关闭文件
        }

        // 创建 __init__.py 文件（标记为 Python 包）
        QFile initFile(fullPath + "/__init__.py");
        initFile.open(QIODevice::WriteOnly | QIODevice::Text);  // 创建空文件
        initFile.close();  // 关闭文件
    } else if (type == "cmake") {  // CMake 项目
        // 创建 main.cpp 文件
        QFile mainFile(fullPath + "/main.cpp");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << "#include <iostream>\n\n";
            out << "int main() {\n";
            out << "    std::cout << \"CMake Project: \" << " << name << " << \"!\" << std::endl;\n";
            out << "    return 0;\n";
            out << "}\n";
            mainFile.close();
        }

        // 创建 CMakeLists.txt 文件
        QFile cmakeFile(fullPath + "/CMakeLists.txt");
        if (cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cmakeFile);
            out << "cmake_minimum_required(VERSION 3.16)\n";
            out << "project(" << name << " LANGUAGES CXX)\n\n";  // 指定项目语言为 C++
            out << "set(CMAKE_CXX_STANDARD 17)\n";
            out << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";  // 强制要求 C++17
            out << "add_executable(" << name << "\n";  // 添加可执行文件
            out << "    main.cpp\n";  // 源文件列表
            out << ")\n\n";
            out << "install(TARGETS " << name << " DESTINATION bin)\n";  // 安装目标
            cmakeFile.close();
        }

        // 创建 build 目录
        QDir buildDir(fullPath + "/build");
        buildDir.mkpath(".");  // 创建构建目录
    } else if (type == "qt") {  // Qt 项目
        // 创建 main.cpp 文件
        QFile mainFile(fullPath + "/main.cpp");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << "#include <QApplication>\n";  // 包含 Qt 应用类
            out << "#include <QLabel>\n\n";  // 包含 Qt 标签类
            out << "int main(int argc, char *argv[]) {\n";
            out << "    QApplication app(argc, argv);\n";  // 创建 Qt 应用对象
            out << "    QLabel label(\"Hello, " << name << "!\");\n";  // 创建标签
            out << "    label.show();\n";  // 显示标签
            out << "    return app.exec();\n";  // 进入事件循环
            out << "}\n";
            mainFile.close();
        }

        // 创建 .pro 文件（Qt 传统项目文件）
        QFile proFile(fullPath + "/" + name + ".pro");
        if (proFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&proFile);
            out << "QT += core gui widgets\n\n";  // 添加 Qt 模块
            out << "TARGET = " << name << "\n";  // 设置目标名称
            out << "TEMPLATE = app\n\n";  // 设置模板为应用程序
            out << "SOURCES += main.cpp\n";  // 添加源文件
            proFile.close();
        }
    }

    result["success"] = true;  // 标记成功
    result["result"] = "项目创建成功: " + fullPath;  // 返回成功提示
    return result;  // 返回成功结果
}

// 【功能说明】生成代码文件
// 【参数说明】args - 包含 path（文件路径）、content（代码内容）
// 【返回值】若成功，result 字段包含生成成功提示；若失败，error 字段包含原因
QVariantMap CodeAgent::generateCode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径
    QString content = args.value("content").toString();  // 提取代码内容

    QFile file(path);  // 创建 QFile 对象
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 尝试以只写文本模式打开
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream out(&file);  // 创建文本输出流
    out << content;  // 将代码内容写入文件
    file.close();  // 关闭文件

    result["success"] = true;  // 标记成功
    result["result"] = "代码文件生成成功: " + path;  // 返回成功提示
    return result;  // 返回成功结果
}

// 【功能说明】生成 CMakeLists.txt 文件
// 【参数说明】args - 包含 path、projectName、sources、targetName、requiresQt
// 【返回值】若成功，result 字段包含生成成功提示；若失败，error 字段包含原因
QVariantMap CodeAgent::generateCMake(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取 CMakeLists.txt 路径
    QString projectName = args.value("projectName").toString();  // 提取项目名称
    QVariantList sources = args.value("sources").toList();  // 提取源文件列表（QVariantList 类型）
    QString targetName = args.value("targetName", projectName).toString();  // 提取目标名，默认使用项目名
    bool requiresQt = args.value("requiresQt", false).toBool();  // 提取是否需要 Qt，默认 false

    // 将 QVariantList 转换为 QStringList
    QStringList sourceList;
    for (const QVariant& src : sources) {
        sourceList.append(src.toString());  // 将每个 QVariant 转为 QString 并添加到列表
    }

    QFile file(path);  // 创建 QFile 对象
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream out(&file);  // 创建文本输出流
    out << "cmake_minimum_required(VERSION 3.16)\n";  // 写入 CMake 最低版本
    out << "project(" << projectName << " LANGUAGES CXX)\n\n";  // 写入项目声明
    out << "set(CMAKE_CXX_STANDARD 17)\n";  // 设置 C++ 标准
    out << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";  // 强制要求 C++17

    if (requiresQt) {  // 如果需要 Qt 支持
        out << "find_package(Qt6 REQUIRED COMPONENTS Core Widgets)\n";  // 查找 Qt6 包
        out << "qt_standard_project_setup()\n\n";  // 设置 Qt 标准项目
    }

    out << "add_executable(" << targetName << "\n";  // 声明可执行文件目标
    for (const QString& src : sourceList) {  // 遍历源文件列表
        out << "    " << src << "\n";  // 写入每个源文件
    }
    out << ")\n\n";  // 目标声明结束

    if (requiresQt) {  // 如果需要 Qt 支持
        out << "target_link_libraries(" << targetName << " PRIVATE Qt6::Core Qt6::Widgets)\n";  // 链接 Qt 库
    }

    out << "install(TARGETS " << targetName << " DESTINATION bin)\n";  // 安装目标
    file.close();  // 关闭文件

    result["success"] = true;  // 标记成功
    result["result"] = "CMakeLists.txt 生成成功: " + path;  // 返回成功提示
    return result;  // 返回成功结果
}

// 【功能说明】生成 Qt 项目文件（.pro 和 CMakeLists.txt）
// 【参数说明】args - 包含 path（项目目录）、projectName（项目名称）、sources（源文件列表）、forms（UI文件列表）、resources（资源文件列表）
// 【返回值】若成功，result 字段包含生成成功提示；若失败，error 字段包含原因
QVariantMap CodeAgent::generateQtProject(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取项目目录路径
    QString projectName = args.value("projectName").toString();  // 提取项目名称
    QVariantList sources = args.value("sources").toList();  // 提取源文件列表
    QVariantList forms = args.value("forms").toList();  // 提取 UI 文件列表
    QVariantList resources = args.value("resources").toList();  // 提取资源文件列表

    // 将 QVariantList 转换为 QStringList
    QStringList sourceList;
    for (const QVariant& src : sources) {
        sourceList.append(src.toString());
    }

    // 生成 .pro 文件
    QString qmakePath = path + "/" + projectName + ".pro";  // 构建 .pro 文件路径
    QFile proFile(qmakePath);  // 创建 QFile 对象
    if (!proFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法创建 .pro 文件: " + qmakePath;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream out(&proFile);  // 创建文本输出流
    out << "QT += core gui widgets\n\n";  // 添加 Qt 模块
    out << "TARGET = " << projectName << "\n";  // 设置目标名称
    out << "TEMPLATE = app\n\n";  // 设置模板
    out << "CONFIG += c++17\n\n";  // 配置 C++17
    out << "SOURCES += \\\n";  // 开始源文件列表
    for (int i = 0; i < sourceList.size(); ++i) {  // 遍历源文件
        out << "    " << sourceList[i];  // 写入源文件路径
        if (i < sourceList.size() - 1) out << " \\\n";  // 除了最后一个，都加换行和续行符
    }
    out << "\n\n";  // 空行

    // 如果有 UI 文件，写入 FORMS 段
    if (!forms.isEmpty()) {
        out << "FORMS += \\\n";
        for (int i = 0; i < forms.size(); ++i) {
            out << "    " << forms[i].toString();
            if (i < forms.size() - 1) out << " \\\n";
        }
        out << "\n\n";
    }

    // 如果有资源文件，写入 RESOURCES 段
    if (!resources.isEmpty()) {
        out << "RESOURCES += \\\n";
        for (int i = 0; i < resources.size(); ++i) {
            out << "    " << resources[i].toString();
            if (i < resources.size() - 1) out << " \\\n";
        }
        out << "\n";
    }

    proFile.close();  // 关闭 .pro 文件

    // 同时生成 CMakeLists.txt（现代 Qt 项目推荐方式）
    QString cmakePath = path + "/CMakeLists.txt";
    QFile cmakeFile(cmakePath);
    if (cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream cmakeOut(&cmakeFile);
        cmakeOut << "cmake_minimum_required(VERSION 3.16)\n";
        cmakeOut << "project(" << projectName << " LANGUAGES CXX)\n\n";
        cmakeOut << "find_package(Qt6 REQUIRED COMPONENTS Core Widgets)\n";  // 查找 Qt6
        cmakeOut << "qt_standard_project_setup()\n\n";
        cmakeOut << "qt_add_executable(" << projectName << "\n";  // 使用 qt_add_executable
        for (const QString& src : sourceList) {
            cmakeOut << "    " << src << "\n";
        }
        cmakeOut << ")\n\n";
        cmakeOut << "target_link_libraries(" << projectName << " PRIVATE Qt6::Core Qt6::Widgets)\n";  // 链接 Qt 库
        cmakeOut << "install(TARGETS " << projectName << " DESTINATION bin)\n";  // 安装目标
        cmakeFile.close();  // 关闭文件
    }

    result["success"] = true;  // 标记成功
    result["result"] = "Qt项目文件生成成功: " + qmakePath;  // 返回成功提示
    return result;  // 返回成功结果
}

// 【功能说明】编译项目
// 【参数说明】args - 包含 path（项目目录）、buildType（Debug/Release，默认 Release）、generator（Ninja/Makefiles，默认 Ninja）
// 【返回值】若成功，result 字段包含编译输出；若失败，error 字段包含原因
// 【说明】使用 cmake 配置项目，然后调用构建工具（ninja 或 make）编译
QVariantMap CodeAgent::compileProject(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取项目目录路径
    QString buildType = args.value("buildType", "Release").toString();  // 提取构建类型，默认 Release
    QString generator = args.value("generator", "Ninja").toString();  // 提取构建生成器，默认 Ninja

    QDir dir(path);  // 创建 QDir 对象
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "项目目录不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 如果环境检测器已设置，先检查编译工具是否可用
    if (m_envDetector) {
        if (!m_envDetector->isToolAvailable("cmake")) {  // 检查 cmake 是否可用
            result["success"] = false;  // 不可用则标记失败
            result["error"] = "CMake 不可用，请确保已安装并添加到系统PATH中";  // 设置错误信息
            return result;  // 返回错误结果
        }

        // 获取可用的构建工具列表
        QStringList buildTools = m_envDetector->availableBuildTools();
        if (buildTools.isEmpty()) {  // 如果没有可用的构建工具
            result["success"] = false;  // 标记失败
            result["error"] = "没有可用的构建工具(Ninja/Make)，请安装构建工具";  // 设置错误信息
            return result;  // 返回错误结果
        }

        // 如果指定的生成器不可用，回退到第一个可用的工具
        if (!m_envDetector->isToolAvailable(generator.toLower())) {
            QString fallback = buildTools.first();  // 获取第一个可用的构建工具
            qWarning() << QString("Generator %1 not available, falling back to %2").arg(generator, fallback);  // 输出警告
            generator = fallback;  // 使用回退工具
        }
    }

    // 创建 build 目录
    QDir buildDir(path + "/build");
    if (!buildDir.exists()) {  // 如果 build 目录不存在
        buildDir.mkpath(".");  // 创建 build 目录
    }

    // 构建 cmake 配置命令
    QString cmakeCommand = QString("cmake -S %1 -B %2/build -G \"%3\" -DCMAKE_BUILD_TYPE=%4")
        .arg(path, path, generator, buildType);  // 格式化命令字符串

    // 启动 cmake 配置进程
    QProcess cmakeProcess;
    cmakeProcess.setWorkingDirectory(path);  // 设置工作目录为项目目录
#ifdef Q_OS_WIN  // 如果是 Windows 系统
    cmakeProcess.start("cmd.exe", QStringList() << "/c" << cmakeCommand);  // 使用 cmd.exe 执行命令
#else  // 如果是 Linux/macOS 系统
    cmakeProcess.start("/bin/sh", QStringList() << "-c" << cmakeCommand);  // 使用 sh 执行命令
#endif

    // 等待 cmake 进程启动
    if (!cmakeProcess.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "CMake 启动失败";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待 cmake 配置完成，超时时间为 120 秒
    if (!cmakeProcess.waitForFinished(120000)) {
        cmakeProcess.kill();  // 超时则强制终止进程
        result["success"] = false;  // 标记失败
        result["error"] = "CMake 配置超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 读取 cmake 的标准输出和标准错误
    QString cmakeOutput = QString::fromLocal8Bit(cmakeProcess.readAllStandardOutput());
    QString cmakeError = QString::fromLocal8Bit(cmakeProcess.readAllStandardError());

    // 检查 cmake 退出码
    if (cmakeProcess.exitCode() != 0) {  // 如果退出码非 0，表示配置失败
        result["success"] = false;  // 标记失败
        result["error"] = cmakeError.isEmpty() ? "CMake 配置失败" : cmakeError;  // 返回错误信息
        return result;  // 返回错误结果
    }

    // 根据生成器选择构建命令
    QString buildCommand;
    if (generator.contains("Ninja")) {  // 如果使用 Ninja
        buildCommand = "ninja";  // Ninja 构建命令
    } else if (generator.contains("Makefiles")) {  // 如果使用 Makefiles
        buildCommand = "make -j4";  // make 并行构建（4 个线程）
    } else {
        buildCommand = "cmake --build " + path + "/build";  // 通用 cmake 构建命令
    }

    // 启动构建进程
    QProcess buildProcess;
    buildProcess.setWorkingDirectory(path + "/build");  // 设置工作目录为 build 目录
#ifdef Q_OS_WIN
    buildProcess.start("cmd.exe", QStringList() << "/c" << buildCommand);
#else
    buildProcess.start("/bin/sh", QStringList() << "-c" << buildCommand);
#endif

    // 等待构建进程启动
    if (!buildProcess.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "构建工具启动失败";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待构建完成，超时时间为 300 秒（5 分钟）
    if (!buildProcess.waitForFinished(300000)) {
        buildProcess.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "编译超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 读取构建输出
    QString buildOutput = QString::fromLocal8Bit(buildProcess.readAllStandardOutput());
    QString buildError = QString::fromLocal8Bit(buildProcess.readAllStandardError());

    // 检查构建退出码
    if (buildProcess.exitCode() != 0) {  // 如果退出码非 0，表示编译失败
        result["success"] = false;  // 标记失败
        result["error"] = buildError.isEmpty() ? "编译失败" : buildError;  // 返回错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "项目编译成功!\nCMake输出:\n" + cmakeOutput + "\n编译输出:\n" + buildOutput;  // 返回完整输出
    return result;  // 返回成功结果
}

// 【功能说明】运行程序
// 【参数说明】args - 包含 path（程序路径）、args（命令行参数数组）、workingDir（工作目录）
// 【返回值】若成功，result 字段包含程序输出；若失败，error 字段包含原因
QVariantMap CodeAgent::runProgram(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取程序路径
    QVariantList argsList = args.value("args").toList();  // 提取命令行参数列表
    QString workingDir = args.value("workingDir").toString();  // 提取工作目录

    // 将 QVariantList 转换为 QStringList
    QStringList programArgs;
    for (const QVariant& arg : argsList) {
        programArgs.append(arg.toString());
    }

    // 如果参数列表为空且路径中包含空格，尝试将路径按空格拆分
    if (programArgs.isEmpty() && path.contains(' ')) {
        QStringList parts = path.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);  // 按空白拆分
        if (parts.size() > 1) {  // 如果拆分后有多部分
            path = parts.first();  // 第一部分作为程序路径
            for (int i = 1; i < parts.size(); ++i) {  // 剩余部分作为参数
                programArgs.append(parts[i]);
            }
        }
    }

    QProcess process;  // 创建 QProcess 对象
    if (!workingDir.isEmpty()) {  // 如果指定了工作目录
        process.setWorkingDirectory(workingDir);  // 设置工作目录
    }

#ifdef Q_OS_WIN  // Windows 平台特殊处理
    if (programArgs.isEmpty()) {
        process.start(path);  // 无参数启动
    } else {
        process.start(path, programArgs);  // 带参数启动
    }
#else  // Linux/macOS 平台
    process.start(path, programArgs);  // 统一带参数启动
#endif

    // 等待程序启动
    if (!process.waitForStarted()) {
        result["success"] = false;  // 启动失败
        result["error"] = "程序启动失败: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 等待程序执行完成，超时 60 秒
    if (!process.waitForFinished(60000)) {
        process.kill();  // 超时则强制终止
        result["success"] = false;  // 标记失败
        result["error"] = "程序运行超时";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 读取程序输出
    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(process.readAllStandardError());

    // 检查程序退出码
    if (process.exitCode() != 0) {  // 如果退出码非 0
        result["success"] = false;  // 标记失败
        result["error"] = error.isEmpty() ? "程序执行失败" : error;  // 返回错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = output.isEmpty() ? "程序执行成功，无输出" : output;  // 返回程序输出
    return result;  // 返回成功结果
}

// 【功能说明】调试代码（目前仅支持 Python 的 pdb）
// 【参数说明】args - 包含 path（代码文件路径）、breakpoints（断点列表）
// 【返回值】若成功，result 字段包含调试输出；若失败，error 字段包含原因
QVariantMap CodeAgent::debugCode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取代码文件路径

    QFileInfo fileInfo(path);  // 获取文件信息
    QString ext = fileInfo.suffix().toLower();  // 获取文件后缀并转为小写

    // 如果环境检测器已设置，检查 Python 是否可用
    if (m_envDetector && !m_envDetector->isToolAvailable("python")) {
        result["success"] = false;  // Python 不可用
        result["error"] = "Python 不可用，无法调试 Python 代码";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 如果是 Python 文件，使用 pdb 调试
    if (ext == "py") {
        QProcess process;  // 创建 QProcess 对象
        process.start("python", QStringList() << "-m" << "pdb" << path);  // 启动 pdb 调试器
        if (!process.waitForStarted()) {  // 等待调试器启动
            result["success"] = false;  // 启动失败
            result["error"] = "调试器启动失败";  // 设置错误信息
            return result;  // 返回错误结果
        }

        process.waitForFinished(300000);  // 等待调试完成，超时 5 分钟
        QString output = QString::fromLocal8Bit(process.readAllStandardOutput());  // 读取标准输出
        QString error = QString::fromLocal8Bit(process.readAllStandardError());  // 读取标准错误

        if (process.exitCode() != 0) {  // 如果退出码非 0
            result["success"] = false;  // 标记失败
            result["error"] = error.isEmpty() ? "调试失败" : error;  // 返回错误信息
        } else {
            result["success"] = true;  // 标记成功
            result["result"] = output;  // 返回调试输出
        }
        return result;  // 返回结果
    }

    // 暂不支持其他语言的调试
    result["success"] = false;  // 标记失败
    result["error"] = "暂不支持该文件类型的调试";  // 设置错误信息
    return result;  // 返回错误结果
}

// 【功能说明】分析单个代码文件（统计行数、查找类/函数/变量）
// 【参数说明】args - 包含 path（文件路径）、language（编程语言，默认 cpp）
// 【返回值】JSON 格式的分析结果，包含行数、字符数、类列表、函数列表、变量列表
QVariantMap CodeAgent::analyzeCode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径
    QString language = args.value("language", "cpp").toString().toLower();  // 提取语言并转为小写，默认 cpp

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试以只读文本模式打开
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流
    QString content = in.readAll();  // 读取文件全部内容
    file.close();  // 关闭文件

    // 构建分析结果 JSON 对象
    QJsonObject analysis;
    analysis["file"] = path;  // 记录文件路径
    analysis["language"] = language;  // 记录编程语言
    analysis["lineCount"] = content.count('\n') + 1;  // 统计行数（换行符数 + 1）
    analysis["charCount"] = content.length();  // 统计字符数
    analysis["size"] = QFileInfo(path).size();  // 获取文件大小（字节）

    QStringList classes, functions, variables;  // 创建字符串列表存储发现的类、函数、变量

    if (language == "cpp" || language == "c") {  // 分析 C/C++ 代码
        // 使用正则表达式匹配类定义：class 后面跟类名
        QRegularExpression classRe("class\\s+(\\w+)");
        // 使用正则表达式匹配函数定义：返回类型 + 函数名 + 左括号
        QRegularExpression funcRe("(void|int|float|double|bool|QString|QVariant|\\w+)\\s+(\\w+)\\s*\\(");
        // 使用正则表达式匹配变量定义：可选 const + 类型 + 变量名 + = 或 ;
        QRegularExpression varRe("(const\\s+)?(\\w+)\\s+(\\w+)\\s*[=;]");

        // 遍历所有类匹配
        QRegularExpressionMatchIterator classIt = classRe.globalMatch(content);
        while (classIt.hasNext()) {
            QRegularExpressionMatch m = classIt.next();
            if (!classes.contains(m.captured(1))) classes.append(m.captured(1));  // 去重添加类名
        }

        // 遍历所有函数匹配
        QRegularExpressionMatchIterator funcIt = funcRe.globalMatch(content);
        while (funcIt.hasNext()) {
            QRegularExpressionMatch m = funcIt.next();
            if (!functions.contains(m.captured(2))) functions.append(m.captured(2));  // 去重添加函数名
        }

        // 遍历所有变量匹配
        QRegularExpressionMatchIterator varIt = varRe.globalMatch(content);
        while (varIt.hasNext()) {
            QRegularExpressionMatch m = varIt.next();
            QString varName = m.captured(3);  // 获取变量名
            // 去重添加，且排除与函数名或类名相同的标识符（减少误报）
            if (!variables.contains(varName) && !functions.contains(varName) && !classes.contains(varName)) {
                variables.append(varName);
            }
        }
    } else if (language == "python") {  // 分析 Python 代码
        QRegularExpression classRe("class\\s+(\\w+)");  // 匹配类定义
        QRegularExpression funcRe("def\\s+(\\w+)\\s*\\(");  // 匹配函数定义

        QRegularExpressionMatchIterator classIt = classRe.globalMatch(content);
        while (classIt.hasNext()) {
            QRegularExpressionMatch m = classIt.next();
            if (!classes.contains(m.captured(1))) classes.append(m.captured(1));
        }

        QRegularExpressionMatchIterator funcIt = funcRe.globalMatch(content);
        while (funcIt.hasNext()) {
            QRegularExpressionMatch m = funcIt.next();
            if (!functions.contains(m.captured(1))) functions.append(m.captured(1));
        }
    }

    // 将列表转换为 JSON 数组并放入分析结果
    analysis["classes"] = QJsonArray::fromStringList(classes);
    analysis["functions"] = QJsonArray::fromStringList(functions);
    analysis["variables"] = QJsonArray::fromStringList(variables);

    result["success"] = true;  // 标记成功
    result["result"] = QJsonDocument(analysis).toJson(QJsonDocument::Compact);  // 序列化为 JSON 字符串
    return result;  // 返回成功结果
}

// 【功能说明】分析项目结构（统计文件类型、数量等）
// 【参数说明】args - 包含 path（项目目录）
// 【返回值】JSON 格式的项目信息，包含总文件数、文件列表、各类型文件数量
QVariantMap CodeAgent::analyzeProject(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取项目目录路径

    QDir dir(path);  // 创建 QDir 对象
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "目录不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 构建项目信息 JSON 对象
    QJsonObject projectInfo;
    projectInfo["path"] = path;  // 记录项目路径
    projectInfo["name"] = dir.dirName();  // 记录目录名称（项目名）

    QJsonArray filesArray;  // 创建 JSON 数组存储文件列表
    // 创建目录迭代器，递归遍历所有文件
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    QStringList extensions;  // 存储所有文件扩展名
    QMap<QString, int> fileCountByType;  // 统计每种扩展名的文件数量

    // 遍历项目中的所有文件
    while (it.hasNext()) {
        it.next();  // 移动到下一个文件
        QFileInfo info = it.fileInfo();  // 获取文件信息
        QString ext = info.suffix().toLower();  // 获取文件后缀并转为小写
        if (!ext.isEmpty()) {  // 如果有后缀
            extensions.append(ext);  // 添加到扩展名列表
            fileCountByType[ext]++;  // 对应类型的计数加 1
        }

        // 构建文件信息 JSON 对象
        QJsonObject fileObj;
        fileObj["name"] = info.fileName();  // 文件名
        fileObj["path"] = info.absoluteFilePath();  // 绝对路径
        fileObj["size"] = info.size();  // 文件大小
        filesArray.append(fileObj);  // 添加到文件数组
    }

    projectInfo["totalFiles"] = filesArray.size();  // 记录总文件数
    projectInfo["files"] = filesArray;  // 记录文件列表

    // 构建文件类型统计 JSON 对象
    QJsonObject fileTypes;
    for (const QString& ext : fileCountByType.keys()) {  // 遍历所有类型
        fileTypes[ext] = fileCountByType[ext];  // 记录每种类型的数量
    }
    projectInfo["fileTypes"] = fileTypes;  // 放入项目信息

    result["success"] = true;  // 标记成功
    result["result"] = QJsonDocument(projectInfo).toJson(QJsonDocument::Compact);  // 序列化为 JSON 字符串
    return result;  // 返回成功结果
}

// 【功能说明】检查项目依赖（解析 CMakeLists.txt、requirements.txt、package.json 等）
// 【参数说明】args - 包含 path（项目目录）、type（项目类型：cpp/python/npm，默认 cpp）
// 【返回值】JSON 格式的依赖列表
QVariantMap CodeAgent::checkDependencies(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取项目目录路径
    QString type = args.value("type", "cpp").toString().toLower();  // 提取项目类型，默认 cpp

    QDir dir(path);  // 创建 QDir 对象
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "目录不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 构建依赖信息 JSON 对象
    QJsonObject depInfo;
    depInfo["projectType"] = type;  // 记录项目类型
    depInfo["path"] = path;  // 记录项目路径

    QJsonArray dependencies;  // 创建 JSON 数组存储依赖列表

    if (type == "cpp" || type == "c") {  // C/C++ 项目
        // 查找 CMakeLists.txt 文件
        QStringList cmakeFiles = dir.entryList(QStringList() << "CMakeLists.txt", QDir::Files);
        QStringList headerFiles = dir.entryList(QStringList() << "*.h" << "*.hpp", QDir::Files);  // 查找头文件

        // 解析 CMakeLists.txt 中的 find_package 依赖
        if (!cmakeFiles.isEmpty()) {
            QFile cmakeFile(dir.absoluteFilePath(cmakeFiles.first()));  // 打开第一个 CMakeLists.txt
            if (cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = cmakeFile.readAll();  // 读取全部内容
                cmakeFile.close();  // 关闭文件

                // 使用正则匹配 find_package(XXX 格式的依赖声明
                QRegularExpression findPkgRe("find_package\\s*\\(\\s*(\\w+)");
                QRegularExpressionMatchIterator it = findPkgRe.globalMatch(content);
                while (it.hasNext()) {
                    QRegularExpressionMatch m = it.next();
                    QJsonObject dep;
                    dep["name"] = m.captured(1);  // 依赖名称
                    dep["type"] = "cmake";  // 依赖类型
                    dependencies.append(dep);  // 添加到依赖列表
                }
            }
        }

        // 解析头文件中的 #include 依赖
        for (const QString& header : headerFiles) {
            QFile hFile(dir.absoluteFilePath(header));  // 打开头文件
            if (hFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = hFile.readAll();  // 读取全部内容
                hFile.close();  // 关闭文件

                // 匹配 #include <xxx> 或 #include "xxx" 格式的包含语句
                QRegularExpression includeRe("#include\\s*[<\"](\\w+)[>\"]");
                QRegularExpressionMatchIterator it = includeRe.globalMatch(content);
                while (it.hasNext()) {
                    QRegularExpressionMatch m = it.next();
                    QString include = m.captured(1);  // 获取包含的文件名
                    if (!include.startsWith(".")) {  // 排除相对路径包含（如 ./xxx）
                        QJsonObject dep;
                        dep["name"] = include;  // 依赖名称
                        dep["type"] = "header";  // 依赖类型为头文件
                        dependencies.append(dep);  // 添加到依赖列表
                    }
                }
            }
        }
    } else if (type == "python") {  // Python 项目
        // 解析 requirements.txt
        QFile requirementsFile(dir.absoluteFilePath("requirements.txt"));
        if (requirementsFile.exists() && requirementsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = requirementsFile.readAll();  // 读取全部内容
            requirementsFile.close();  // 关闭文件

            QStringList lines = content.split('\n');  // 按行分割
            for (const QString& line : lines) {
                QString pkg = line.trimmed();  // 去除首尾空白
                if (!pkg.isEmpty() && !pkg.startsWith('#')) {  // 排除空行和注释行
                    // 去除版本号限制（如 >=、==、< 等），只保留包名
                    pkg = pkg.split('=').first().split('>').first().split('<').first();
                    QJsonObject dep;
                    dep["name"] = pkg;  // 包名
                    dep["type"] = "python";  // 依赖类型
                    dependencies.append(dep);  // 添加到依赖列表
                }
            }
        }
    } else if (type == "npm") {  // Node.js/npm 项目
        // 解析 package.json
        QFile packageFile(dir.absoluteFilePath("package.json"));
        if (packageFile.exists() && packageFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = packageFile.readAll();  // 读取全部内容
            packageFile.close();  // 关闭文件

            QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());  // 解析 JSON
            if (doc.isObject()) {  // 如果是 JSON 对象
                QJsonObject obj = doc.object();
                if (obj.contains("dependencies")) {  // 如果有 dependencies 字段
                    QJsonObject deps = obj["dependencies"].toObject();  // 获取依赖对象
                    for (const QString& key : deps.keys()) {  // 遍历所有依赖
                        QJsonObject dep;
                        dep["name"] = key;  // 包名
                        dep["version"] = deps[key].toString();  // 版本号
                        dep["type"] = "npm";  // 依赖类型
                        dependencies.append(dep);  // 添加到依赖列表
                    }
                }
            }
        }
    }

    depInfo["dependencies"] = dependencies;  // 将依赖列表放入依赖信息

    result["success"] = true;  // 标记成功
    result["result"] = QJsonDocument(depInfo).toJson(QJsonDocument::Compact);  // 序列化为 JSON 字符串
    return result;  // 返回成功结果
}

// 【功能说明】清理项目构建文件（删除 build 目录、中间文件等）
// 【参数说明】args - 包含 path（项目目录）
// 【返回值】清理结果字典，列出已删除的文件和目录
QVariantMap CodeAgent::cleanProject(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取项目目录路径

    QDir dir(path);  // 创建 QDir 对象
    if (!dir.exists()) {  // 检查目录是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "目录不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QStringList cleanedItems;  // 创建列表，记录已清理的条目

    // 定义要删除的构建目录列表
    QStringList buildDirs = {"build", "build-release", "build-debug", ".build", "cmake-build-debug", "cmake-build-release"};
    for (const QString& buildDir : buildDirs) {  // 遍历所有可能的构建目录名
        QString fullPath = dir.absoluteFilePath(buildDir);  // 获取绝对路径
        QDir d(fullPath);  // 创建 QDir 对象
        if (d.exists()) {  // 如果目录存在
            if (d.removeRecursively()) {  // 递归删除目录及其内容
                cleanedItems.append("删除目录: " + fullPath);  // 记录删除操作
            }
        }
    }

    // 定义要删除的构建相关文件列表
    QStringList filesToRemove = {"Makefile", "CMakeCache.txt", "compile_commands.json"};
    for (const QString& fileName : filesToRemove) {  // 遍历文件列表
        QString fullPath = dir.absoluteFilePath(fileName);  // 获取绝对路径
        QFile f(fullPath);  // 创建 QFile 对象
        if (f.exists()) {  // 如果文件存在
            if (f.remove()) {  // 删除文件
                cleanedItems.append("删除文件: " + fullPath);  // 记录删除操作
            }
        }
    }

    // 定义要删除的中间文件后缀列表
    QStringList suffixesToRemove = {".o", ".obj", ".exe", ".dll", ".so", ".a", ".lib", ".dylib", ".pdb", ".ilk"};
    // 递归遍历项目目录，查找并删除匹配后缀的文件
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();  // 移动到下一个文件
        QString ext = it.fileInfo().suffix().toLower();  // 获取文件后缀
        if (suffixesToRemove.contains(ext)) {  // 如果在待删除后缀列表中
            QString fullPath = it.fileInfo().absoluteFilePath();  // 获取绝对路径
            QFile f(fullPath);  // 创建 QFile 对象
            if (f.remove()) {  // 删除文件
                cleanedItems.append("删除文件: " + fullPath);  // 记录删除操作
            }
        }
    }

    // 构建返回结果
    if (cleanedItems.isEmpty()) {  // 如果没有清理任何内容
        result["success"] = true;  // 标记成功
        result["result"] = "项目已干净，没有需要清理的文件";  // 返回提示
    } else {  // 如果清理了文件
        result["success"] = true;  // 标记成功
        result["result"] = "清理完成，共清理 " + QString::number(cleanedItems.size()) + " 项:\n" + cleanedItems.join("\n");  // 返回清理清单
    }

    return result;  // 返回结果
}

// 【功能说明】格式化代码（调用 clang-format、black、prettier 等外部工具）
// 【参数说明】args - 包含 path（文件路径）、language（编程语言，默认 cpp）
// 【返回值】若成功，result 字段包含格式化成功提示；若失败，error 字段包含原因
QVariantMap CodeAgent::formatCode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径
    QString language = args.value("language", "cpp").toString().toLower();  // 提取语言并转为小写，默认 cpp

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString formatter;  // 格式化工具名称
    QStringList argsList;  // 格式化工具的参数列表

    // 根据语言选择对应的格式化工具
    if (language == "cpp" || language == "c") {
        formatter = "clang-format";  // C/C++ 使用 clang-format
        argsList << "-i" << path;  // -i 表示直接修改文件
    } else if (language == "python") {
        formatter = "python";  // Python 使用 black
        argsList << "-m" << "black" << path;
    } else if (language == "javascript" || language == "js") {
        formatter = "npx";  // JavaScript 使用 prettier
        argsList << "prettier" << "--write" << path;
    }

    // 如果没有找到对应的格式化工具
    if (formatter.isEmpty()) {
        result["success"] = false;  // 标记失败
        result["error"] = "暂不支持该语言的格式化";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 如果环境检测器已设置，检查格式化工具是否可用
    if (m_envDetector && !m_envDetector->isToolAvailable(formatter)) {
        result["success"] = false;  // 工具不可用
        result["error"] = QString("格式化工具 %1 不可用").arg(formatter);  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 启动格式化进程
    QProcess process;
    process.start(formatter, argsList);  // 启动格式化工具
    if (!process.waitForStarted()) {  // 等待启动
        result["success"] = false;  // 启动失败
        result["error"] = "格式化工具启动失败";  // 设置错误信息
        return result;  // 返回错误结果
    }

    process.waitForFinished(30000);  // 等待格式化完成，超时 30 秒
    QString error = QString::fromLocal8Bit(process.readAllStandardError());  // 读取标准错误

    // 检查退出码
    if (process.exitCode() != 0) {  // 如果退出码非 0
        result["success"] = false;  // 标记失败
        result["error"] = error.isEmpty() ? "格式化失败" : error;  // 返回错误信息
        return result;  // 返回错误结果
    }

    result["success"] = true;  // 标记成功
    result["result"] = "代码格式化完成: " + path;  // 返回成功提示
    return result;  // 返回成功结果
}

// 【功能说明】统计代码行数
// 【参数说明】args - 包含 path（目录或文件路径）、language（语言过滤，默认空表示不过滤）
// 【返回值】JSON 格式的行数统计结果，包含总行数、文件数、各类型行数
QVariantMap CodeAgent::countLines(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取路径
    QString language = args.value("language", "").toString().toLower();  // 提取语言过滤，默认空

    QFileInfo info(path);  // 获取路径信息
    if (!info.exists()) {  // 检查路径是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "路径不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QMap<QString, int> linesByType;  // 按文件类型统计行数
    int totalLines = 0;  // 总行数

    // 根据语言设置文件过滤器
    QStringList filters;
    if (language == "cpp" || language == "c") {
        filters << "*.cpp" << "*.h" << "*.hpp" << "*.cxx";  // C/C++ 文件后缀
    } else if (language == "python") {
        filters << "*.py";  // Python 文件后缀
    } else if (language == "javascript" || language == "js") {
        filters << "*.js" << "*.jsx" << "*.ts" << "*.tsx";  // JavaScript/TypeScript 文件后缀
    } else if (language == "qml") {
        filters << "*.qml";  // QML 文件后缀
    }

    if (info.isFile()) {  // 如果是单个文件
        QFile file(path);  // 创建 QFile 对象
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 打开文件
            QTextStream in(&file);  // 创建文本输入流
            QString content = in.readAll();  // 读取全部内容
            int lines = content.count('\n') + 1;  // 统计行数
            linesByType[info.suffix().toLower()] = lines;  // 按后缀记录行数
            totalLines += lines;  // 累加总行数
            file.close();  // 关闭文件
        }
    } else {  // 如果是目录
        // 创建目录迭代器，递归遍历匹配的文件
        QDirIterator it(path, filters.isEmpty() ? QStringList() << "*" : filters,
                        QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();  // 移动到下一个文件
            QFile file(it.fileInfo().absoluteFilePath());  // 打开文件
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                int lines = content.count('\n') + 1;  // 统计行数
                linesByType[it.fileInfo().suffix().toLower()] += lines;  // 按后缀累加行数
                totalLines += lines;  // 累加总行数
                file.close();  // 关闭文件
            }
        }
    }

    // 构建统计结果 JSON 对象
    QJsonObject stats;
    stats["path"] = path;  // 记录路径
    stats["totalLines"] = totalLines;  // 记录总行数
    stats["files"] = linesByType.size();  // 记录文件类型数

    QJsonObject byType;  // 各类型行数统计
    for (const QString& ext : linesByType.keys()) {
        byType[ext] = linesByType[ext];  // 记录每种后缀的行数
    }
    stats["byType"] = byType;  // 放入统计结果

    result["success"] = true;  // 标记成功
    result["result"] = QJsonDocument(stats).toJson(QJsonDocument::Compact);  // 序列化为 JSON 字符串
    return result;  // 返回成功结果
}

// 【功能说明】查找代码中的函数定义
// 【参数说明】args - 包含 path（文件路径）
// 【返回值】函数列表字符串，包含行号和函数名
QVariantMap CodeAgent::findFunctions(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流
    QStringList functions;  // 存储找到的函数信息
    int lineNum = 0;  // 当前行号

    // 定义多种语言的函数匹配正则表达式列表
    QList<QRegularExpression> patterns;
    // C++ 函数：可选访问修饰符/修饰符 + 返回类型 + 函数名 + 左括号
    patterns << QRegularExpression("^\\s*(?:public|private|protected|static|virtual|inline)?\\s*[\\w<>]+\\s+(\\w+)\\s*\\(");
    patterns << QRegularExpression("^\\s*def\\s+(\\w+)\\s*\\(");  // Python 函数
    patterns << QRegularExpression("^\\s*function\\s+(\\w+)\\s*\\(");  // JavaScript 函数
    patterns << QRegularExpression("^\\s*func\\s+(?:\\w+\\s*)?(\\w+)\\s*\\(");  // Swift/Go 函数
    patterns << QRegularExpression("^\\s*(?:async\\s+)?function\\s+(\\w+)\\s*\\(");  // 异步 JavaScript 函数

    // 逐行读取文件
    while (!in.atEnd()) {
        QString line = in.readLine();  // 读取一行
        lineNum++;  // 行号加 1

        // 尝试所有正则表达式模式
        for (const auto& regex : patterns) {
            QRegularExpressionMatch match = regex.match(line);  // 匹配当前行
            if (match.hasMatch()) {  // 如果匹配成功
                QString funcName = match.captured(1);  // 获取函数名（第一个捕获组）
                functions << QString("第 %1 行: %2").arg(lineNum).arg(funcName);  // 记录行号和函数名
                break;  // 匹配到一个就跳出当前行的模式循环
            }
        }
    }
    file.close();  // 关闭文件

    result["success"] = true;  // 标记成功
    if (functions.isEmpty()) {  // 如果没有找到函数
        result["result"] = "未找到函数定义";  // 返回提示
    } else {  // 如果找到了函数
        result["result"] = QString("找到 %1 个函数:\n%2").arg(functions.size()).arg(functions.join("\n"));  // 返回列表
    }
    return result;  // 返回结果
}

// 【功能说明】查找代码中的类定义
// 【参数说明】args - 包含 path（文件路径）
// 【返回值】类/结构列表字符串，包含行号和类名
QVariantMap CodeAgent::findClasses(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流
    QStringList classes;  // 存储找到的类/结构信息
    int lineNum = 0;  // 当前行号

    // 定义多种语言的类/结构匹配正则表达式列表
    QList<QRegularExpression> patterns;
    // C++/Java/C# 类：可选修饰符 + class + 类名
    patterns << QRegularExpression("^\\s*(?:public|private|protected|abstract|sealed|final)?\\s*class\\s+(\\w+)");
    patterns << QRegularExpression("^\\s*class\\s+(\\w+)\\s*:");  // Python 类（带继承）
    patterns << QRegularExpression("^\\s*interface\\s+(\\w+)");  // Java 接口
    patterns << QRegularExpression("^\\s*struct\\s+(\\w+)");  // C/C++ 结构体
    patterns << QRegularExpression("^\\s*type\\s+(\\w+)\\s+struct");  // Go 结构体

    // 逐行读取文件
    while (!in.atEnd()) {
        QString line = in.readLine();  // 读取一行
        lineNum++;  // 行号加 1

        // 尝试所有正则表达式模式
        for (const auto& regex : patterns) {
            QRegularExpressionMatch match = regex.match(line);  // 匹配当前行
            if (match.hasMatch()) {  // 如果匹配成功
                QString className = match.captured(1);  // 获取类名
                classes << QString("第 %1 行: class %2").arg(lineNum).arg(className);  // 记录行号和类名
                break;  // 匹配到一个就跳出当前行的模式循环
            }
        }
    }
    file.close();  // 关闭文件

    result["success"] = true;  // 标记成功
    if (classes.isEmpty()) {  // 如果没有找到类
        result["result"] = "未找到类定义";  // 返回提示
    } else {  // 如果找到了类
        result["result"] = QString("找到 %1 个类/结构:\n%2").arg(classes.size()).arg(classes.join("\n"));  // 返回列表
    }
    return result;  // 返回结果
}

// 【功能说明】查找代码中的 TODO/FIXME/XXX/HACK/BUG/OPTIMIZE 等标记
// 【参数说明】args - 包含 path（文件或目录路径）、recursive（是否递归，默认 true）
// 【返回值】标记列表字符串，包含文件名、行号和内容
QVariantMap CodeAgent::findTodos(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取路径
    bool recursive = args.value("recursive", true).toBool();  // 提取是否递归，默认 true

    QFileInfo info(path);  // 获取路径信息
    if (!info.exists()) {  // 检查路径是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "路径不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QStringList todos;  // 存储找到的标记信息

    // 定义 lambda 函数处理单个文件
    auto processFile = [&](const QString& filePath) {
        QFile file(filePath);  // 创建 QFile 对象
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;  // 打开失败则返回

        QTextStream in(&file);  // 创建文本输入流
        int lineNum = 0;  // 当前行号
        while (!in.atEnd()) {  // 逐行读取
            QString line = in.readLine();  // 读取一行
            lineNum++;  // 行号加 1
            // 检查是否包含 TODO/FIXME/XXX/HACK/BUG/OPTIMIZE 标记
            if (line.contains("TODO") || line.contains("FIXME") ||
                line.contains("XXX") || line.contains("HACK") ||
                line.contains("BUG") || line.contains("OPTIMIZE")) {
                // 记录文件名、行号和内容
                todos << QString("%1:第%2行: %3").arg(QFileInfo(filePath).fileName()).arg(lineNum).arg(line.trimmed());
            }
        }
        file.close();  // 关闭文件
    };

    if (info.isFile()) {  // 如果是单个文件
        processFile(path);  // 直接处理该文件
    } else {  // 如果是目录
        // 创建目录迭代器，根据 recursive 决定是否递归
        QDirIterator it(path, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            processFile(it.next());  // 处理目录中的每个文件
        }
    }

    result["success"] = true;  // 标记成功
    if (todos.isEmpty()) {  // 如果没有找到标记
        result["result"] = "未找到TODO/FIXME标记";  // 返回提示
    } else {  // 如果找到了标记
        result["result"] = QString("找到 %1 个标记:\n\n%2").arg(todos.size()).arg(todos.join("\n"));  // 返回列表
    }
    return result;  // 返回结果
}

// 【功能说明】检测文件编码格式（UTF-8、UTF-16、GBK 等）
// 【参数说明】args - 包含 path（文件路径）
// 【返回值】编码检测结果字符串，包含文件大小、检测到的编码和置信度
QVariantMap CodeAgent::detectEncoding(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly)) {  // 以二进制只读模式打开（不指定 Text，保留原始字节）
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QByteArray data = file.read(1024);  // 读取前 1024 字节用于检测
    file.close();  // 关闭文件

    QString encoding;  // 检测到的编码
    QString confidence;  // 置信度

    // 检查 BOM（字节顺序标记）
    if (data.startsWith("\xEF\xBB\xBF")) {  // UTF-8 BOM
        encoding = "UTF-8 (带BOM)";
        confidence = "高";
    } else if (data.startsWith("\xFF\xFE")) {  // UTF-16 LE BOM
        encoding = "UTF-16 LE (带BOM)";
        confidence = "高";
    } else if (data.startsWith("\xFE\xFF")) {  // UTF-16 BE BOM
        encoding = "UTF-16 BE (带BOM)";
        confidence = "高";
    } else if (data.startsWith("\x00\x00\xFE\xFF")) {  // UTF-32 BE BOM
        encoding = "UTF-32 BE (带BOM)";
        confidence = "高";
    } else {
        // 没有 BOM，尝试判断是否为 UTF-8
        bool isUtf8 = true;  // 假设是 UTF-8
        int i = 0;
        while (i < data.size()) {
            unsigned char c = static_cast<unsigned char>(data[i]);  // 获取当前字节
            if (c < 0x80) {  // 单字节 ASCII 字符（0xxxxxxx）
                i++;  // 继续下一个字节
            } else if ((c & 0xE0) == 0xC0) {  // 双字节字符（110xxxxx 10xxxxxx）
                // 检查下一个字节是否为有效的后续字节（10xxxxxx）
                if (i + 1 >= data.size() || (static_cast<unsigned char>(data[i+1]) & 0xC0) != 0x80) {
                    isUtf8 = false;  // 不符合 UTF-8 规则
                    break;
                }
                i += 2;  // 跳过两个字节
            } else if ((c & 0xF0) == 0xE0) {  // 三字节字符（1110xxxx 10xxxxxx 10xxxxxx）
                // 检查后续两个字节
                if (i + 2 >= data.size() ||
                    (static_cast<unsigned char>(data[i+1]) & 0xC0) != 0x80 ||
                    (static_cast<unsigned char>(data[i+2]) & 0xC0) != 0x80) {
                    isUtf8 = false;
                    break;
                }
                i += 3;  // 跳过三个字节
            } else {
                isUtf8 = false;  // 不符合 UTF-8 规则（四字节及以上暂不支持详细检测）
                break;
            }
        }

        if (isUtf8) {  // 如果是有效的 UTF-8
            encoding = "UTF-8 (无BOM)";
            confidence = "中";
        } else {  // 不是 UTF-8，可能是其他编码
            encoding = "未知编码（可能是GBK/GB2312/ISO-8859-1等）";
            confidence = "低";
        }
    }

    // 构建检测结果字符串
    QFileInfo fileInfo(path);
    QString resultStr = QString(
        "文件: %1\n"
        "大小: %2 字节\n"
        "检测编码: %3\n"
        "置信度: %4"
    ).arg(fileInfo.fileName()).arg(fileInfo.size()).arg(encoding).arg(confidence);

    result["success"] = true;  // 标记成功
    result["result"] = resultStr;  // 返回检测结果
    return result;  // 返回结果
}

// 【功能说明】统计代码注释比例
// 【参数说明】args - 包含 path（文件路径）
// 【返回值】注释统计结果字符串，包含总行数、代码行、注释行、空白行、注释占比
QVariantMap CodeAgent::commentRatio(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流
    int totalLines = 0;  // 总行数
    int commentLines = 0;  // 注释行数
    int codeLines = 0;  // 代码行数
    int blankLines = 0;  // 空白行数
    bool inBlockComment = false;  // 是否在块注释中（/* ... */、"""..."""、'''...'''）

    while (!in.atEnd()) {  // 逐行读取
        QString line = in.readLine().trimmed();  // 读取一行并去除首尾空白
        totalLines++;  // 总行数加 1

        if (line.isEmpty()) {  // 如果是空白行
            blankLines++;  // 空白行数加 1
            continue;  // 跳过后续判断
        }

        bool isComment = false;  // 标记当前行是否为注释

        if (inBlockComment) {  // 如果在块注释中
            isComment = true;  // 当前行是注释
            if (line.contains("*/")) {  // 如果包含块注释结束标记
                inBlockComment = false;  // 退出块注释状态
            }
        } else if (line.startsWith("//") || line.startsWith("#") || line.startsWith("--")) {
            // 单行注释：C++ 的 //、Python 的 #、SQL 的 --
            isComment = true;
        } else if (line.startsWith("/*") || line.startsWith("\"\"\"") || line.startsWith("'''")) {
            // 块注释开始：C 的 /*、Python 的 """ 或 '''
            isComment = true;
            inBlockComment = true;  // 进入块注释状态
            // 检查是否在同一行结束
            if (line.endsWith("*/") || (line.size() > 3 && (line.endsWith("\"\"\"") || line.endsWith("'''")))) {
                inBlockComment = false;  // 同一行结束，退出块注释状态
            }
        }

        if (isComment) {  // 如果是注释行
            commentLines++;  // 注释行数加 1
        } else {  // 否则是代码行
            codeLines++;  // 代码行数加 1
        }
    }
    file.close();  // 关闭文件

    // 计算注释占比
    int totalEffective = codeLines + commentLines;  // 有效行数 = 代码行 + 注释行
    int ratio = totalEffective > 0 ? commentLines * 100 / totalEffective : 0;  // 注释占比（百分比）

    // 构建统计结果字符串
    QString resultStr = QString(
        "=== 注释比例统计 ===\n"
        "文件: %1\n"
        "总行数: %2\n"
        "代码行: %3\n"
        "注释行: %4\n"
        "空白行: %5\n"
        "注释占比: %6%"
    ).arg(QFileInfo(path).fileName()).arg(totalLines).arg(codeLines).arg(commentLines).arg(blankLines).arg(ratio);

    result["success"] = true;  // 标记成功
    result["result"] = resultStr;  // 返回统计结果
    return result;  // 返回结果
}

// 【功能说明】移除代码中的空白行
// 【参数说明】args - 包含 path（文件路径）、outputPath（输出路径，可选，默认覆盖原文件）
// 【返回值】处理结果字典，报告移除的空白行数
QVariantMap CodeAgent::removeBlankLines(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径
    QString outputPath = args.value("outputPath", "").toString();  // 提取输出路径，默认空

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream in(&file);  // 创建文本输入流
    QStringList lines;  // 存储非空白行
    int removedCount = 0;  // 移除的空白行计数

    while (!in.atEnd()) {  // 逐行读取
        QString line = in.readLine();  // 读取一行
        if (line.trimmed().isEmpty()) {  // 如果去除空白后为空
            removedCount++;  // 空白行计数加 1
        } else {  // 否则是非空白行
            lines << line;  // 添加到非空白行列表
        }
    }
    file.close();  // 关闭文件

    // 确定输出路径：如果未指定 outputPath，则覆盖原文件
    QString targetPath = outputPath.isEmpty() ? path : outputPath;
    QFile outFile(targetPath);  // 创建输出 QFile 对象
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 尝试打开输出文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法写入文件: " + targetPath;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream out(&outFile);  // 创建文本输出流
    for (const QString& line : lines) {  // 遍历所有非空白行
        out << line << "\n";  // 写入文件，每行后加换行
    }
    outFile.close();  // 关闭输出文件

    result["success"] = true;  // 标记成功
    result["result"] = QString("已移除 %1 个空白行，结果保存在: %2").arg(removedCount).arg(targetPath);  // 返回结果
    return result;  // 返回结果
}

// 【功能说明】生成单元测试代码模板
// 【参数说明】args - 包含 path（源文件路径）、language（编程语言，默认 cpp）
// 【返回值】若成功，result 字段包含测试文件路径；若失败，error 字段包含原因
QVariantMap CodeAgent::generateTests(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取源文件路径
    QString language = args.value("language", "cpp").toString().toLower();  // 提取语言，默认 cpp

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString content = QTextStream(&file).readAll();  // 读取全部内容
    file.close();  // 关闭文件

    QString testContent;  // 存储生成的测试代码
    QFileInfo fileInfo(path);  // 获取源文件信息

    if (language == "cpp") {  // 生成 C++ 测试代码（使用 gtest）
        QString className;  // 尝试提取类名
        QRegularExpression classRe("class\\s+(\\w+)");  // 匹配类定义
        QRegularExpressionMatch classMatch = classRe.match(content);
        if (classMatch.hasMatch()) {  // 如果找到类名
            className = classMatch.captured(1);  // 获取类名
        }

        // 使用 QString 模板生成 gtest 测试代码
        testContent = QString(
            "#include <gtest/gtest.h>\n"
            "#include \"%1\"\n\n"
            "TEST(%2Test, BasicFunctionality) {\n"
            "    // TODO: 添加测试用例\n"
            "    EXPECT_EQ(true, true);\n"
            "}\n\n"
            "int main(int argc, char **argv) {\n"
            "    ::testing::InitGoogleTest(&argc, argv);\n"
            "    return RUN_ALL_TESTS();\n"
            "}\n"
        ).arg(fileInfo.fileName()).arg(className.isEmpty() ? "Default" : className);  // 填充模板
    } else if (language == "python") {  // 生成 Python 测试代码（使用 unittest）
        QString baseName = fileInfo.baseName();  // 获取文件名（不含后缀）
        if (!baseName.isEmpty()) {
            baseName[0] = baseName[0].toUpper();  // 首字母大写，作为类名
        }
        testContent = QString(
            "import unittest\n"
            "from %1 import *\n\n"
            "class Test%2(unittest.TestCase):\n"
            "    \"\"\"测试用例类\"\"\"\n\n"
            "    def test_basic_functionality(self):\n"
            "        \"\"\"基础功能测试\"\"\"\n"
            "        # TODO: 添加测试用例\n"
            "        self.assertTrue(True)\n\n"
            "if __name__ == '__main__':\n"
            "    unittest.main()\n"
        ).arg(fileInfo.baseName()).arg(baseName);  // 填充模板
    } else {  // 不支持其他语言
        result["success"] = false;  // 标记失败
        result["error"] = "暂不支持该语言的测试生成";  // 设置错误信息
        return result;  // 返回错误结果
    }

    // 构建测试文件路径：在原文件同目录下，文件名加 _test 后缀
    QString testPath = fileInfo.path() + "/" + fileInfo.baseName() + "_test." + (language == "cpp" ? "cpp" : "py");
    QFile testFile(testPath);  // 创建测试文件
    if (testFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 尝试打开文件
        QTextStream out(&testFile);
        out << testContent;  // 写入测试代码
        testFile.close();  // 关闭文件
        result["success"] = true;  // 标记成功
        result["result"] = "测试文件生成成功: " + testPath;  // 返回成功提示
    } else {  // 如果创建失败
        result["success"] = false;  // 标记失败
        result["error"] = "无法创建测试文件: " + testPath;  // 设置错误信息
    }

    return result;  // 返回结果
}

// 【功能说明】重构代码（批量替换名称）
// 【参数说明】args - 包含 path（文件路径）、oldName（旧名称）、newName（新名称）
// 【返回值】重构结果字典，报告替换次数
QVariantMap CodeAgent::refactorCode(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取文件路径
    QString oldName = args.value("oldName").toString();  // 提取旧名称
    QString newName = args.value("newName").toString();  // 提取新名称

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件读取
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString content = QTextStream(&file).readAll();  // 读取全部内容
    file.close();  // 关闭文件

    int replacements = content.count(oldName);  // 统计旧名称出现的次数
    content.replace(oldName, newName);  // 批量替换所有出现的位置

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 重新以只写模式打开，覆盖原文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法写入文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QTextStream out(&file);
    out << content;  // 写入替换后的内容
    file.close();  // 关闭文件

    result["success"] = true;  // 标记成功
    result["result"] = QString("重构完成，共替换 %1 处 '%2' 为 '%3'").arg(replacements).arg(oldName).arg(newName);  // 返回结果
    return result;  // 返回结果
}

// 【功能说明】提取类接口头文件（生成纯虚接口）
// 【参数说明】args - 包含 path（源文件路径）、outputPath（输出头文件路径）
// 【返回值】若成功，result 字段包含输出文件路径；若失败，error 字段包含原因
QVariantMap CodeAgent::extractInterface(const QVariantMap& args)
{
    QVariantMap result;  // 创建结果字典
    QString path = args.value("path").toString();  // 提取源文件路径
    QString outputPath = args.value("outputPath").toString();  // 提取输出路径

    QFile file(path);  // 创建 QFile 对象
    if (!file.exists()) {  // 检查文件是否存在
        result["success"] = false;  // 不存在则标记失败
        result["error"] = "文件不存在: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  // 尝试打开文件
        result["success"] = false;  // 打开失败
        result["error"] = "无法打开文件: " + path;  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString content = QTextStream(&file).readAll();  // 读取全部内容
    file.close();  // 关闭文件

    // 使用正则表达式匹配类定义：class 类名 { 类体 }
    QRegularExpression classRe("class\\s+(\\w+)\\s*\\{([^\\}]*)\\}");
    QRegularExpressionMatch classMatch = classRe.match(content);

    if (!classMatch.hasMatch()) {  // 如果没有找到类定义
        result["success"] = false;  // 标记失败
        result["error"] = "未找到类定义";  // 设置错误信息
        return result;  // 返回错误结果
    }

    QString className = classMatch.captured(1);  // 获取类名
    QString classBody = classMatch.captured(2);  // 获取类体内容

    // 构建接口头文件内容
    QString interfaceContent;
    interfaceContent += QString("#ifndef I%1_H\n").arg(className.toUpper());  // 头文件保护宏
    interfaceContent += QString("#define I%1_H\n\n").arg(className.toUpper());
    interfaceContent += QString("class I%1 {\n").arg(className);  // 接口类声明
    interfaceContent += "public:\n";
    interfaceContent += QString("    virtual ~I%1() = default;\n\n").arg(className);  // 虚析构函数

    // 使用正则表达式匹配类中的方法定义
    QRegularExpression methodRe("(?:virtual\\s+)?([\\w<>\\s]+)\\s+(\\w+)\\s*\\([^)]*\\)\\s*(?:=\\s*0)?");
    QRegularExpressionMatchIterator it = methodRe.globalMatch(classBody);

    while (it.hasNext()) {  // 遍历所有匹配的方法
        QRegularExpressionMatch m = it.next();
        QString returnType = m.captured(1).trimmed();  // 获取返回类型
        QString methodName = m.captured(2);  // 获取方法名
        interfaceContent += QString("    virtual %1 %2() = 0;\n").arg(returnType).arg(methodName);  // 添加纯虚方法
    }

    interfaceContent += "};\n\n";  // 类结束
    interfaceContent += QString("#endif // I%1_H\n").arg(className.toUpper());  // 头文件保护宏结束

    // 写入输出文件
    QFile outFile(outputPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {  // 尝试打开输出文件
        QTextStream out(&outFile);
        out << interfaceContent;  // 写入接口内容
        outFile.close();  // 关闭文件
        result["success"] = true;  // 标记成功
        result["result"] = "接口文件生成成功: " + outputPath;  // 返回成功提示
    } else {  // 如果创建失败
        result["success"] = false;  // 标记失败
        result["error"] = "无法创建接口文件: " + outputPath;  // 设置错误信息
    }

    return result;  // 返回结果
}
