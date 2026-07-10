/**
 * @file thememanager.h
 * @brief 主题管理器头文件 —— 全局暗色/亮色主题切换与颜色管理
 *
 * 【整体功能】
 * ThemeManager 是一个单例类（全局唯一实例），负责管理应用程序的主题状态，
 * 支持 Dark（暗色）和 Light（亮色）两种主题模式切换。
 * 提供统一的色彩获取接口，并通过 themeChanged 信号通知所有订阅者进行样式更新。
 * 主题配置通过 QSettings 持久化保存到本地，应用重启后自动恢复上次设置。
 *
 * 【使用方式】
 * ThemeManager* tm = ThemeManager::instance();
 * QColor bg = tm->background();        // 获取当前主题的背景色
 * tm->toggleTheme();                   // 在暗色和亮色之间切换
 * connect(tm, &ThemeManager::themeChanged, widget, &Widget::onThemeChanged);
 *
 * 【颜色体系（暗色主题示例）】
 * - background()       → #0f172a（深蓝黑）     → 页面底层背景
 * - bgSecondary()      → #1e293b（深蓝灰）     → 卡片、面板、输入框背景
 * - bgTertiary()       → #334155（中蓝灰）     → 悬停状态、徽章背景
 * - surface()          → #1e293b（深蓝灰）     → 卡片表面、弹窗背景
 * - border()           → #334155（中蓝灰）     → 边框、分隔线
 * - primary()          → #3B82F6（蓝色）       → 主按钮、链接、高亮
 * - primaryForeground()→ #ffffff（白色）       → 主按钮上的文字
 * - primarySubtle()    → rgba(59,130,246,0.15)→ 主色调半透明背景
 * - textPrimary()      → #f1f5f9（浅灰白）     → 主要文字
 * - textSecondary()    → #94a3b8（中灰）       → 次要文字、悬停色
 * - textTertiary()     → #64748b（深灰）       → 辅助文字、徽章文字
 * - stateSuccess()     → #22c55e（绿色）       → 成功状态
 * - stateSuccessBg()   → rgba(34,197,94,0.15) → 成功状态背景
 * - stateWarning()     → #f59e0b（橙色）       → 警告状态
 * - stateError()       → #ef4444（红色）       → 错误状态
 *
 * 【设计模式】单例模式（Singleton）
 * - 构造函数私有，禁止外部直接创建实例
 * - 通过 instance() 静态方法获取唯一实例
 * - 保证全局只有一个 ThemeManager 对象，避免颜色状态不一致
 */

#ifndef THEMEMANAGER_H  // 【条件编译保护】若未定义 THEMEMANAGER_H 宏，则编译以下内容；防止头文件被同一编译单元多次包含导致重定义错误
#define THEMEMANAGER_H  // 【定义保护宏】定义 THEMEMANAGER_H 标识符，标记此文件已被包含过一次

#include <QObject>    // 【引入Qt对象基类】QObject是所有Qt对象的基类，提供信号与槽、属性系统、对象树等核心机制；本类继承QObject，因此必须包含此头文件
#include <QColor>     // 【引入颜色类】QColor表示RGB/ARGB颜色值；本类所有颜色getter方法返回QColor对象
#include <QPalette>   // 【引入调色板类】QPalette管理控件的颜色组（正常、禁用、激活等状态的颜色）；用于设置应用程序全局调色板
#include <QSettings>  // 【引入设置类】QSettings提供跨平台的本地持久化存储（Windows注册表/macOS plist/Linux INI文件）；用于保存和读取用户主题偏好

/**
 * @brief 主题管理器单例类 —— 管理应用程序的全局主题状态和颜色体系
 *
 * 【Q_OBJECT宏】启用Qt元对象系统（信号与槽、运行时类型信息等）；必须放在类声明的私有区域首行
 * 【继承关系】继承QObject，支持信号与槽机制和对象树管理
 *
 * 核心职责：
 * 1. 维护当前主题状态（Dark或Light）
 * 2. 提供统一的颜色获取接口（背景色、文字色、边框色、状态色等）
 * 3. 主题切换时发射 themeChanged 信号通知所有UI组件
 * 4. 通过 QSettings 持久化保存用户主题偏好
 * 5. 生成全局 QSS 样式表字符串
 */
