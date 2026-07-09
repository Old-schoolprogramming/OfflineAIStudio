#ifndef ENVIRONMENTDETECTOR_H
#define ENVIRONMENTDETECTOR_H

#include <QObject>
#include <QStringList>
#include <QMap>

/**
 * @brief 环境检测器 - 检测系统中可用的开发工具和环境
 *
 * 在离线环境中，必须在启动时检测哪些工具可用，避免调用不可用的工具。
 * 检测结果会影响Agent的行为：
 * - 如果cmake不可用，CodeAgent会跳过编译步骤并提示用户
 * - 如果pip不可用，不会尝试安装任何包
 * - 所有网络相关操作在离线模式下都应被禁用
 */
class EnvironmentDetector : public QObject
{
    Q_OBJECT

public:
    struct ToolInfo {
        QString name;
        QString path;
        QString version;
        bool available;
    };

    explicit EnvironmentDetector(QObject *parent = nullptr);

    void detect();
    const QMap<QString, ToolInfo>& tools() const;
    bool isToolAvailable(const QString& name) const;
    QString toolPath(const QString& name) const;
    bool isOfflineMode() const;
    QStringList availableCompilers() const;
    QStringList availableBuildTools() const;

signals:
    void detectionCompleted();

private:
    void detectTool(const QString& name, const QStringList& possibleNames);
    QString getToolVersion(const QString& path);

    QMap<QString, ToolInfo> m_tools;
    bool m_offlineMode;
};

#endif