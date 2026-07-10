/**
 * @file skillimportpage.h
 * @brief 技能导入页面头文件 —— 技能包上传、展示与管理界面
 *
 * 【整体功能】
 * SkillImportPage 是一个完整的技能管理页面，采用滚动布局设计，包含：
 * - 顶部拖拽上传区域：支持拖拽技能包文件或点击选择文件上传
 * - 路径输入行：手动输入技能配置文件路径并导入
 * - 已安装技能网格：以卡片形式展示所有已安装技能，每张卡片包含：
 *   · 渐变色图标 + 技能名称 + 版本徽章
 *   · 技能描述文本（自动换行）
 *   · 类型标签 + 配置按钮 + 启用/禁用开关
 * - 底部信息栏：显示技能统计数量和外部链接
 *
 * 【页面布局从上到下】
 * 拖拽上传区 → 路径输入行 → 分隔线+"已安装的技能"标题 → 技能卡片网格 → 底部信息栏
 */

#ifndef SKILLIMPORTPAGE_H  // 【条件编译保护】若未定义 SKILLIMPORTPAGE_H 宏，则编译以下内容；防止头文件被同一编译单元多次包含导致重定义错误
#define SKILLIMPORTPAGE_H  // 【定义保护宏】定义 SKILLIMPORTPAGE_H 标识符，标记此文件已被包含过一次

#include <QWidget>      // 【引入Qt基础控件类】QWidget是所有UI控件的基类，提供显示、布局、事件处理等基础能力；本类继承QWidget，因此必须包含此头文件
#include <QLineEdit>    // 【引入单行输入框类】QLineEdit提供单行文本输入、占位提示等功能；用于手动输入技能配置文件路径
#include <QPushButton>  // 【引入按钮类】QPushButton提供可点击按钮；用于"导入"按钮、"选择文件"按钮、"配置"按钮和启用/禁用开关
#include <QVBoxLayout>  // 【引入垂直布局类】QVBoxLayout将子控件沿垂直方向排列；用于整体页面的上下结构
#include <QLabel>       // 【引入标签类】QLabel显示静态文本；用于标题、描述、版本徽章、类型标签等
#include <QGridLayout>  // 【引入网格布局类】QGridLayout将控件按行和列排列；用于技能卡片网格（每行2列）
#include <QScrollArea>  // 【引入滚动区域类】QScrollArea为内部控件提供滚动条支持；当页面内容超出窗口高度时可上下滚动查看
#include <QFileDialog>  // 【引入文件对话框类】QFileDialog打开文件选择窗口；用于点击"选择文件"按钮时弹出文件选择器

/**
 * @brief 技能导入页面类 - 提供技能包的上传导入、路径输入导入及已安装技能的管理功能
 *
 * 【Q_OBJECT宏】启用Qt元对象系统（信号与槽、运行时类型信息等）；必须放在类声明的私有区域首行
 * 【继承关系】继承QWidget，成为一个完整的可视化页面组件；可嵌入主窗口的堆叠控件或标签页中
 *
 * 设计特点：
 * 1. 采用QScrollArea滚动布局，确保内容超出屏幕时可上下滚动
 * 2. 技能卡片使用QGridLayout两列网格排列，自适应宽度
 * 3. 每张卡片包含渐变色图标、版本徽章、描述文本、类型标签和启用开关
 * 4. 支持主题切换，通过ThemeManager动态获取颜色值
 */
class SkillImportPage : public QWidget
{
    Q_OBJECT  // 【Qt元对象宏】必须放在类声明的私有区域首行；启用信号与槽、运行时类型信息、属性系统等核心机制；缺少此宏则signals/slots关键字无法使用

public:  // 【公有接口区】外部可访问的构造函数和析构函数

