/**
 * @file thememanager.cpp
 * @brief 主题管理器实现
 *
 * @details
 * ThemeManager 采用单例模式（Meyers' Singleton）管理应用主题，
 * 支持深色（Dark）和浅色（Light）两种主题切换。
 *
 * 核心设计要点：
 * 1. 单例模式：通过 instance() 静态方法获取唯一实例，线程安全由 C++11 静态局部变量保证
 * 2. 持久化：主题设置保存到 ~/.config/OfflineAIStudio/theme.ini（QSettings IniFormat）
 * 3. 懒加载：构造时自动调用 loadTheme() 恢复上次设置
 * 4. 信号通知：setTheme() 仅在主题变化时发射 themeChanged 信号，避免重复刷新
 * 5. 统一调色板：所有颜色通过 getter 方法动态计算，确保 UI 各处颜色一致
 */

#include "thememanager.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>

/**
 * @brief 获取 ThemeManager 单例实例
 * @return ThemeManager 指针
 *
 * @implementation
 * 使用 Meyers' Singleton 模式：
 * - 静态局部指针在首次调用时创建，C++11 保证线程安全
 * - 实例生命周期贯穿整个应用运行期，不主动销毁
 */
ThemeManager* ThemeManager::instance()
{
    static ThemeManager* s_instance = new ThemeManager();
    return s_instance;
}

/**
 * @brief 构造函数
 * @param parent 父 QObject
 *
 * @implementation
 * 1. 初始化默认主题为 Dark
 * 2. 计算配置目录：~/.config/OfflineAIStudio
 * 3. 确保目录存在（mkpath）
 * 4. 创建 QSettings 对象，指向 theme.ini
 * 5. 调用 loadTheme() 恢复持久化的主题设置
 */
ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent),
      m_theme(Dark)
{
    QString configDir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(configDir);
    m_settings = new QSettings(configDir + "/theme.ini", QSettings::IniFormat, this);
    loadTheme();
}

/**
 * @brief 析构函数
 */
ThemeManager::~ThemeManager()
{
}

/**
 * @brief 获取当前主题
 * @return 当前主题枚举值（Dark 或 Light）
 */
ThemeManager::Theme ThemeManager::currentTheme() const
{
    return m_theme;
}

/**
 * @brief 设置当前主题
 * @param theme 目标主题枚举值
 *
 * @implementation
 * 1. 检查新主题与当前主题是否不同
 * 2. 更新内部 m_theme 变量
 * 3. 调用 saveTheme() 持久化到新设置
 * 4. 发射 themeChanged(theme) 信号通知所有监听者更新 UI
 */
void ThemeManager::setTheme(Theme theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        saveTheme();
        emit themeChanged(theme);
    }
}

/**
 * @brief 切换主题（深色 ↔ 浅色）
 *
 * @implementation
 * 根据当前主题自动切换到另一种：
 * - 当前为 Dark → 切换为 Light
 * - 当前为 Light → 切换为 Dark
 * 实际切换逻辑委托给 setTheme()，确保持久化和信号发射
 */
void ThemeManager::toggleTheme()
{
    setTheme(m_theme == Dark ? Light : Dark);
}

/**
 * @brief 从配置文件加载主题设置
 *
 * @implementation
 * 1. 从 QSettings 读取 "theme" 键，默认值为 "dark"
 * 2. 将字符串转换为 Theme 枚举："light" → Light，其他 → Dark
 * 3. 结果写入 m_theme，不发射信号（构造时调用）
 */
void ThemeManager::loadTheme()
{
    QString themeStr = m_settings->value("theme", "dark").toString();
    m_theme = (themeStr == "light") ? Light : Dark;
}

/**
 * @brief 将当前主题保存到配置文件
 *
 * @implementation
 * 将 m_theme 转换为字符串：
 * - Dark → "dark"
 * - Light → "light"
 * 写入 QSettings 的 "theme" 键
 */
void ThemeManager::saveTheme()
{
    m_settings->setValue("theme", m_theme == Dark ? "dark" : "light");
}

/**
 * @brief 获取主背景色
 * @return QColor 主背景色
 *
 * - Dark: #0F172A (深蓝灰 slate-900)
 * - Light: #FFFFFF (纯白)
 */
QColor ThemeManager::background() const
{
    return m_theme == Dark ? QColor("#0F172A") : QColor("#FFFFFF");
}

/**
 * @brief 获取主前景色（与背景高对比）
 * @return QColor 主前景色
 *
 * - Dark: #F1F5F9 (浅灰 slate-100)
 * - Light: #0F172A (深蓝灰 slate-900)
 */
QColor ThemeManager::foreground() const
{
    return m_theme == Dark ? QColor("#F1F5F9") : QColor("#0F172A");
}