class ThemeManager : public QObject
{
    Q_OBJECT  // 【Qt元对象宏】必须放在类声明的私有区域首行；启用信号与槽、运行时类型信息、属性系统等核心机制；缺少此宏则signals/slots关键字无法使用

public:  // 【公有接口区】外部可访问的方法

    /**
     * @brief 主题枚举类型 —— 定义支持的两种主题模式
     *
     * 【枚举值说明】
     * - Dark  → 暗色主题（值0）；适合夜间使用，背景深色、文字浅色
     * - Light → 亮色主题（值1）；适合日间使用，背景浅色、文字深色
     */
    enum Theme {
        Dark,   // 【暗色主题】背景为深蓝黑色系（如#0f172a），文字为浅色系（如#f1f5f9）；适合低光环境使用，减少眼部疲劳
        Light   // 【亮色主题】背景为白色或浅灰色系（如#ffffff），文字为深色系（如#0f172a）；适合正常光照环境使用
    };

    /**
     * @brief 获取主题管理器单例实例
     * @return ThemeManager单例指针（全局唯一，不会为nullptr）
     *
     * 【单例模式说明】第一次调用时创建实例，后续调用返回同一实例。
     * 【线程安全】当前实现未加锁，应在主线程中调用。
     * 【使用示例】ThemeManager::instance()->background()
     */
    static ThemeManager* instance();

    /**
     * @brief 获取当前主题
     * @return 当前主题枚举值（Dark 或 Light）
     *
     * 【使用场景】判断当前是暗色还是亮色主题，以执行条件逻辑。
     */
    Theme currentTheme() const;

    /**
     * @brief 设置主题
     * @param theme 目标主题枚举值（Dark 或 Light）
     *
     * 【内部流程】
     * 1. 更新 m_theme 成员变量
     * 2. 调用 saveTheme() 将新主题保存到 QSettings
     * 3. 发射 themeChanged(theme) 信号通知所有订阅者
     *
     * 【视觉效果】所有连接了 themeChanged 信号的UI组件会刷新样式，页面颜色整体切换。
     */
    void setTheme(Theme theme);

    /**
     * @brief 切换主题（Dark 与 Light 之间自动切换）
     *
     * 【内部逻辑】若当前为 Dark，则切换为 Light；若当前为 Light，则切换为 Dark。
     * 【使用场景】用户点击主题切换按钮（如太阳/月亮图标）时调用。
     *
     * 【视觉效果】整个应用程序的颜色瞬间切换：
     * - 背景从深蓝黑(#0f172a)变为白色(#ffffff)，或反之
     * - 文字从浅灰白(#f1f5f9)变为深黑(#0f172a)，或反之
     * - 卡片、边框、按钮等所有颜色同步切换
     */
    void toggleTheme();

    // ============================================
    // 【颜色获取接口】以下方法返回当前主题对应的颜色值
    // ============================================

    /**
     * @brief 获取背景色 —— 页面底层背景色
     * @return 背景色 QColor
     *
     * 【暗色主题】#0f172a（深蓝黑）
     * 【亮色主题】#ffffff（白色）
     * 【使用场景】页面底层背景、滚动区域背景
     * 【改色效果】改#ff0000则背景变红色，改#000000则纯黑
     */
    QColor background() const;

    /**
     * @brief 获取前景色 —— 与背景色形成对比的主要前景色
     * @return 前景色 QColor
     *
     * 【暗色主题】#f8fafc（近白色）
     * 【亮色主题】#0f172a（深蓝黑）
     * 【使用场景】与background()搭配使用，确保对比度
     */
    QColor foreground() const;

    /**
     * @brief 获取次要背景色 —— 卡片、面板、输入框等的背景色
     * @return 次要背景色 QColor
     *
     * 【暗色主题】#1e293b（深蓝灰）
     * 【亮色主题】#f1f5f9（浅灰白）
     * 【使用场景】卡片背景、输入框背景、上传区背景
     * 【改色效果】改#ff0000则卡片背景变红色
     */
    QColor bgSecondary() const;