    /**
     * @brief 构造函数
     * @param parent 父窗口指针，默认为nullptr
     *
     * 【初始化流程】
     * 1. 调用setupUI()创建和排列所有界面控件
     * 2. 调用applyTheme()应用当前主题的颜色样式
     * 3. 连接ThemeManager的themeChanged信号到onThemeChanged槽函数
     *
     * 【内存管理】若parent不为nullptr，本页面会自动嵌入父窗口并在父窗口销毁时自动释放
     */
    explicit SkillImportPage(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 【Qt自动内存管理】本类所有子控件（m_scrollArea、m_contentWidget、m_uploadZone等）在创建时都指定了this或父控件作为父对象，
     * 因此当SkillImportPage对象被销毁时，Qt会自动删除所有子控件，无需手动delete。
     */
    ~SkillImportPage();

signals:  // 【信号声明区】当用户操作发生时通知外部，实现UI与业务逻辑的解耦

    /**
     * @brief 技能被导入时发出的信号
     * @param path 导入的技能文件绝对路径（如 "D:/skills/my-skill.zip"）
     *
     * 【触发时机】用户通过拖拽上传或手动输入路径后点击"导入"按钮时发射。
     * 【接收方】通常由主窗口接收，然后调用技能管理器解析并安装技能包。
     * 【连接示例】
     * connect(skillImportPage, &SkillImportPage::skillImported,
     *         mainWindow, &MainWindow::onSkillImported);
     */
    void skillImported(const QString& path);

    /**
     * @brief 技能启用状态切换时发出的信号
     * @param name 技能名称字符串（如 "Python 代码助手"）
     * @param enabled 启用状态，true表示启用，false表示禁用
     *
     * 【触发时机】用户点击技能卡片上的启用/禁用开关时发射。
     * 【接收方】主窗口接收后通知技能管理器启用或禁用对应技能。
     * 【视觉效果】开关为蓝色时表示启用（#3B82F6），灰色时表示禁用（#475569）。
     */
    void skillToggled(const QString& name, bool enabled);

    /**
     * @brief 技能配置按钮被点击时发出的信号
     * @param name 技能名称字符串
     *
     * 【触发时机】用户点击技能卡片上的"配置"按钮时发射。
     * 【接收方】主窗口接收后可弹出配置对话框，显示该技能的详细参数设置。
     */
    void skillConfigClicked(const QString& name);

private slots:  // 【私有槽函数区】只能由类内部或友元访问的槽函数；处理按钮点击和主题变化

    /**
     * @brief 上传按钮点击槽函数
     *
     * 【信号连接】在setupUploadZone()中通过 connect(selectBtn, &QPushButton::clicked, this, &SkillImportPage::onUploadClicked) 建立关联
     * 【内部逻辑】弹出QFileDialog文件选择对话框，过滤支持格式（.zip/.json/.yaml/.yml）。
     * 若用户选择了文件，将路径写入m_pathEdit输入框并发射skillImported信号。
     */
    void onUploadClicked();

    /**
     * @brief 导入按钮点击槽函数
     *
     * 【信号连接】在setupUI()中通过 connect(m_importBtn, &QPushButton::clicked, this, &SkillImportPage::onImportClicked) 建立关联
     * 【内部逻辑】读取m_pathEdit输入框中的文本并去除首尾空白，若非空则发射skillImported信号。
     */
    void onImportClicked();

    /**
     * @brief 主题变更响应槽函数
     *
     * 【触发方式】当用户切换暗色/亮色主题时，ThemeManager发射themeChanged信号，
     * 本页面通过connect()连接该信号到此槽函数，从而重新应用颜色样式。
     * 【执行动作】重新调用applyTheme()，从ThemeManager获取新的颜色值并应用到整个页面。
     */
    void onThemeChanged();

private:  // 【私有成员区】封装内部UI控件和辅助方法，外部不可直接访问

    /**
     * @brief 初始化整体界面布局
     *
     * 【内部流程】
     * 1. 创建主滚动区域QScrollArea，设置无边框、隐藏水平滚动条
     * 2. 创建内容面板QWidget，四边留32像素内边距
     * 3. 依次调用setupUploadZone()创建拖拽上传区
     * 4. 创建路径输入行（标签+输入框+导入按钮）
     * 5. 创建分隔线和"已安装的技能"标题
     * 6. 调用setupSkillGrid()创建技能卡片网格
     * 7. 创建底部信息栏（统计标签+外部链接按钮）
     * 8. 将内容面板放入滚动区域
     */
    void setupUI();

    /**
     * @brief 构建拖拽上传区域
     *
     * 【视觉效果】固定高度180像素的垂直居中面板，包含：
     * - 云上传图标（48×48像素，灰色#94A3B8）
     * - 主标题"拖拽技能包到此处，或点击上传"（15px加粗）
     * - 格式提示"支持 .zip, .json, .yaml 格式的技能配置文件"（13px灰色）
     * - "选择文件"按钮（带边框的轮廓按钮样式）
     *
     * 【QSS样式】在applyTheme()中通过#uploadZone设置：
     * - 背景色为次级背景色
     * - 2像素虚线边框（border: 2px dashed）
     * - 16px大圆角（border-radius: 16px）
     * - 悬停时边框变为主色调蓝色，背景变为第三级背景色
     */
    void setupUploadZone();

    /**
     * @brief 构建已安装技能网格
     *
     * 【功能说明】创建QGridLayout网格布局（每行2列，间距16像素），
     * 并预置4个示例技能数据，为每个技能调用createSkillCard()创建卡片。
     *
     * 【预置示例数据】
     * - Python 代码助手（v2.1.0，代码生成类型，蓝橙渐变，已启用）
     * - 数据分析技能（v1.3.2，数据分析类型，绿青渐变，已启用）
     * - 技术文档撰写（v1.0.0，文档撰写类型，橙红渐变，已禁用）
     * - 自动化工作流（v3.2.1，自动化类型，紫蓝渐变，已启用）
     */
    void setupSkillGrid();

    /**
     * @brief 应用当前主题样式
     *
     * 【QSS样式表说明】本函数从ThemeManager获取当前主题的颜色值，
     * 拼接成一大段CSS风格的样式字符串，然后通过m_contentWidget->setStyleSheet()应用到页面。
     * 样式表中定义了内容区背景、上传区边框、输入框、各类按钮、技能卡片、徽章等视觉效果。
     *
     * 【颜色变量】bg=主背景, bg2=次级背景, bg3=第三级背景, bdr=边框色,
     * pri=主色调蓝色, txtPri=主文字色, txtSec=次要文字色, txtTer=第三级文字色
     */
    void applyTheme();

    /**
     * @brief 创建单个技能卡片并添加到网格
     * @param name 技能名称（如 "Python 代码助手"）
     * @param version 技能版本号（如 "v2.1.0"）
     * @param description 技能描述文本（自动换行显示）
     * @param type 技能类型标签（如 "代码生成"、"数据分析"）
     * @param gradient 渐变色定义（格式如"#3B82F6, #F59E0B"，两个颜色逗号分隔）
     * @param enabled 初始启用状态（true=启用，false=禁用）
     *
     * 【卡片内部结构】
     * 技能卡片是一个QWidget容器，内部使用QVBoxLayout垂直排列：
     * - 头部（QHBoxLayout）：渐变色方形图标(32×32) + 技能名称(14px加粗) + 版本徽章(11px)
     * - 中部：描述文本(13px，自动换行)
     * - 底部（QHBoxLayout）：类型标签(11px) + 弹性空间 + "配置"按钮 + 启用/禁用开关(44×24)
     *
     * 【视觉效果参数】
     * - 卡片内边距：20px（四边）；改大则内容与边缘距离增大
     * - 卡片间距：12px（内部元素之间）
     * - 渐变色图标：32×32像素，8px圆角；使用qlineargradient实现从左上到右下的双色渐变
     * - 版本徽章：背景第三级背景色，文字第三级文字色，1px上下内边距，8px左右内边距，999px圆角形成胶囊形状
     * - 启用开关：44×24像素，12px圆角形成药丸形状；蓝色(#3B82F6)=启用，灰色(#475569)=禁用
     */
    void createSkillCard(const QString& name, const QString& version,
                         const QString& description, const QString& type,
                         const QString& gradient, bool enabled);

    QScrollArea* m_scrollArea;      // 【滚动区域容器】QScrollArea实例；当页面内容超出窗口高度时自动显示垂直滚动条；隐藏水平滚动条
    QWidget* m_contentWidget;       // 【滚动内容主窗口】QScrollArea内部的真实内容容器；所有子控件都放在这里；对象名为"skillContent"供QSS定位
    QWidget* m_uploadZone;          // 【拖拽上传区域】QWidget实例；固定高度180px；包含上传图标、标题、提示文字和选择文件按钮；对象名为"uploadZone"供QSS定位
    QLineEdit* m_pathEdit;          // 【技能路径输入框】QLineEdit实例；用户可手动输入技能配置文件路径；对象名为"pathInput"供QSS定位；占位提示"例如: /path/to/skill-config.yaml"
    QPushButton* m_importBtn;       // 【导入按钮】QPushButton实例；点击后读取输入框路径并发射skillImported信号；对象名为"primaryButton"供QSS设置为蓝色主按钮样式
    QGridLayout* m_skillGrid;       // 【技能卡片网格布局】QGridLayout实例；两列网格排列技能卡片；setSpacing(16)表示卡片之间16像素间距，改大则卡片之间空隙更大

    /**
     * @brief 技能信息结构体
     *
     * 【用途】在内存中存储每个技能的基本信息，用于创建技能卡片时传递参数。
     * 注意：此结构体仅用于setupSkillGrid()中预置示例数据，实际技能数据通常由外部技能管理器提供。
     */
    struct SkillInfo {
        QString name;        // 【技能名称】如 "Python 代码助手"；显示在卡片头部
        QString version;     // 【技能版本】如 "v2.1.0"；显示在版本徽章中
        QString description; // 【技能描述】如 "智能 Python 代码生成、优化与调试辅助"；显示在卡片中部，自动换行
        QString type;        // 【技能类型】如 "代码生成"、"数据分析"；显示在底部类型标签中
        QString gradient;    // 【卡片渐变色彩】格式为"#3B82F6, #F59E0B"（两个HEX颜色逗号分隔）；用于绘制32×32渐变色图标
        bool enabled;        // 【启用状态】true表示已启用（开关显示蓝色），false表示已禁用（开关显示灰色）
    };
    QList<SkillInfo> m_skills;  // 【已安装技能列表】存储当前要展示在网格中的所有技能信息；在setupSkillGrid()中初始化为4个示例技能
};

#endif  // 【结束条件编译】对应开头的#ifndef，结束头文件保护区域
