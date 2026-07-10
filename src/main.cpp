/**
 * @file main.cpp
 * @brief 应用程序入口
 *
 * 负责初始化Qt应用框架、设置全局样式（Fusion风格+主题管理器样式表），
 * 并创建显示主窗口。所有核心业务逻辑由MainWindow及其子组件承载。
 */
#include "mainwindow.h"       // 【引入主窗口头文件】包含MainWindow类的声明
#include "ui/thememanager.h"  // 【引入主题管理器头文件】管理深色/浅色主题及全局样式表
#include <QApplication>       // 【引入Qt应用类】GUI应用程序的入口类，管理事件循环
#include <QStyleFactory>      // 【引入样式工厂类】用于创建指定名称的Qt风格

/**
 * @brief 主函数 —— 应用程序入口点
 * @param argc 命令行参数个数（程序名也算第1个）
 * @param argv 命令行参数数组（字符串数组，argv[0]通常是程序路径）
 * @return 应用程序退出码（0表示正常退出，非0表示异常）
 *
 * @details 初始化流程：
 * 1. 创建QApplication实例（Qt GUI应用必需，负责事件循环）
 * 2. 设置Fusion风格（跨平台一致性最佳的原生风格）
 * 3. 加载ThemeManager全局样式表（深色/浅色主题）
 * 4. 创建并显示MainWindow主窗口
 * 5. 进入Qt事件循环，等待用户交互
 */
int main(int argc, char *argv[])  // 【程序入口】操作系统从这里开始执行程序
{
    QApplication a(argc, argv);  // 【创建Qt应用对象】将命令行参数传给Qt，初始化GUI框架

    // 设置Fusion风格，保证跨平台一致的视觉基线
    // 【设置应用风格】"Fusion"是Qt推荐的原生风格，在各操作系统上外观一致
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // 应用全局主题样式表（深色/浅色由ThemeManager根据本地配置决定）
    // 【加载全局样式表】ThemeManager::instance()获取单例，styleSheet()返回当前主题的CSS样式字符串
    qApp->setStyleSheet(ThemeManager::instance()->styleSheet());
    // 【qApp说明】qApp是全局宏，等价于QApplication::instance()，指向当前应用实例

    MainWindow w;  // 【创建主窗口对象】实例化MainWindow，构造函数会初始化所有子组件和布局
    w.show();      // 【显示主窗口】将窗口显示在屏幕上，此时窗口才可见

    return a.exec();  // 【进入事件循环】阻塞在此处，持续处理用户输入、绘图、网络等事件，直到程序退出
}
