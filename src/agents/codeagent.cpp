#include "codeagent.h"
#include "tool.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QProcess>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

CodeAgent::CodeAgent(QObject *parent)
    : Agent(parent),
      m_envDetector(nullptr)
{
    initializeTools();
}

CodeAgent::~CodeAgent()
{
    qDeleteAll(m_tools);
}

void CodeAgent::setEnvironmentDetector(EnvironmentDetector* detector)
{
    m_envDetector = detector;
}

QString CodeAgent::name() const
{
    return "CodeAgent";
}

QString CodeAgent::description() const
{
    return "代码开发Agent，负责项目创建、代码生成、编译和运行";
}

Agent::AgentType CodeAgent::type() const
{
    return CodeAgentType;
}

QList<Tool*> CodeAgent::tools() const
{
    return m_tools;
}

void CodeAgent::initializeTools()
{
    Tool* createProjectTool = new Tool("createProject", "创建项目模板");
    createProjectTool->addParameter("path", "string", "项目路径", true);
    createProjectTool->addParameter("type", "string", "项目类型(c/cpp/cmake/qt/python)", true);
    createProjectTool->addParameter("name", "string", "项目名称", true);
    m_tools.append(createProjectTool);

    Tool* generateCodeTool = new Tool("generateCode", "生成代码文件");
    generateCodeTool->addParameter("path", "string", "文件路径", true);
    generateCodeTool->addParameter("content", "string", "代码内容", true);
    generateCodeTool->addParameter("language", "string", "编程语言(c/cpp/python/java/js)", false, "cpp");
    m_tools.append(generateCodeTool);

    Tool* generateCMakeTool = new Tool("generateCMake", "生成CMakeLists.txt");
    generateCMakeTool->addParameter("path", "string", "CMakeLists.txt路径", true);
    generateCMakeTool->addParameter("projectName", "string", "项目名称", true);
    generateCMakeTool->addParameter("sources", "array", "源文件列表", true);
    generateCMakeTool->addParameter("targetName", "string", "目标可执行文件名称", false);
    generateCMakeTool->addParameter("requiresQt", "bool", "是否需要Qt", false, "false");
    m_tools.append(generateCMakeTool);

    Tool* generateQtProjectTool = new Tool("generateQtProject", "生成Qt项目文件");
    generateQtProjectTool->addParameter("path", "string", "项目目录", true);
    generateQtProjectTool->addParameter("projectName", "string", "项目名称", true);
    generateQtProjectTool->addParameter("sources", "array", "源文件列表", true);
    generateQtProjectTool->addParameter("forms", "array", "UI文件列表", false);
    generateQtProjectTool->addParameter("resources", "array", "资源文件列表", false);
    m_tools.append(generateQtProjectTool);

    Tool* compileProjectTool = new Tool("compileProject", "编译项目");
    compileProjectTool->addParameter("path", "string", "项目目录", true);
    compileProjectTool->addParameter("buildType", "string", "构建类型(Debug/Release)", false, "Release");
    compileProjectTool->addParameter("generator", "string", "构建生成器(Ninja/Makefiles)", false, "Ninja");
    m_tools.append(compileProjectTool);

    Tool* runProgramTool = new Tool("runProgram", "运行程序");
    runProgramTool->addParameter("path", "string", "程序路径", true);
    runProgramTool->addParameter("args", "array", "命令行参数", false);
    runProgramTool->addParameter("workingDir", "string", "工作目录", false);
    m_tools.append(runProgramTool);

    Tool* debugCodeTool = new Tool("debugCode", "调试代码");
    debugCodeTool->addParameter("path", "string", "代码文件路径", true);
    debugCodeTool->addParameter("breakpoints", "array", "断点列表", false);
    m_tools.append(debugCodeTool);

    Tool* analyzeCodeTool = new Tool("analyzeCode", "分析代码文件");
    analyzeCodeTool->addParameter("path", "string", "代码文件路径", true);
    analyzeCodeTool->addParameter("language", "string", "编程语言(cpp/python/js)", false, "cpp");
    m_tools.append(analyzeCodeTool);

    Tool* analyzeProjectTool = new Tool("analyzeProject", "分析项目结构");
    analyzeProjectTool->addParameter("path", "string", "项目目录", true);
    m_tools.append(analyzeProjectTool);

    Tool* checkDependenciesTool = new Tool("checkDependencies", "检查项目依赖");
    checkDependenciesTool->addParameter("path", "string", "项目目录", true);
    checkDependenciesTool->addParameter("type", "string", "项目类型(cpp/python/npm/go)", false, "cpp");
    m_tools.append(checkDependenciesTool);

    Tool* cleanProjectTool = new Tool("cleanProject", "清理项目构建文件");
    cleanProjectTool->addParameter("path", "string", "项目目录", true);
    m_tools.append(cleanProjectTool);

    Tool* formatCodeTool = new Tool("formatCode", "格式化代码");
    formatCodeTool->addParameter("path", "string", "代码文件路径", true);
    formatCodeTool->addParameter("language", "string", "编程语言(cpp/python/js)", false, "cpp");
    m_tools.append(formatCodeTool);

    Tool* countLinesTool = new Tool("countLines", "统计代码行数");
    countLinesTool->addParameter("path", "string", "项目目录或文件路径", true);
    countLinesTool->addParameter("language", "string", "语言过滤(cpp/python/js)", false, "");
    m_tools.append(countLinesTool);
}

