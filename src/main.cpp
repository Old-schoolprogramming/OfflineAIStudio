/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * 负责初始化Qt应用框架、设置全局样式（Fusion风格+主题管理器样式表），
 * 并创建显示主窗口。所有核心业务逻辑由MainWindow及其子组件承载。
 */
#include "mainwindow.h"
#include "ui/thememanager.h"
#include <QApplication>
#include <QStyleFactory>

/**
 * @brief 主函数 —— 应用程序入口点
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 应用程序退出码
 *
 * @details 初始化流程：
 * 1. 创建QApplication实例（Qt GUI应用必需）
 * 2. 设置Fusion风格（跨平台一致性最佳的原生风格）
 * 3. 加载ThemeManager全局样式表（深色/浅色主题）
 * 4. 创建并显示MainWindow主窗口
 * 5. 进入Qt事件循环
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置Fusion风格，保证跨平台一致的视觉基线
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 应用全局主题样式表（深色/浅色由ThemeManager根据本地配置决定）
    qApp->setStyleSheet(ThemeManager::instance()->styleSheet());

    MainWindow w;
    w.show();

    return a.exec();
}
