/**
 * @file chatwidget.h
 * @brief 聊天窗口组件头文件 —— 用户与AI对话的主要交互界面
 *
 * 【整体功能】
 * ChatWidget 是一个复合组件（将多个基础控件组合成更高层级的UI单元），包含：
 * - 上方的聊天消息显示区域（只读的富文本编辑器 QTextEdit）
 * - 下方的消息输入框（QLineEdit）和发送按钮（QPushButton）
 *
 * 它负责展示对话历史，接收用户输入，并通过 Qt 信号-槽机制通知
 * 上层逻辑有新消息需要发送。设计上与业务逻辑解耦，只关注界面呈现。
 *
 * 【布局结构】
 * ChatWidget (this)
 * └── m_mainLayout (QVBoxLayout, 0边距)
 *     ├── m_chatDisplay (QTextEdit, 只读, 富文本) —— 占据上方大部分空间
 *     └── inputWidget (QWidget容器)
 *         └── inputLayout (QHBoxLayout)
 *             ├── m_messageInput (QLineEdit, 占位提示"输入消息...") —— 占据输入行大部分空间
 *             └── m_sendButton (QPushButton, "发送") —— 位于输入行右侧
 */

#ifndef CHATWIDGET_H  // 【条件编译保护】防止头文件被重复包含；如果未定义 CHATWIDGET_H 宏才编译以下内容
#define CHATWIDGET_H  // 【定义宏】定义 CHATWIDGET_H 宏，确保同一个编译单元中此头文件只被处理一次

#include <QWidget>     // 【引入Qt基础控件类】QWidget是所有UI控件的基类，提供窗口显示、事件处理、布局承载等基础能力；缺少此头文件无法声明ChatWidget类
#include <QTextEdit>   // 【引入富文本编辑器类】QTextEdit支持显示和编辑多行富文本（HTML格式），这里用作聊天消息展示区；可渲染颜色、字体、图片等富媒体内容
#include <QLineEdit>   // 【引入单行输入框类】QLineEdit提供单行文本输入能力，带光标、选中文本、占位提示等功能；这里用作用户消息输入框
#include <QPushButton> // 【引入按钮类】QPushButton提供可点击的按钮控件，支持文本标签、点击信号、样式美化；这里用作"发送"按钮
#include <QVBoxLayout> // 【引入垂直布局类】QVBoxLayout将子控件沿垂直方向（从上到下）排列，可设置间距和边距；用于组织聊天显示区和输入区的上下关系
#include <QScrollArea> // 【引入滚动区域类】QScrollArea为内部控件提供滚动条支持，当内容超出可视区域时可滚动查看；当前代码中虽未直接使用，但QTextEdit内部自带滚动功能

/**
 * @brief 聊天窗口组件 - 用户与AI对话的主要交互界面
 *
 * 【Q_OBJECT宏】必须放在类声明的私有区域首行；启用Qt的元对象系统，使该类支持信号与槽、运行时类型信息、属性系统等核心机制；缺少此宏则signals/slots关键字无法使用
 * 【继承关系】ChatWidget继承自QWidget，因此拥有QWidget的所有属性和方法（如显示、隐藏、设置尺寸、接收鼠标键盘事件等）
 *
 * 设计特点：
 * 1. 消息显示区使用QTextEdit的HTML富文本能力，支持彩色气泡、字体样式、多行内容
 * 2. 用户消息和AI消息使用不同颜色的气泡区分（用户=靛蓝色#6366f1，AI=红色#ef4444）
 * 3. 支持按钮点击和键盘回车两种发送方式，提升用户体验
 * 4. 通过sendMessage信号与外部解耦，不直接处理网络请求
 */
class ChatWidget : public QWidget
{
    Q_OBJECT  // 【Qt元对象宏】必须放在类声明的私有区域首行；启用Qt的元对象系统，使该类支持信号与槽、运行时类型信息、属性系统等核心机制；缺少此宏则signals/slots关键字无法使用

public:  // 【公有访问区】以下成员可被类外部访问，是对外的公共接口

