/**
 * @file thememanager.h
 * @brief 主题管理器头文件
 *
 * 提供应用程序主题管理的单例类，支持Dark（暗色）和Light（亮色）两种主题模式。
 * 使用QSettings进行本地持久化存储，提供丰富的色彩getter接口及全局样式表生成。
 * 主题切换时通过信号通知各界面组件更新样式。
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QPalette>
#include <QSettings>

/**
 * @brief 主题管理器单例类
 *
 * 负责管理应用程序的全局主题状态，支持暗色/亮色两种模式切换。
 * 提供统一的色彩获取接口，并通过themeChanged信号通知所有订阅者进行样式更新。
 * 主题配置通过QSettings持久化保存到本地。
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 主题枚举类型
     */
    enum Theme {
        Dark,   ///< 暗色主题
        Light   ///< 亮色主题
    };

    /**
     * @brief 获取主题管理器单例实例
     * @return ThemeManager单例指针
     */
    static ThemeManager* instance();

    /**
     * @brief 获取当前主题
     * @return 当前主题枚举值
     */
    Theme currentTheme() const;

    /**
     * @brief 设置主题
     * @param theme 目标主题枚举值
     */
    void setTheme(Theme theme);

    /**
     * @brief 切换主题（Dark与Light之间切换）
     */
    void toggleTheme();

    /**
     * @brief 获取背景色
     * @return 背景色QColor
     */
    QColor background() const;

    /**
     * @brief 获取前景色
     * @return 前景色QColor
     */
    QColor foreground() const;

    /**
     * @brief 获取次要背景色
     * @return 次要背景色QColor
     */
    QColor bgSecondary() const;

    /**
     * @brief 获取第三级背景色
     * @return 第三级背景色QColor
     */
    QColor bgTertiary() const;

    /**
     * @brief 获取表面/卡片背景色
     * @return 表面色QColor
     */
    QColor surface() const;

    /**
     * @brief 获取边框色
     * @return 边框色QColor
     */
    QColor border() const;

    /**
     * @brief 获取主色调
     * @return 主色调QColor
     */
    QColor primary() const;

    /**
     * @brief 获取主色调前景色
     * @return 主色调前景色QColor
     */
    QColor primaryForeground() const;

    /**
     * @brief 获取主色调柔和色
     * @return 主色调柔和色QColor
     */
    QColor primarySubtle() const;

    /**
     * @brief 获取主要文本色
     * @return 主要文本色QColor
     */
    QColor textPrimary() const;

    /**
     * @brief 获取次要文本色
     * @return 次要文本色QColor
     */
    QColor textSecondary() const;

    /**
     * @brief 获取第三级文本色
     * @return 第三级文本色QColor
     */
    QColor textTertiary() const;

    /**
     * @brief 获取成功状态色
     * @return 成功状态色QColor
     */
    QColor stateSuccess() const;

    /**
     * @brief 获取成功状态背景色
     * @return 成功状态背景色QColor
     */
    QColor stateSuccessBg() const;

    /**
     * @brief 获取警告状态色
     * @return 警告状态色QColor
     */
    QColor stateWarning() const;

    /**
     * @brief 获取错误状态色
     * @return 错误状态色QColor
     */
    QColor stateError() const;

    /**
     * @brief 获取全局样式表字符串
     * @return 样式表QString
     */
    QString styleSheet() const;

signals:
    /**
     * @brief 主题发生变更时发出的信号
     * @param theme 变更后的主题枚举值
     */
    void themeChanged(Theme theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);   ///< 构造函数（私有，禁止外部实例化）
    ~ThemeManager();                                    ///< 析构函数

    void loadTheme();   ///< 从QSettings加载主题配置
    void saveTheme();   ///< 将当前主题保存到QSettings

    Theme m_theme;          ///< 当前主题状态
    QSettings* m_settings;  ///< 本地设置存储对象
};

#endif