    /**
     * @brief 获取第三级背景色 —— 悬停状态、徽章背景等
     * @return 第三级背景色 QColor
     *
     * 【暗色主题】#334155（中蓝灰）
     * 【亮色主题】#e2e8f0（中灰）
     * 【使用场景】按钮悬停背景、版本徽章背景、类型标签背景
     */
    QColor bgTertiary() const;

    /**
     * @brief 获取表面/卡片背景色 —— 浮层卡片、弹窗的背景色
     * @return 表面色 QColor
     *
     * 【暗色主题】#1e293b（深蓝灰）
     * 【亮色主题】#ffffff（白色）
     * 【使用场景】下拉菜单、弹窗、浮动面板的背景
     */
    QColor surface() const;

    /**
     * @brief 获取边框色 —— 控件边框、分隔线的颜色
     * @return 边框色 QColor
     *
     * 【暗色主题】#334155（中蓝灰）
     * 【亮色主题】#cbd5e1（浅灰）
     * 【使用场景】输入框边框、卡片边框、分隔线、开关关闭状态
     * 【改色效果】改#ff0000则所有边框变红色
     */
    QColor border() const;

    /**
     * @brief 获取主色调 —— 品牌色、强调色
     * @return 主色调 QColor
     *
     * 【暗色主题】#3B82F6（明亮蓝色）
     * 【亮色主题】#2563eb（稍深蓝色）
     * 【使用场景】主按钮背景、链接文字、选中高亮、开关开启状态
     * 【改色效果】改#ff0000则主色调变红色，所有蓝色元素同步变红
     */
    QColor primary() const;

    /**
     * @brief 获取主色调前景色 —— 主色调背景上的文字颜色
     * @return 主色调前景色 QColor
     *
     * 【暗色主题】#ffffff（白色）
     * 【亮色主题】#ffffff（白色）
     * 【使用场景】蓝色主按钮上的白色文字
     */
    QColor primaryForeground() const;

    /**
     * @brief 获取主色调柔和色 —— 主色调的半透明版本
     * @return 主色调柔和色 QColor
     *
     * 【暗色主题】rgba(59, 130, 246, 0.15)（蓝色15%不透明度）
     * 【亮色主题】rgba(37, 99, 235, 0.15)（蓝色15%不透明度）
     * 【使用场景】选中项背景、标签底色、轻量高亮区域
     */
    QColor primarySubtle() const;

    /**
     * @brief 获取主要文本色 —— 正文、标题的主要颜色
     * @return 主要文本色 QColor
     *
     * 【暗色主题】#f1f5f9（浅灰白）
     * 【亮色主题】#0f172a（深蓝黑）
     * 【使用场景】正文文字、标题、输入框文字
     * 【改色效果】改#ff0000则所有主要文字变红色
     */
    QColor textPrimary() const;

    /**
     * @brief 获取次要文本色 —— 辅助说明、占位提示的颜色
     * @return 次要文本色 QColor
     *
     * 【暗色主题】#94a3b8（中灰）
     * 【亮色主题】#64748b（深灰）
     * 【使用场景】占位提示文字、辅助说明、次要信息
     */
    QColor textSecondary() const;

    /**
     * @brief 获取第三级文本色 —— 更淡的辅助文字、禁用状态
     * @return 第三级文本色 QColor
     *
     * 【暗色主题】#64748b（深灰）
     * 【亮色主题】#94a3b8（中灰）
     * 【使用场景】徽章文字、禁用状态文字、时间戳
     */
    QColor textTertiary() const;

    /**
     * @brief 获取成功状态色 —— 表示成功、完成、正常的状态色
     * @return 成功状态色 QColor
     *
     * 【暗色主题】#22c55e（明亮绿色）
     * 【亮色主题】#16a34a（深绿色）
     * 【使用场景】成功提示、已安装状态、勾选标记
     * 【改色效果】改#0000ff则成功状态变蓝色
     */
    QColor stateSuccess() const;