    /**
     * @brief 构造函数
     * @param parent 父窗口指针；当parent不为nullptr时，本控件会自动嵌入父窗口内，并在父窗口销毁时一并销毁
     *
     * 【初始化流程】构造函数内部会：
     * 1. 创建QVBoxLayout垂直布局，设置0边距使控件紧贴窗口边缘
     * 2. 创建QTextEdit作为聊天显示区，设置为只读(setReadOnly(true))，并设置对象名"chatDisplay"供QSS定位
     * 3. 创建输入区域容器QWidget和水平布局QHBoxLayout
     * 4. 创建QLineEdit作为消息输入框，设置占位提示文本"输入消息..."，设置对象名"messageInput"供QSS定位
     * 5. 创建QPushButton作为发送按钮，显示文本"发送"，设置对象名"sendButton"供QSS定位
     * 6. 连接发送按钮的clicked信号到onSendClicked槽函数
     * 7. 连接输入框的returnPressed信号到onReturnPressed槽函数
     *
     * 【UI参数视觉效果】
     * - m_mainLayout->setContentsMargins(0, 0, 0, 0)：布局四周无额外边距；改(8,8,8,8)则聊天区与窗口边缘有8px空白
     * - m_mainLayout->setSpacing(0)：显示区与输入区之间无额外间距；改8则两者之间有8px间隙
     * - m_chatDisplay样式（在chatpage.cpp中设置）：
     *   · background: transparent → 背景透明，继承父容器背景色
     *   · border: none → 无边框，外观更简洁
     *   · padding: 12px → 消息内容与边缘之间12像素留白；改大则留白更多
     * - m_messageInput样式：
     *   · background: #1e293b → 输入框背景为深蓝灰色（暗色主题）；改#ffffff则为白色
     *   · color: #f1f5f9 → 输入文字为浅灰白色；改#000000则为黑色
     *   · border: 1px solid #334155 → 1像素灰色边框；改2px则边框更粗，改#ff0000则边框变红
     *   · border-radius: 8px → 输入框圆角半径8像素；改0则变成直角，改16px则更圆润
     *   · padding: 10px 16px → 上下10px左右16px内边距；改大则输入框内部空间更大
     * - m_sendButton样式：
     *   · background: #3B82F6 → 按钮背景蓝色；改#22c55e则变绿色，改#ef4444则变红色
     *   · color: white → 按钮文字白色；改#000000则变黑色
     *   · border-radius: 8px → 按钮圆角8像素；改0则直角，改16px则更圆
     *   · padding: 10px 24px → 上下10px左右24px内边距；改大则按钮更大
     *   · font-weight: 500 → 文字中等粗细；改700则加粗
     */
    explicit ChatWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 【Qt自动内存管理】由于Qt的父子对象机制（对象树），子控件（如QTextEdit、QPushButton）
     * 会自动随父对象（this即ChatWidget）的销毁而释放，因此此处无需额外代码。
     */
    ~ChatWidget();

    /**
     * @brief 向聊天显示区域追加一条消息气泡
     * @param sender 发送者显示名称（如"用户"或"AI"），会显示在消息顶部作为小标签
     * @param content 消息正文内容（纯文本，方法内部会进行HTML转义防止XSS注入攻击）
     * @param isUser true表示用户消息（使用靛蓝色#6366f1气泡），false表示AI消息（使用红色#ef4444气泡）
     *
     * 【视觉效果】
     * - 每条消息是一个带圆角的卡片状气泡
     * - isUser=true：背景色为 rgba(99, 102, 241, 0.15)（靛蓝色15%不透明度），左边框4px实线#6366f1
     * - isUser=false：背景色为 rgba(239, 68, 68, 0.15)（红色15%不透明度），左边框4px实线#ef4444
     * - 发送者名称：12px字体，对应气泡颜色（#6366f1或#ef4444），加粗
     * - 消息内容：13px字体，主文字色（暗色主题为#f1f5f9），自动换行
     * - 每条消息之间有12px的垂直间距（通过空的<div>实现）
     *
     * 【颜色改后效果】
     * - 改靛蓝色#6366f1为#3B82F6，则用户消息气泡变蓝色
     * - 改红色#ef4444为#22c55e，则AI消息气泡变绿色
     * - 改边框宽度4px为8px，则消息左侧彩色条更宽
     * - 改圆角8px为0px，则消息气泡变成直角矩形
     */
    void addMessage(const QString& sender, const QString& content, bool isUser = false);

