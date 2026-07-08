#include "thememanager.h"
#include <QApplication>
#include <QDir>

ThemeManager* ThemeManager::instance()
{
    static ThemeManager* s_instance = new ThemeManager();
    return s_instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent),
      m_theme(Dark)
{
    QString configDir = QDir::homePath() + "/.config/OfflineAIStudio";
    QDir().mkpath(configDir);
    m_settings = new QSettings(configDir + "/theme.ini", QSettings::IniFormat, this);
    loadTheme();
}

ThemeManager::~ThemeManager()
{
}

ThemeManager::Theme ThemeManager::currentTheme() const
{
    return m_theme;
}

void ThemeManager::setTheme(Theme theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        saveTheme();
        emit themeChanged(theme);
    }
}

void ThemeManager::toggleTheme()
{
    setTheme(m_theme == Dark ? Light : Dark);
}

void ThemeManager::loadTheme()
{
    QString themeStr = m_settings->value("theme", "dark").toString();
    m_theme = (themeStr == "light") ? Light : Dark;
}

void ThemeManager::saveTheme()
{
    m_settings->setValue("theme", m_theme == Dark ? "dark" : "light");
}

QColor ThemeManager::background() const
{
    return m_theme == Dark ? QColor("#0F172A") : QColor("#FFFFFF");
}

QColor ThemeManager::foreground() const
{
    return m_theme == Dark ? QColor("#F1F5F9") : QColor("#0F172A");
}

QColor ThemeManager::bgSecondary() const
{
    return m_theme == Dark ? QColor("#1E293B") : QColor("#F8FAFC");
}

QColor ThemeManager::bgTertiary() const
{
    return m_theme == Dark ? QColor("#334155") : QColor("#F1F5F9");
}

QColor ThemeManager::surface() const
{
    return m_theme == Dark ? QColor("#1E293B") : QColor("#FFFFFF");
}

QColor ThemeManager::border() const
{
    return m_theme == Dark ? QColor("#334155") : QColor("#E2E8F0");
}

QColor ThemeManager::primary() const
{
    return m_theme == Dark ? QColor("#3B82F6") : QColor("#2563EB");
}

QColor ThemeManager::primaryForeground() const
{
    return QColor("#FFFFFF");
}

QColor ThemeManager::primarySubtle() const
{
    return m_theme == Dark ? QColor("#172554") : QColor("#EFF6FF");
}

QColor ThemeManager::textPrimary() const
{
    return m_theme == Dark ? QColor("#F1F5F9") : QColor("#0F172A");
}

QColor ThemeManager::textSecondary() const
{
    return m_theme == Dark ? QColor("#94A3B8") : QColor("#475569");
}

QColor ThemeManager::textTertiary() const
{
    return m_theme == Dark ? QColor("#64748B") : QColor("#94A3B8");
}

QColor ThemeManager::stateSuccess() const
{
    return m_theme == Dark ? QColor("#22C55E") : QColor("#16A34A");
}

QColor ThemeManager::stateSuccessBg() const
{
    return m_theme == Dark ? QColor("#052E16") : QColor("#F0FDF4");
}

QColor ThemeManager::stateWarning() const
{
    return m_theme == Dark ? QColor("#F59E0B") : QColor("#D97706");
}

QColor ThemeManager::stateError() const
{
    return m_theme == Dark ? QColor("#EF4444") : QColor("#DC2626");
}

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