    /**
     * @brief 获取成功状态背景色 —— 成功状态的轻量背景色
     * @return 成功状态背景色 QColor
     *
     * 【暗色主题】rgba(34, 197, 94, 0.15)（绿色15%不透明度）
     * 【亮色主题】rgba(22, 163, 74, 0.15)（绿色15%不透明度）
     * 【使用场景】成功消息气泡背景、成功标签底色
     */
    QColor stateSuccessBg() const;

    /**
     * @brief 获取警告状态色 —— 表示警告、注意的状态色
     * @return 警告状态色 QColor
     *
     * 【暗色主题】#f59e0b（橙色）
     * 【亮色主题】#d97706（深橙色）
     * 【使用场景】警告提示、需要用户注意的信息
     */
    QColor stateWarning() const;

    /**
     * @brief 获取错误状态色 —— 表示错误、失败的状态色
     * @return 错误状态色 QColor
     *
     * 【暗色主题】#ef4444（明亮红色）
     * 【亮色主题】#dc2626（深红色）
     * 【使用场景】错误提示、验证失败、删除操作
     * 【改色效果】改#0000ff则错误状态变蓝色（不建议，违反用户认知习惯）
     */
    QColor stateError() const;

    /**
     * @brief 获取全局样式表字符串 —— 预定义的QSS样式表
     * @return 样式表 QString（一大段QSS规则字符串）
     *
     * 【使用场景】可一次性应用到整个应用程序或特定控件，快速统一外观。
     * 【内容说明】包含QToolTip、QMenu、QScrollBar、QSlider等控件的基础样式规则。
     */
    QString styleSheet() const;

signals:  // 【信号声明区】当主题状态变化时通知外部，实现观察者模式

    /**
     * @brief 主题发生变更时发出的信号
     * @param theme 变更后的主题枚举值（Dark 或 Light）
     *
     * 【触发时机】调用 setTheme() 或 toggleTheme() 后发射。
     * 【连接方式】各UI页面使用 connect(tm, &ThemeManager::themeChanged, page, &Page::onThemeChanged) 建立关联。
     * 【设计意义】实现主题管理器与UI组件的解耦；UI组件无需主动轮询主题变化，由主题管理器统一推送通知。
     */
    void themeChanged(Theme theme);

private:  // 【私有成员区】封装内部实现细节，外部不可直接访问

    /**
     * @brief 构造函数（私有，禁止外部实例化）
     * @param parent 父QObject指针
     *
     * 【单例模式】构造函数声明为private，防止外部通过new直接创建实例。
     * 只能通过 instance() 静态方法获取唯一实例。
     */
    explicit ThemeManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 【Qt自动内存管理】m_settings（QSettings指针）会在析构时自动保存未写入的数据并释放资源。
     */
    ~ThemeManager();

    /**
     * @brief 从 QSettings 加载主题配置
     *
     * 【内部逻辑】读取本地存储的"theme"键值，恢复上次退出时的主题设置。
     * 若从未保存过，默认使用 Dark 主题。
     * 【存储位置】
     * - Windows：HKEY_CURRENT_USER\Software\[Organization]\[Application]
     * - macOS：~/Library/Preferences/[BundleID].plist
     * - Linux：~/.config/[Organization]/[Application].conf
     */
    void loadTheme();

    /**
     * @brief 将当前主题保存到 QSettings
     *
     * 【内部逻辑】将 m_theme 的当前值写入"theme"键，持久化保存到本地。
     * 下次启动应用时通过 loadTheme() 恢复。
     */
    void saveTheme();

    Theme m_theme;          // 【当前主题状态】存储当前使用的主题枚举值（Dark 或 Light）；在构造函数中通过 loadTheme() 初始化
    QSettings* m_settings;  // 【本地设置存储对象】QSettings实例；用于持久化保存和读取主题配置；在构造函数中创建，在析构函数中自动释放
};

#endif  // 【结束条件编译】对应开头的#ifndef，结束头文件保护区域