QVariantMap CodeAgent::checkEnvironment(const QString& requiredTool)
{
    QVariantMap result;
    if (!m_envDetector) {
        return result;
    }

    if (!m_envDetector->isToolAvailable(requiredTool)) {
        result["success"] = false;
        result["error"] = QString("工具 %1 不可用，请确保已安装并添加到系统PATH中").arg(requiredTool);
        return result;
    }

    result["success"] = true;
    return result;
}

QVariantMap CodeAgent::executeTool(const QString& toolName, const QVariantMap& args)
{
    if (toolName == "createProject") {
        return createProject(args);
    } else if (toolName == "generateCode") {
        return generateCode(args);
    } else if (toolName == "generateCMake") {
        return generateCMake(args);
    } else if (toolName == "generateQtProject") {
        return generateQtProject(args);
    } else if (toolName == "compileProject") {
        return compileProject(args);
    } else if (toolName == "runProgram") {
        return runProgram(args);
    } else if (toolName == "debugCode") {
        return debugCode(args);
    } else if (toolName == "analyzeCode") {
        return analyzeCode(args);
    } else if (toolName == "analyzeProject") {
        return analyzeProject(args);
    } else if (toolName == "checkDependencies") {
        return checkDependencies(args);
    } else if (toolName == "cleanProject") {
        return cleanProject(args);
    } else if (toolName == "formatCode") {
        return formatCode(args);
    } else if (toolName == "countLines") {
        return countLines(args);
    }

    QVariantMap result;
    result["success"] = false;
    result["error"] = "Unknown tool: " + toolName;
    return result;
}

