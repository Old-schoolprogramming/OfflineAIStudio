#include "environmentdetector.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QNetworkInformation>

EnvironmentDetector::EnvironmentDetector(QObject *parent)
    : QObject(parent),
      m_offlineMode(false)
{
}

void EnvironmentDetector::detect()
{
    m_tools.clear();

    // 检测网络状态 (Qt 6 中 QNetworkConfigurationManager 已移除，改用 QNetworkInformation)
    QNetworkInformation::load(QNetworkInformation::Feature::Reachability);
    auto *netInfo = QNetworkInformation::instance();
    m_offlineMode = (netInfo && netInfo->reachability() == QNetworkInformation::Reachability::Disconnected);

    // 检测编译器
    detectTool("gcc", {"gcc", "gcc.exe"});
    detectTool("g++", {"g++", "g++.exe"});
    detectTool("clang", {"clang", "clang.exe"});
    detectTool("cl", {"cl.exe"});

    // 检测构建工具
    detectTool("cmake", {"cmake", "cmake.exe"});
    detectTool("make", {"make", "mingw32-make", "mingw64-make", "make.exe"});
    detectTool("ninja", {"ninja", "ninja.exe"});

    // 检测Qt工具
    detectTool("qmake", {"qmake", "qmake.exe"});
    detectTool("moc", {"moc", "moc.exe"});
    detectTool("uic", {"uic", "uic.exe"});

    // 检测Python
    detectTool("python", {"python", "python3", "python.exe", "python3.exe"});
    detectTool("pip", {"pip", "pip3", "pip.exe", "pip3.exe"});

    // 检测其他工具
    detectTool("git", {"git", "git.exe"});

    emit detectionCompleted();
}

void EnvironmentDetector::detectTool(const QString& name, const QStringList& possibleNames)
{
    ToolInfo info;
    info.name = name;
    info.available = false;

    for (const QString& toolName : possibleNames) {
        QProcess process;
#ifdef Q_OS_WIN
        process.start("where", QStringList() << toolName);
#else
        process.start("which", QStringList() << toolName);
#endif

        if (process.waitForFinished(3000)) {
            QString output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
            QString error = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();

            if (!output.isEmpty() && process.exitCode() == 0) {
                QStringList lines = output.split('\n');
                QString path = lines.first().trimmed();

                QFileInfo fileInfo(path);
                if (fileInfo.exists() && fileInfo.isFile()) {
                    info.path = path;
                    info.version = getToolVersion(path);
                    info.available = true;
                    break;
                }
            }
        }
    }

    m_tools[name] = info;
}

QString EnvironmentDetector::getToolVersion(const QString& path)
{
    QProcess process;

    QStringList args;
    QString toolName = QFileInfo(path).baseName().toLower();

    if (toolName.contains("gcc") || toolName.contains("g++") || toolName.contains("clang")) {
        args = {"--version"};
    } else if (toolName.contains("cmake")) {
        args = {"--version"};
    } else if (toolName.contains("ninja")) {
        args = {"--version"};
    } else if (toolName.contains("python")) {
        args = {"--version"};
    } else if (toolName.contains("qmake")) {
        args = {"--version"};
    } else if (toolName.contains("git")) {
        args = {"--version"};
    } else {
        return "";
    }

    process.start(path, args);
    if (process.waitForFinished(3000)) {
        QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
        QStringList lines = output.split('\n');
        if (!lines.isEmpty()) {
            return lines.first().trimmed();
        }
    }

    return "";
}

const QMap<QString, EnvironmentDetector::ToolInfo>& EnvironmentDetector::tools() const
{
    return m_tools;
}

bool EnvironmentDetector::isToolAvailable(const QString& name) const
{
    return m_tools.contains(name) && m_tools[name].available;
}

QString EnvironmentDetector::toolPath(const QString& name) const
{
    if (m_tools.contains(name) && m_tools[name].available) {
        return m_tools[name].path;
    }
    return "";
}

bool EnvironmentDetector::isOfflineMode() const
{
    return m_offlineMode;
}

QStringList EnvironmentDetector::availableCompilers() const
{
    QStringList compilers;
    if (isToolAvailable("gcc")) compilers << "gcc";
    if (isToolAvailable("g++")) compilers << "g++";
    if (isToolAvailable("clang")) compilers << "clang";
    if (isToolAvailable("cl")) compilers << "cl";
    return compilers;
}

QStringList EnvironmentDetector::availableBuildTools() const
{
    QStringList tools;
    if (isToolAvailable("cmake")) tools << "cmake";
    if (isToolAvailable("make")) tools << "make";
    if (isToolAvailable("ninja")) tools << "ninja";
    return tools;
}