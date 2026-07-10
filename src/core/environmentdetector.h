#ifndef ENVIRONMENTDETECTOR_H  // 【条件编译】如果未定义ENVIRONMENTDETECTOR_H，则继续编译，防止头文件重复包含
#define ENVIRONMENTDETECTOR_H  // 【宏定义】定义ENVIRONMENTDETECTOR_H宏，标记该头文件已被包含一次

#include <QObject>      // 【引入Qt对象基类】所有使用信号槽的类都必须继承自QObject
#include <QStringList>  // 【引入字符串列表类】存储一组QString
#include <QMap>         // 【引入映射容器类】键值对字典，类似std::map

/**
 * @brief 环境检测器 - 检测系统中可用的开发工具和环境
 *
 * 在离线环境中，必须在启动时检测哪些工具可用，避免调用不可用的工具。
 * 检测结果会影响Agent的行为：
 * - 如果cmake不可用，CodeAgent会跳过编译步骤并提示用户
 * - 如果pip不可用，不会尝试安装任何包
 * - 所有网络相关操作在离线模式下都应被禁用
 */
class EnvironmentDetector : public QObject  // 【类声明】继承QObject以支持信号槽机制
{
    Q_OBJECT  // 【Qt宏】启用元对象系统，必须写在继承QObject的类中

public:
    // 【结构体定义】ToolInfo存储单个工具的信息：名称、路径、版本、是否可用
    struct ToolInfo {
        QString name;       // 【字段】工具的显示名称（如"gcc"、"cmake"）
        QString path;       // 【字段】工具在系统中的完整路径（如"/usr/bin/gcc"）
        QString version;    // 【字段】工具的版本号字符串（如"gcc 11.2.0"）
        bool available;     // 【字段】标记该工具是否在系统中被找到且可执行
    };

    // 【构造函数】explicit防止隐式转换，parent指定父对象（用于Qt对象树自动内存管理）
    explicit EnvironmentDetector(QObject *parent = nullptr);

    void detect();  // 【公共方法】执行环境检测，扫描系统中所有已知工具
    // 【公共方法】返回所有已检测工具的信息映射（键为工具名，值为ToolInfo）
    const QMap<QString, ToolInfo>& tools() const;
    // 【公共方法】查询指定名称的工具是否可用，返回true表示可用
    bool isToolAvailable(const QString& name) const;
    // 【公共方法】获取指定工具的完整路径，如果不可用则返回空字符串
    QString toolPath(const QString& name) const;
    // 【公共方法】判断当前是否处于离线模式（无网络连接）
    bool isOfflineMode() const;
    // 【公共方法】获取系统中可用的编译器列表（gcc/g++/clang/cl）
    QStringList availableCompilers() const;
    // 【公共方法】获取系统中可用的构建工具列表（cmake/make/ninja）
    QStringList availableBuildTools() const;

signals:
    // 【信号】检测完成后发射，其他组件可连接此信号获知检测结束
    void detectionCompleted();

private:
    // 【私有方法】检测单个工具，name为逻辑名，possibleNames为系统中可能的文件名列表
    void detectTool(const QString& name, const QStringList& possibleNames);
    // 【私有方法】运行工具获取其版本信息，path为工具的完整路径
    QString getToolVersion(const QString& path);

    QMap<QString, ToolInfo> m_tools;  // 【成员变量】存储所有检测到的工具信息
    bool m_offlineMode;               // 【成员变量】标记当前是否为离线模式（无网络）
};

#endif  // 【条件编译结束】ENVIRONMENTDETECTOR_H宏定义结束