QVariantMap CodeAgent::createProject(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString type = args.value("type").toString().toLower();
    QString name = args.value("name").toString();

    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            result["success"] = false;
            result["error"] = "无法创建目录: " + path;
            return result;
        }
    }

    QString fullPath = dir.absolutePath();

    if (type == "c" || type == "cpp") {
        QFile mainFile(fullPath + "/main.cpp");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << "#include <iostream>\n\n";
            out << "int main() {\n";
            out << "    std::cout << \"Hello, \" << " << name << " << \"!\" << std::endl;\n";
            out << "    return 0;\n";
            out << "}\n";
            mainFile.close();
        }

        QFile cmakeFile(fullPath + "/CMakeLists.txt");
        if (cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cmakeFile);
            out << "cmake_minimum_required(VERSION 3.16)\n";
            out << "project(" << name << ")\n\n";
            out << "set(CMAKE_CXX_STANDARD 17)\n";
            out << "add_executable(" << name << " main.cpp)\n";
            cmakeFile.close();
        }
    } else if (type == "python") {
        QFile mainFile(fullPath + "/main.py");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << "print(\"Hello, " << name << "!\")\n";
            mainFile.close();
        }

        QFile initFile(fullPath + "/__init__.py");
        initFile.open(QIODevice::WriteOnly | QIODevice::Text);
        initFile.close();
    } else if (type == "cmake") {
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

        QFile cmakeFile(fullPath + "/CMakeLists.txt");
        if (cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&cmakeFile);
            out << "cmake_minimum_required(VERSION 3.16)\n";
            out << "project(" << name << " LANGUAGES CXX)\n\n";
            out << "set(CMAKE_CXX_STANDARD 17)\n";
            out << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
            out << "add_executable(" << name << "\n";
            out << "    main.cpp\n";
            out << ")\n\n";
            out << "install(TARGETS " << name << " DESTINATION bin)\n";
            cmakeFile.close();
        }

        QDir buildDir(fullPath + "/build");
        buildDir.mkpath(".");
    } else if (type == "qt") {
        QFile mainFile(fullPath + "/main.cpp");
        if (mainFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&mainFile);
            out << "#include <QApplication>\n";
            out << "#include <QLabel>\n\n";
            out << "int main(int argc, char *argv[]) {\n";
            out << "    QApplication app(argc, argv);\n";
            out << "    QLabel label(\"Hello, " << name << "!\");\n";
            out << "    label.show();\n";
            out << "    return app.exec();\n";
            out << "}\n";
            mainFile.close();
        }

        QFile proFile(fullPath + "/" + name + ".pro");
        if (proFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&proFile);
            out << "QT += core gui widgets\n\n";
            out << "TARGET = " << name << "\n";
            out << "TEMPLATE = app\n\n";
            out << "SOURCES += main.cpp\n";
            proFile.close();
        }
    }

    result["success"] = true;
    result["result"] = "项目创建成功: " + fullPath;
    return result;
}

QVariantMap CodeAgent::generateCode(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString content = args.value("content").toString();

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + path;
        return result;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    result["success"] = true;
    result["result"] = "代码文件生成成功: " + path;
    return result;
}

QVariantMap CodeAgent::generateCMake(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString projectName = args.value("projectName").toString();
    QVariantList sources = args.value("sources").toList();
    QString targetName = args.value("targetName", projectName).toString();
    bool requiresQt = args.value("requiresQt", false).toBool();

    QStringList sourceList;
    for (const QVariant& src : sources) {
        sourceList.append(src.toString());
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + path;
        return result;
    }

    QTextStream out(&file);
    out << "cmake_minimum_required(VERSION 3.16)\n";
    out << "project(" << projectName << " LANGUAGES CXX)\n\n";
    out << "set(CMAKE_CXX_STANDARD 17)\n";
    out << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";

    if (requiresQt) {
        out << "find_package(Qt6 REQUIRED COMPONENTS Core Widgets)\n";
        out << "qt_standard_project_setup()\n\n";
    }

    out << "add_executable(" << targetName << "\n";
    for (const QString& src : sourceList) {
        out << "    " << src << "\n";
    }
    out << ")\n\n";

    if (requiresQt) {
        out << "target_link_libraries(" << targetName << " PRIVATE Qt6::Core Qt6::Widgets)\n";
    }

    out << "install(TARGETS " << targetName << " DESTINATION bin)\n";
    file.close();

    result["success"] = true;
    result["result"] = "CMakeLists.txt 生成成功: " + path;
    return result;
}