    /**
     * @brief 清除聊天显示区中的所有内容
     *
     * 【功能说明】调用QTextEdit::clear()删除所有聊天消息，恢复为空白状态。
     * 适用于切换对话话题或开始新会话的场景。
     * 注意：此操作不可撤销，历史消息会永久丢失（除非外部有保存）。
     */
    void clearChat();

signals:  // 【信号声明区】信号是Qt特有的通知机制，当某个事件发生时发射（emit），由外部槽函数接收处理；信号本身不实现逻辑，只负责通知

    /**
     * @brief 发送消息信号
     * @param message 用户输入的文本内容（已去除首尾空白）
     *
     * 【触发时机】用户点击"发送"按钮或在输入框中按下回车键时发射。
     * 【连接方式】外部主窗口或业务逻辑层应使用connect将此信号关联到实际的发送处理槽函数。
     * 【设计意义】实现UI层与业务逻辑层的解耦，ChatWidget只负责界面展示和输入收集，不负责网络请求或AI调用。
     *
     * 【连接示例】
     * connect(chatWidget, &ChatWidget::sendMessage,
     *         mainWindow, &MainWindow::onSendMessage);
     */
    void sendMessage(const QString& message);

private slots:  // 【私有槽函数区】槽函数是信号的接收者，当关联的信号被发射时自动调用；private表示只能在类内部connect连接，外部不能直接调用

    /**
     * @brief 发送按钮点击槽函数
     *
     * 【信号连接】在构造函数中通过 connect(m_sendButton, &QPushButton::clicked, this, &ChatWidget::onSendClicked) 建立关联
     * 【内部逻辑】读取m_messageInput输入框文本→trim去除首尾空白→若非空则发射sendMessage信号→清空输入框
     * 【用户体验】发送后输入框立即清空，准备接收下一次输入；若输入为空（或只有空白），则不发射信号，避免发送空消息
     */
    void onSendClicked();

    /**
     * @brief 回车键按下槽函数
     *
     * 【信号连接】在构造函数中通过 connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatWidget::onReturnPressed) 建立关联
     * 【内部逻辑】直接调用onSendClicked()，确保按钮点击和键盘回车两种操作行为完全一致
     * 【提升用户体验】用户无需移动鼠标点击按钮，直接按回车即可发送消息，符合主流聊天应用（微信、Slack等）的操作习惯
     */
    void onReturnPressed();

private:  // 【私有成员区】以下成员只能在ChatWidget类内部访问，外部无法直接操作，实现封装

    QTextEdit* m_chatDisplay;   // 【聊天消息显示区指针】指向QTextEdit实例；用于显示所有聊天消息；设置为只读（setReadOnly(true)），用户无法编辑历史消息；支持HTML富文本渲染，因此消息可以包含颜色、字体大小等样式；对象名为"chatDisplay"供QSS样式表定位
    QLineEdit* m_messageInput;  // 【消息输入框指针】指向QLineEdit实例；用户在此输入要发送的消息；支持占位提示文本"输入消息..."（输入框为空时显示灰色提示）；对象名为"messageInput"供QSS样式表定位；与回车键信号连接
    QPushButton* m_sendButton;  // 【发送按钮指针】指向QPushButton实例；显示文本"发送"；点击后触发onSendClicked槽函数；可通过QSS设置按钮颜色、圆角、悬停效果；对象名为"sendButton"供QSS样式表定位；设置光标为Qt::PointingHandCursor（手型）提示可点击
    QVBoxLayout* m_mainLayout;  // 【主布局指针】指向QVBoxLayout实例，作为ChatWidget的顶级布局；管理m_chatDisplay（上方，占据主要垂直空间）和inputWidget（下方输入区域）的垂直排列关系；setContentsMargins(0,0,0,0)表示布局四周无额外边距，使控件紧贴窗口边缘；setSpacing(0)表示显示区与输入区之间无额外间隙
};

#endif  // 【结束条件编译】对应开头的#ifndef，结束整个头文件的保护区域
