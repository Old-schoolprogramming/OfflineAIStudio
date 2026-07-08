/**
 * @file skillimportpage.h
 * @brief 技能导入页面头文件
 *
 * 提供技能导入界面的声明，支持通过拖拽或手动选择方式导入技能包。
 * 包含拖拽上传区域、技能路径输入框、已安装技能网格展示，
 * 并支持对已安装技能进行启用/禁用切换和配置操作。
 */

#ifndef SKILLIMPORTPAGE_H
#define SKILLIMPORTPAGE_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QScrollArea>
#include <QFileDialog>

/**
 * @brief 技能导入页面类
 *
 * 继承自QWidget的技能管理页面，提供技能导入、展示和管理功能。
 * 支持拖拽上传技能包、手动选择技能路径、展示已安装技能卡片，
 * 并允许对每个技能进行启用/禁用切换及配置操作。
 */
class SkillImportPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针，默认为nullptr
     */
    explicit SkillImportPage(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SkillImportPage();

signals:
    /**
     * @brief 技能被导入时发出的信号
     * @param path 导入的技能文件路径
     */
    void skillImported(const QString& path);

    /**
     * @brief 技能启用状态切换时发出的信号
     * @param name 技能名称
     * @param enabled 启用状态，true为启用，false为禁用
     */
    void skillToggled(const QString& name, bool enabled);

    /**
     * @brief 技能配置按钮被点击时发出的信号
     * @param name 技能名称
     */
    void skillConfigClicked(const QString& name);

private slots:
    void onUploadClicked();     ///< 上传按钮点击槽函数
    void onImportClicked();     ///< 导入按钮点击槽函数
    void onThemeChanged();      ///< 主题变更响应槽函数

private:
    void setupUI();             ///< 初始化整体界面布局
    void setupUploadZone();     ///< 构建拖拽上传区域
    void setupSkillGrid();      ///< 构建已安装技能网格
    void applyTheme();          ///< 应用当前主题样式

    /**
     * @brief 创建技能卡片
     * @param name 技能名称
     * @param version 技能版本号
     * @param description 技能描述
     * @param type 技能类型
     * @param gradient 卡片渐变色彩标识
     * @param enabled 初始启用状态
     */
    void createSkillCard(const QString& name, const QString& version,
                         const QString& description, const QString& type,
                         const QString& gradient, bool enabled);

    QScrollArea* m_scrollArea;      ///< 滚动区域容器
    QWidget* m_contentWidget;       ///< 滚动内容主窗口
    QWidget* m_uploadZone;          ///< 拖拽上传区域
    QLineEdit* m_pathEdit;          ///< 技能路径输入框
    QPushButton* m_importBtn;       ///< 导入按钮
    QGridLayout* m_skillGrid;       ///< 技能卡片网格布局

    struct SkillInfo {
        QString name;           ///< 技能名称
        QString version;        ///< 技能版本
        QString description;    ///< 技能描述
        QString type;           ///< 技能类型
        QString gradient;       ///< 卡片渐变色彩
        bool enabled;           ///< 启用状态
    };
    QList<SkillInfo> m_skills;  ///< 已安装技能列表
};

#endif