QVariantMap CodeAgent::generateQtProject(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString projectName = args.value("projectName").toString();
    QVariantList sources = args.value("sources").toList();
    QVariantList forms = args.value("forms").toList();
    QVariantList resources = args.value("resources").toList();

    QStringList sourceList;
    for (const QVariant& src : sources) {
        sourceList.append(src.toString());
    }

    QString qmakePath = path + "/" + projectName + ".pro";
    QFile proFile(qmakePath);
    if (!proFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法创建 .pro 文件: " + qmakePath;
        return result;
    }

    QTextStream out(&proFile);
    out << "QT += core gui widgets\n\n";
    out << "TARGET = " << projectName << "\n";
    out << "TEMPLATE = app\n\n";
    out << "CONFIG += c++17\n\n";
    out << "SOURCES += \\\n";
    for (int i = 0; i < sourceList.size(); ++i) {
        out << "    " << sourceList[i];
        if (i < sourceList.size() - 1) out << " \\\n";
    }
    out << "\n\n";

    if (!forms.isEmpty()) {
        out << "FORMS += \\\n";
        for (int i = 0; i < forms.size(); ++i) {
            out << "    " << forms[i].toString();
            if (i < forms.size() - 1) out << " \\\n";
        }
        out << "\n\n";
    }

    if (!resources.isEmpty()) {
        out << "RESOURCES += \\\n";
        for (int i = 0; i < resources.size(); ++i) {
            out << "    " << resources[i].toString();
            if (i < resources.size() - 1) out << " \\\n";
        }
        out << "\n";
    }

    proFile.close();

    QString cmakePath = path + "/CMakeLists.txt";
    QFile cmakeFile(cmakePath);
    if (cmakeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream cmakeOut(&cmakeFile);
        cmakeOut << "cmake_minimum_required(VERSION 3.16)\n";
        cmakeOut << "project(" << projectName << " LANGUAGES CXX)\n\n";
        cmakeOut << "find_package(Qt6 REQUIRED COMPONENTS Core Widgets)\n";
        cmakeOut << "qt_standard_project_setup()\n\n";
        cmakeOut << "qt_add_executable(" << projectName << "\n";
        for (const QString& src : sourceList) {
            cmakeOut << "    " << src << "\n";
        }
        cmakeOut << ")\n\n";
        cmakeOut << "target_link_libraries(" << projectName << " PRIVATE Qt6::Core Qt6::Widgets)\n";
        cmakeOut << "install(TARGETS " << projectName << " DESTINATION bin)\n";
        cmakeFile.close();
    }

    result["success"] = true;
    result["result"] = "Qt项目文件生成成功: " + qmakePath;
    return result;
}

QVariantMap CodeAgent::compileProject(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString buildType = args.value("buildType", "Release").toString();
    QString generator = args.value("generator", "Ninja").toString();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "项目目录不存在: " + path;
        return result;
    }

    if (m_envDetector) {
        if (!m_envDetector->isToolAvailable("cmake")) {
            result["success"] = false;
            result["error"] = "CMake 不可用，请确保已安装并添加到系统PATH中";
            return result;
        }

        QStringList buildTools = m_envDetector->availableBuildTools();
        if (buildTools.isEmpty()) {
            result["success"] = false;
            result["error"] = "没有可用的构建工具(Ninja/Make)，请安装构建工具";
            return result;
        }

        if (!m_envDetector->isToolAvailable(generator.toLower())) {
            QString fallback = buildTools.first();
            qWarning() << QString("Generator %1 not available, falling back to %2").arg(generator, fallback);
            generator = fallback;
        }
    }

    QDir buildDir(path + "/build");
    if (!buildDir.exists()) {
        buildDir.mkpath(".");
    }

    QString cmakeCommand = QString("cmake -S %1 -B %2/build -G \"%3\" -DCMAKE_BUILD_TYPE=%4")
        .arg(path, path, generator, buildType);

    QProcess cmakeProcess;
    cmakeProcess.setWorkingDirectory(path);
#ifdef Q_OS_WIN
    cmakeProcess.start("cmd.exe", QStringList() << "/c" << cmakeCommand);
#else
    cmakeProcess.start("/bin/sh", QStringList() << "-c" << cmakeCommand);