/**
 * @brief 获取次级背景色
 * @return QColor 次级背景色
 *
 * - Dark: #1E293B (slate-800)
 * - Light: #F8FAFC (slate-50)
 */
QColor ThemeManager::bgSecondary() const
{
    return m_theme == Dark ? QColor("#1E293B") : QColor("#F8FAFC");
}

/**
 * @brief 获取第三级背景色
 * @return QColor 第三级背景色
 *
 * - Dark: #334155 (slate-700)
 * - Light: #F1F5F9 (slate-100)
 */
QColor ThemeManager::bgTertiary() const
{
    return m_theme == Dark ? QColor("#334155") : QColor("#F1F5F9");
}

/**
 * @brief 获取表面/卡片背景色
 * @return QColor 表面背景色
 *
 * - Dark: #1E293B (slate-800)
 * - Light: #FFFFFF (纯白)
 */
QColor ThemeManager::surface() const
{
    return m_theme == Dark ? QColor("#1E293B") : QColor("#FFFFFF");
}

/**
 * @brief 获取边框颜色
 * @return QColor 边框颜色
 *
 * - Dark: #334155 (slate-700)
 * - Light: #E2E8F0 (slate-200)
 */
QColor ThemeManager::border() const
{
    return m_theme == Dark ? QColor("#334155") : QColor("#E2E8F0");
}

/**
 * @brief 获取主题主色（品牌色）
 * @return QColor 主色
 *
 * - Dark: #3B82F6 (蓝色 blue-500)
 * - Light: #2563EB (蓝色 blue-600)
 */
QColor ThemeManager::primary() const
{
    return m_theme == Dark ? QColor("#3B82F6") : QColor("#2563EB");
}

/**
 * @brief 获取主色上的前景文字色
 * @return QColor 始终为 #FFFFFF（白色）
 *
 * @note 用于主色按钮上的文字，确保对比度
 */
QColor ThemeManager::primaryForeground() const
{
    return QColor("#FFFFFF");
}

/**
 * @brief 获取主色弱变体（背景高亮用）
 * @return QColor 主色弱变体
 *
 * - Dark: #172554 (深蓝 blue-950)
 * - Light: #EFF6FF (极浅蓝 blue-50)
 */
QColor ThemeManager::primarySubtle() const
{
    return m_theme == Dark ? QColor("#172554") : QColor("#EFF6FF");
}

/**
 * @brief 获取主要文字颜色
 * @return QColor 主要文字色
 *
 * - Dark: #F1F5F9 (slate-100)
 * - Light: #0F172A (slate-900)
 */
QColor ThemeManager::textPrimary() const
{
    return m_theme == Dark ? QColor("#F1F5F9") : QColor("#0F172A");
}

/**
 * @brief 获取次要文字颜色
 * @return QColor 次要文字色
 *
 * - Dark: #94A3B8 (slate-400)
 * - Light: #475569 (slate-600)
 */
QColor ThemeManager::textSecondary() const
{
    return m_theme == Dark ? QColor("#94A3B8") : QColor("#475569");
}

/**
 * @brief 获取第三级文字颜色（提示、禁用状态）
 * @return QColor 第三级文字色
 *
 * - Dark: #64748B (slate-500)
 * - Light: #94A3B8 (slate-400)
 */
QColor ThemeManager::textTertiary() const
{
    return m_theme == Dark ? QColor("#64748B") : QColor("#94A3B8");
}

/**
 * @brief 获取成功状态色
 * @return QColor 成功色
 *
 * - Dark: #22C55E (绿色 green-500)
 * - Light: #16A34A (绿色 green-600)
 */
QColor ThemeManager::stateSuccess() const
{
    return m_theme == Dark ? QColor("#22C55E") : QColor("#16A34A");
}

/**
 * @brief 获取成功状态背景色
 * @return QColor 成功背景色
 *
 * - Dark: #052E16 (深绿 green-950)
 * - Light: #F0FDF4 (极浅绿 green-50)
 */
QColor ThemeManager::stateSuccessBg() const
{
    return m_theme == Dark ? QColor("#052E16") : QColor("#F0FDF4");
}

/**
 * @brief 获取警告状态色
 * @return QColor 警告色
 *
 * - Dark: #F59E0B (琥珀色 amber-500)
 * - Light: #D97706 (琥珀色 amber-600)
 */
QColor ThemeManager::stateWarning() const
{
    return m_theme == Dark ? QColor("#F59E0B") : QColor("#D97706");
}

/**
 * @brief 获取错误状态色
 * @return QColor 错误色
 *
 * - Dark: #EF4444 (红色 red-500)
 * - Light: #DC2626 (红色 red-600)
 */