#endif

    if (!cmakeProcess.waitForStarted()) {
        result["success"] = false;
        result["error"] = "CMake 启动失败";
        return result;
    }

    if (!cmakeProcess.waitForFinished(120000)) {
        cmakeProcess.kill();
        result["success"] = false;
        result["error"] = "CMake 配置超时";
        return result;
    }

    QString cmakeOutput = QString::fromLocal8Bit(cmakeProcess.readAllStandardOutput());
    QString cmakeError = QString::fromLocal8Bit(cmakeProcess.readAllStandardError());

    if (cmakeProcess.exitCode() != 0) {
        result["success"] = false;
        result["error"] = cmakeError.isEmpty() ? "CMake 配置失败" : cmakeError;
        return result;
    }

    QString buildCommand;
    if (generator.contains("Ninja")) {
        buildCommand = "ninja";
    } else if (generator.contains("Makefiles")) {
        buildCommand = "make -j4";
    } else {
        buildCommand = "cmake --build " + path + "/build";
    }

    QProcess buildProcess;
    buildProcess.setWorkingDirectory(path + "/build");
#ifdef Q_OS_WIN
    buildProcess.start("cmd.exe", QStringList() << "/c" << buildCommand);
#else
    buildProcess.start("/bin/sh", QStringList() << "-c" << buildCommand);
#endif

    if (!buildProcess.waitForStarted()) {
        result["success"] = false;
        result["error"] = "构建工具启动失败";
        return result;
    }

    if (!buildProcess.waitForFinished(300000)) {
        buildProcess.kill();
        result["success"] = false;
        result["error"] = "编译超时";
        return result;
    }

    QString buildOutput = QString::fromLocal8Bit(buildProcess.readAllStandardOutput());
    QString buildError = QString::fromLocal8Bit(buildProcess.readAllStandardError());

    if (buildProcess.exitCode() != 0) {
        result["success"] = false;
        result["error"] = buildError.isEmpty() ? "编译失败" : buildError;
        return result;
    }

    result["success"] = true;
    result["result"] = "项目编译成功!\nCMake输出:\n" + cmakeOutput + "\n编译输出:\n" + buildOutput;
    return result;
}

QVariantMap CodeAgent::runProgram(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QVariantList argsList = args.value("args").toList();
    QString workingDir = args.value("workingDir").toString();

    QStringList programArgs;
    for (const QVariant& arg : argsList) {
        programArgs.append(arg.toString());
    }

    if (programArgs.isEmpty() && path.contains(' ')) {
        QStringList parts = path.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() > 1) {
            path = parts.first();
            for (int i = 1; i < parts.size(); ++i) {
                programArgs.append(parts[i]);
            }
        }
    }

    QProcess process;
    if (!workingDir.isEmpty()) {
        process.setWorkingDirectory(workingDir);
    }

#ifdef Q_OS_WIN
    if (programArgs.isEmpty()) {
        process.start(path);
    } else {
        process.start(path, programArgs);
    }
#else
    process.start(path, programArgs);
#endif

    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "程序启动失败: " + path;
        return result;
    }

    if (!process.waitForFinished(60000)) {
        process.kill();
        result["success"] = false;
        result["error"] = "程序运行超时";
        return result;
    }

    QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
    QString error = QString::fromLocal8Bit(process.readAllStandardError());

    if (process.exitCode() != 0) {
        result["success"] = false;
        result["error"] = error.isEmpty() ? "程序执行失败" : error;
        return result;
    }

    result["success"] = true;
    result["result"] = output.isEmpty() ? "程序执行成功，无输出" : output;
    return result;
}

QVariantMap CodeAgent::debugCode(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QFileInfo fileInfo(path);
    QString ext = fileInfo.suffix().toLower();

    if (m_envDetector && !m_envDetector->isToolAvailable("python")) {
        result["success"] = false;
        result["error"] = "Python 不可用，无法调试 Python 代码";
        return result;
    }

    if (ext == "py") {
        QProcess process;
        process.start("python", QStringList() << "-m" << "pdb" << path);
        if (!process.waitForStarted()) {
            result["success"] = false;
            result["error"] = "调试器启动失败";
            return result;
        }

        process.waitForFinished(300000);
        QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
        QString error = QString::fromLocal8Bit(process.readAllStandardError());

        if (process.exitCode() != 0) {
            result["success"] = false;
            result["error"] = error.isEmpty() ? "调试失败" : error;
        } else {
            result["success"] = true;
            result["result"] = output;
        }
        return result;
    }

    result["success"] = false;
    result["error"] = "暂不支持该文件类型的调试";
    return result;
}

QVariantMap CodeAgent::analyzeCode(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString language = args.value("language", "cpp").toString().toLower();

    QFile file(path);
    if (!file.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result["success"] = false;
        result["error"] = "无法打开文件: " + path;
        return result;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    QJsonObject analysis;
    analysis["file"] = path;
    analysis["language"] = language;
    analysis["lineCount"] = content.count('\n') + 1;
    analysis["charCount"] = content.length();
    analysis["size"] = QFileInfo(path).size();

    QStringList classes, functions, variables;

    if (language == "cpp" || language == "c") {
        QRegularExpression classRe("class\\s+(\\w+)");
        QRegularExpression funcRe("(void|int|float|double|bool|QString|QVariant|\\w+)\\s+(\\w+)\\s*\\(");
        QRegularExpression varRe("(const\\s+)?(\\w+)\\s+(\\w+)\\s*[=;]");

        QRegularExpressionMatchIterator classIt = classRe.globalMatch(content);
        while (classIt.hasNext()) {
            QRegularExpressionMatch m = classIt.next();
            if (!classes.contains(m.captured(1))) classes.append(m.captured(1));
        }

        QRegularExpressionMatchIterator funcIt = funcRe.globalMatch(content);
        while (funcIt.hasNext()) {
            QRegularExpressionMatch m = funcIt.next();
            if (!functions.contains(m.captured(2))) functions.append(m.captured(2));
        }

        QRegularExpressionMatchIterator varIt = varRe.globalMatch(content);
        while (varIt.hasNext()) {
            QRegularExpressionMatch m = varIt.next();
            QString varName = m.captured(3);
            if (!variables.contains(varName) && !functions.contains(varName) && !classes.contains(varName)) {
                variables.append(varName);
            }
        }
    } else if (language == "python") {
        QRegularExpression classRe("class\\s+(\\w+)");
        QRegularExpression funcRe("def\\s+(\\w+)\\s*\\(");

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

    analysis["classes"] = QJsonArray::fromStringList(classes);
    analysis["functions"] = QJsonArray::fromStringList(functions);
    analysis["variables"] = QJsonArray::fromStringList(variables);

    result["success"] = true;
    result["result"] = QJsonDocument(analysis).toJson(QJsonDocument::Compact);
    return result;
}

QVariantMap CodeAgent::analyzeProject(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + path;
        return result;
    }

    QJsonObject projectInfo;
    projectInfo["path"] = path;
    projectInfo["name"] = dir.dirName();

    QJsonArray filesArray;
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    QStringList extensions;
    QMap<QString, int> fileCountByType;

    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        QString ext = info.suffix().toLower();
        if (!ext.isEmpty()) {
            extensions.append(ext);
            fileCountByType[ext]++;
        }

        QJsonObject fileObj;
        fileObj["name"] = info.fileName();
        fileObj["path"] = info.absoluteFilePath();
        fileObj["size"] = info.size();
        filesArray.append(fileObj);
    }

    projectInfo["totalFiles"] = filesArray.size();
    projectInfo["files"] = filesArray;

    QJsonObject fileTypes;
    for (const QString& ext : fileCountByType.keys()) {
        fileTypes[ext] = fileCountByType[ext];
    }
    projectInfo["fileTypes"] = fileTypes;

    result["success"] = true;
    result["result"] = QJsonDocument(projectInfo).toJson(QJsonDocument::Compact);
    return result;
}