QColor ThemeManager::stateError() const
{
    return m_theme == Dark ? QColor("#EF4444") : QColor("#DC2626");
}

/**
 * @brief 生成当前主题对应的 Qt 样式表（QSS）
 * @return QString 完整 QSS 字符串
 *
 * @implementation
 * 1. 调用所有颜色 getter 获取当前主题配色
 * 2. 使用 QString::arg() 批量替换占位符（%1~%10）
 * 3. 覆盖的控件类型：
 *    - 容器：QMainWindow, QWidget, QFrame
 *    - 文本：QLabel
 *    - 按钮：QPushButton（含 primaryButton 特殊 ID）
 *    - 输入：QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox
 *    - 滚动条：QScrollBar（垂直/水平）
 *    - 列表：QListWidget
 *    - 分割器：QSplitter
 *    - 进度条：QProgressBar
 *    - 菜单：QMenu
 *    - 提示：QToolTip
 * 4. 所有颜色值动态计算，确保与当前主题一致
 */
QString ThemeManager::styleSheet() const
{
    QString bg = background().name();
    QString fg = foreground().name();
    QString bg2 = bgSecondary().name();
    QString bg3 = bgTertiary().name();
    QString surf = surface().name();
    QString bdr = border().name();
    QString pri = primary().name();
    QString priFg = primaryForeground().name();
    QString priSub = primarySubtle().name();
    QString txtPri = textPrimary().name();
    QString txtSec = textSecondary().name();
    QString txtTer = textTertiary().name();
    QString suc = stateSuccess().name();
    QString err = stateError().name();

    return QString(R"(
        QMainWindow {
            background-color: %1;
        }
        QWidget {
            background-color: %1;
            color: %7;
        }
        QFrame {
            background-color: %1;
            border: none;
        }
        QLabel {
            color: %7;
            background: transparent;
        }
        QPushButton {
            border: none;
            border-radius: 8px;
            padding: 8px 18px;
            font-weight: 500;
            font-size: 13px;
            background-color: %5;
            color: %7;
        }
        QPushButton:hover {
            background-color: %3;
        }
        QPushButton:pressed {
            background-color: %4;
        }
        QPushButton#primaryButton {
            background-color: %6;
            color: %8;
        }
        QPushButton#primaryButton:hover {
            background-color: %9;
        }
        QLineEdit {
            background-color: %2;
            color: %7;
            border: 1px solid %4;
            border-radius: 8px;
            padding: 8px 14px;
            font-size: 13px;
            selection-background-color: %6;
        }
        QLineEdit:focus {
            border-color: %6;
        }
        QTextEdit {
            background-color: %2;
            color: %7;
            border: 1px solid %4;
            border-radius: 8px;
            padding: 12px;
            font-size: 13px;
            line-height: 1.6;
        }
        QScrollBar:vertical {
            background-color: %2;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background-color: %3;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %4;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: %2;
            height: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal {
            background-color: %3;
            border-radius: 4px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: %4;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
        QComboBox {
            background-color: %2;
            color: %7;
            border: 1px solid %4;
            border-radius: 8px;
            padding: 6px 12px;
            min-height: 22px;
        }
        QComboBox:focus {
            border-color: %6;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: %2;
            color: %7;
            border: 1px solid %4;
            border-radius: 8px;
            selection-background-color: %6;
        }
        QSpinBox, QDoubleSpinBox {
            background-color: %2;
            color: %7;
            border: 1px solid %4;
            border-radius: 8px;
            padding: 6px 10px;
        }
        QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: %6;
        }
        QListWidget {
            background-color: %1;
            color: %7;
            border: none;
            outline: none;
        }
        QListWidget::item {
            border-radius: 8px;
            padding: 10px 12px;
            margin: 2px 4px;
        }
        QListWidget::item:selected {
            background-color: %10;
            color: %7;
        }
        QListWidget::item:hover {
            background-color: %3;
        }
        QSplitter::handle {
            background-color: %4;
        }
        QSplitter::handle:hover {
            background-color: %6;
        }
        QProgressBar {
            border: none;
            border-radius: 6px;
            background-color: %3;
            text-align: center;
            color: %7;
            font-size: 12px;
            height: 20px;
        }
        QProgressBar::chunk {
            border-radius: 6px;
            background-color: %6;
        }
        QMenu {
            background-color: %2;
            color: %7;
            border: 1px solid %4;
            border-radius: 8px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 20px;
            border-radius: 6px;
        }
        QMenu::item:selected {
            background-color: %3;
        }
        QToolTip {
            background-color: %7;
            color: %1;
            border: none;
            border-radius: 6px;
            padding: 4px 10px;
            font-size: 12px;
        }
    )").arg(bg, bg2, bg3, bdr, surf, pri, txtPri, priFg, txtSec, priSub);
}