QVariantMap CodeAgent::checkDependencies(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString type = args.value("type", "cpp").toString().toLower();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + path;
        return result;
    }

    QJsonObject depInfo;
    depInfo["projectType"] = type;
    depInfo["path"] = path;

    QJsonArray dependencies;

    if (type == "cpp" || type == "c") {
        QStringList cmakeFiles = dir.entryList(QStringList() << "CMakeLists.txt", QDir::Files);
        QStringList headerFiles = dir.entryList(QStringList() << "*.h" << "*.hpp", QDir::Files);

        if (!cmakeFiles.isEmpty()) {
            QFile cmakeFile(dir.absoluteFilePath(cmakeFiles.first()));
            if (cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = cmakeFile.readAll();
                cmakeFile.close();

                QRegularExpression findPkgRe("find_package\\s*\\(\\s*(\\w+)");
                QRegularExpressionMatchIterator it = findPkgRe.globalMatch(content);
                while (it.hasNext()) {
                    QRegularExpressionMatch m = it.next();
                    QJsonObject dep;
                    dep["name"] = m.captured(1);
                    dep["type"] = "cmake";
                    dependencies.append(dep);
                }
            }
        }

        for (const QString& header : headerFiles) {
            QFile hFile(dir.absoluteFilePath(header));
            if (hFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = hFile.readAll();
                hFile.close();

                QRegularExpression includeRe("#include\\s*[<\"](\\w+)[>\"]");
                QRegularExpressionMatchIterator it = includeRe.globalMatch(content);
                while (it.hasNext()) {
                    QRegularExpressionMatch m = it.next();
                    QString include = m.captured(1);
                    if (!include.startsWith(".")) {
                        QJsonObject dep;
                        dep["name"] = include;
                        dep["type"] = "header";
                        dependencies.append(dep);
                    }
                }
            }
        }
    } else if (type == "python") {
        QFile requirementsFile(dir.absoluteFilePath("requirements.txt"));
        if (requirementsFile.exists() && requirementsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = requirementsFile.readAll();
            requirementsFile.close();

            QStringList lines = content.split('\n');
            for (const QString& line : lines) {
                QString pkg = line.trimmed();
                if (!pkg.isEmpty() && !pkg.startsWith('#')) {
                    pkg = pkg.split('=').first().split('>').first().split('<').first();
                    QJsonObject dep;
                    dep["name"] = pkg;
                    dep["type"] = "python";
                    dependencies.append(dep);
                }
            }
        }
    } else if (type == "npm") {
        QFile packageFile(dir.absoluteFilePath("package.json"));
        if (packageFile.exists() && packageFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = packageFile.readAll();
            packageFile.close();

            QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("dependencies")) {
                    QJsonObject deps = obj["dependencies"].toObject();
                    for (const QString& key : deps.keys()) {
                        QJsonObject dep;
                        dep["name"] = key;
                        dep["version"] = deps[key].toString();
                        dep["type"] = "npm";
                        dependencies.append(dep);
                    }
                }
            }
        }
    }

    depInfo["dependencies"] = dependencies;

    result["success"] = true;
    result["result"] = QJsonDocument(depInfo).toJson(QJsonDocument::Compact);
    return result;
}

QVariantMap CodeAgent::cleanProject(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();

    QDir dir(path);
    if (!dir.exists()) {
        result["success"] = false;
        result["error"] = "目录不存在: " + path;
        return result;
    }

    QStringList cleanedItems;

    QStringList buildDirs = {"build", "build-release", "build-debug", ".build", "cmake-build-debug", "cmake-build-release"};
    for (const QString& buildDir : buildDirs) {
        QString fullPath = dir.absoluteFilePath(buildDir);
        QDir d(fullPath);
        if (d.exists()) {
            if (d.removeRecursively()) {
                cleanedItems.append("删除目录: " + fullPath);
            }
        }
    }

    QStringList filesToRemove = {"Makefile", "CMakeCache.txt", "compile_commands.json"};
    for (const QString& fileName : filesToRemove) {
        QString fullPath = dir.absoluteFilePath(fileName);
        QFile f(fullPath);
        if (f.exists()) {
            if (f.remove()) {
                cleanedItems.append("删除文件: " + fullPath);
            }
        }
    }

    QStringList suffixesToRemove = {".o", ".obj", ".exe", ".dll", ".so", ".a", ".lib", ".dylib", ".pdb", ".ilk"};
    QDirIterator it(path, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString ext = it.fileInfo().suffix().toLower();
        if (suffixesToRemove.contains(ext)) {
            QString fullPath = it.fileInfo().absoluteFilePath();
            QFile f(fullPath);
            if (f.remove()) {
                cleanedItems.append("删除文件: " + fullPath);
            }
        }
    }

    if (cleanedItems.isEmpty()) {
        result["success"] = true;
        result["result"] = "项目已干净，没有需要清理的文件";
    } else {
        result["success"] = true;
        result["result"] = "清理完成，共清理 " + QString::number(cleanedItems.size()) + " 项:\n" + cleanedItems.join("\n");
    }

    return result;
}

QVariantMap CodeAgent::formatCode(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString language = args.value("language", "cpp").toString().toLower();

    QFile file(path);
    if (!file.exists()) {
        result["success"] = false;
        result["error"] = "文件不存在: " + path;
        return result;
    }

    QString formatter;
    QStringList argsList;

    if (language == "cpp" || language == "c") {
        formatter = "clang-format";
        argsList << "-i" << path;
    } else if (language == "python") {
        formatter = "python";
        argsList << "-m" << "black" << path;
    } else if (language == "javascript" || language == "js") {
        formatter = "npx";
        argsList << "prettier" << "--write" << path;
    }

    if (formatter.isEmpty()) {
        result["success"] = false;
        result["error"] = "暂不支持该语言的格式化";
        return result;
    }

    if (m_envDetector && !m_envDetector->isToolAvailable(formatter)) {
        result["success"] = false;
        result["error"] = QString("格式化工具 %1 不可用").arg(formatter);
        return result;
    }

    QProcess process;
    process.start(formatter, argsList);
    if (!process.waitForStarted()) {
        result["success"] = false;
        result["error"] = "格式化工具启动失败";
        return result;
    }

    process.waitForFinished(30000);
    QString error = QString::fromLocal8Bit(process.readAllStandardError());

    if (process.exitCode() != 0) {
        result["success"] = false;
        result["error"] = error.isEmpty() ? "格式化失败" : error;
        return result;
    }

    result["success"] = true;
    result["result"] = "代码格式化完成: " + path;
    return result;
}

QVariantMap CodeAgent::countLines(const QVariantMap& args)
{
    QVariantMap result;
    QString path = args.value("path").toString();
    QString language = args.value("language", "").toString().toLower();

    QFileInfo info(path);
    if (!info.exists()) {
        result["success"] = false;
        result["error"] = "路径不存在: " + path;
        return result;
    }

    QMap<QString, int> linesByType;
    int totalLines = 0;

    QStringList filters;
    if (language == "cpp" || language == "c") {
        filters << "*.cpp" << "*.h" << "*.hpp" << "*.cxx";
    } else if (language == "python") {
        filters << "*.py";
    } else if (language == "javascript" || language == "js") {
        filters << "*.js" << "*.jsx" << "*.ts" << "*.tsx";
    } else if (language == "qml") {
        filters << "*.qml";
    }

    if (info.isFile()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            int lines = content.count('\n') + 1;
            linesByType[info.suffix().toLower()] = lines;
            totalLines += lines;
            file.close();
        }
    } else {
        QDirIterator it(path, filters.isEmpty() ? QStringList() << "*" : filters, 
                        QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            QFile file(it.fileInfo().absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                int lines = content.count('\n') + 1;
                linesByType[it.fileInfo().suffix().toLower()] += lines;
                totalLines += lines;
                file.close();
            }
        }
    }

    QJsonObject stats;
    stats["path"] = path;
    stats["totalLines"] = totalLines;
    stats["files"] = linesByType.size();

    QJsonObject byType;
    for (const QString& ext : linesByType.keys()) {
        byType[ext] = linesByType[ext];
    }
    stats["byType"] = byType;

    result["success"] = true;
    result["result"] = QJsonDocument(stats).toJson(QJsonDocument::Compact);
    return result;
}