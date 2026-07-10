/**
 * @file outputpanel.cpp
 * @brief 执行输出面板实现 —— 对话式纯文本输出
 *
 * 本实现采用简洁的纯文本列表形式展示Multi-Agent执行日志，
 * 不再使用彩色HTML卡片（与早期设计不同），确保输出清晰易读。
 * 所有内容以等宽字体渲染，保持代码和命令输出的格式对齐。
 *
 * UI参数视觉效果详细说明：
 * - m_mainLayout->setContentsMargins(12, 12, 12, 12)：面板四周留12像素边距；若改为8px则紧贴边缘显得拥挤；改为20px则空白过多
 * - m_mainLayout->setSpacing(8)：头部区域与输出区之间8px间距；若改为16px则间距过大，视觉上断开连接
 * - headerLayout->setContentsMargins(0, 0, 0, 0)：头部布局无边距，使标题和工具栏紧密排列在headerWidget内
 * - headerLayout->setSpacing(8)：标题与工具栏之间8px间距；若改为0px则两者紧贴，若改为16px则距离过远
 * - m_titleLabel样式：color: #f1f5f9 → 极浅灰白色，在深色背景上清晰可读；若改为#ffffff则纯白，长时间阅读刺眼
 *   font-size: 14px → 14像素中等字号；若改为18px则标题过大，若改为12px则偏小不易辨认
 *   font-weight: 600 → 半加粗（介于normal和bold之间），标题有分量但不突兀；若改为400则与普通文字无异
 * - m_toolBar->setIconSize(QSize(16, 16))：工具栏图标/按钮尺寸16x16像素；若改为24x24则按钮过大，占用过多头部空间
 * - QToolBar QSS样式：
 *   background: transparent → 工具栏背景完全透明，与headerWidget背景融为一体；若设为#1e293b则会出现色块分割
 *   spacing: 4px → 复制和清空按钮之间4px间距；若改为8px则按钮距离增大，若改为0px则两按钮紧贴
 * - QToolButton QSS样式：
 *   background-color: transparent → 默认状态按钮背景透明，低调不抢注意力
 *   color: #94a3b8 → 默认文字为中性灰色（slate-400）；若改为#f1f5f9则默认状态过于醒目
 *   border: 1px solid #334155 → 1像素深灰色边框，勾勒出按钮轮廓；若改为2px则边框过粗，若去掉border则按钮像纯文本难以识别可点击
 *   border-radius: 6px → 6像素圆角，使矩形按钮边缘柔和；若改为0px则直角生硬，若改为12px则过于圆润像胶囊
 *   padding: 4px 10px → 上下4px、左右10px的内边距；若改为8px 16px则按钮变大，若改为2px 6px则按钮过于紧凑
 *   font-size: 12px → 12像素小字，与标题形成层级对比；若改为14px则与标题同大，失去层级感
 * - QToolButton:hover样式：
 *   background-color: #1e293b → 鼠标悬停时按钮背景变为深蓝灰色，提供视觉反馈；若改为#6366f1则悬停过于鲜艳刺眼
 *   color: #f1f5f9 → 悬停时文字变为浅白色，与深色背景形成对比；若保持#94a3b8则对比度不足
 * - QTextEdit QSS样式：
 *   background-color: #0f172a → 极深蓝黑色背景（slate-900），典型的代码编辑器深色主题；若改为#000000则纯黑缺乏层次
 *   color: #e2e8f0 → 浅灰白色文字（slate-200），在深色背景上阅读舒适；若改为#ffffff则纯白对比度过高易疲劳
 *   border: 1px solid #334155 → 1像素深灰边框勾勒输出区轮廓；若改为2px则边框过粗，若去掉则区域边界不清
 *   border-radius: 8px → 8像素圆角，使输出区四边柔和；若改为0px则直角生硬，若改为16px则过于圆润
 *   padding: 12px → 内容与边缘12px内边距，确保文字不贴边；若改为6px则拥挤，若改为20px则浪费空间
 *   font-family: 'Consolas', 'Monaco', 'Courier New', monospace → 等宽字体回退链；Consolas是Windows主流等宽字体，Monaco是macOS等宽字体，Courier New是通用备用
 *   font-size: 13px → 13像素等宽字号；若改为16px则显示内容减少，若改为11px则过小难读
 *   line-height: 1.6 → 行高为字号的1.6倍，即约20.8像素；增加行间距使多行文本不拥挤；若改为1.0则行与行紧贴，若改为2.0则间距过大
 */

#include "outputpanel.h"  // 【引入头文件】包含OutputPanel类的声明，使本文件知道类有哪些成员和方法
#include <QDateTime>      // 【引入日期时间类】用于获取当前系统时间，作为步骤头的时间戳
#include <QApplication>   // 【引入应用类】QApplication::clipboard()提供对系统剪贴板的访问，用于复制操作
#include <QClipboard>     // 【引入剪贴板类】QClipboard用于读写系统剪贴板内容

/**
 * @brief 构造函数
 * @param parent 父QWidget指针；决定面板的嵌入位置和生命周期
 *
 * 内部调用setupUI()创建所有子控件和布局，并应用QSS样式表。
 * 构造函数执行后，面板即可显示，但输出区为空（等待外部调用append方法）。
 */
OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)  // 【初始化列表】调用父类QWidget构造函数，传入parent建立Qt对象树关系
{
    setupUI();  // 【调用UI初始化】创建标题、工具栏、输出显示区等所有控件并建立布局
}

/**
 * @brief 析构函数
 *
 * Qt父子对象机制自动释放所有子控件（m_outputDisplay、m_titleLabel、m_toolBar等），
 * 无需手动delete。保持为空即可。
 */
OutputPanel::~OutputPanel()
{
    // Qt对象树自动管理内存，此处无需编写代码
}

/**
 * @brief 创建和布局所有 UI 子控件
 *
 * @implementation
 * 控件层次结构：
 * m_mainLayout (QVBoxLayout) - 整个面板的顶级布局，边距12px，间距8px
 *   ├── headerWidget (QWidget) - 头部区域容器
 *   │   └── headerLayout (QHBoxLayout) - 头部水平布局，边距0，间距8px
 *   │       ├── m_titleLabel - "执行输出"标题（14px白色半加粗文字）
 *   │       ├── addStretch() - 水平弹簧，将标题推向左侧、工具栏推向右侧
 *   │       └── m_toolBar - 工具栏（复制、清空两个QAction按钮）
 *   └── m_outputDisplay (QTextEdit) - 输出显示区（只读、等宽字体、深色背景）
 *
 * 样式设计：
 * - 整体深色主题（#0f172a背景 + #e2e8f0文字），与主窗口暗色风格协调
 * - 标题使用14px半加粗白色文字，在头部左侧突出显示
 * - 工具栏按钮使用透明背景+深灰边框，悬停时变深蓝灰背景，提供交互反馈
 * - 输出区使用等宽字体+1.6倍行高，便于阅读代码和格式化文本
 */
void OutputPanel::setupUI()
{
    // 【创建顶级垂直布局】QVBoxLayout将子控件沿垂直方向从上到下排列
    // 参数this表示此布局绑定到当前OutputPanel面板
    m_mainLayout = new QVBoxLayout(this);
    
    // 【设置布局四周边距】setContentsMargins(左, 上, 右, 下) = (12, 12, 12, 12)
    // 视觉效果：面板内的控件与面板边框之间保持12像素的空白内边距，形成舒适的呼吸空间
    // 若改为setContentsMargins(8, 8, 8, 8)，则边距缩小，控件更贴近边缘，显得拥挤
    // 若改为setContentsMargins(20, 20, 20, 20)，则空白过大，内容区域缩小，浪费屏幕空间
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    
    // 【设置布局控件间距】setSpacing(8)表示相邻控件（headerWidget与m_outputDisplay）之间的垂直间距为8像素
    // 视觉效果：头部区域和输出区之间有8px的间隙，轻微分隔两个功能区域
    // 若改为setSpacing(16)，则头部与输出区之间距离过大，视觉上断开连接
    // 若改为setSpacing(0)，则头部紧贴输出区，没有分界感
    m_mainLayout->setSpacing(8);

    // 【创建头部区域容器】QWidget作为容器，承载标题标签和工具栏
    // 使用独立容器便于对头部区域整体设置样式（如背景色、边框）
    QWidget* headerWidget = new QWidget(this);
    
    // 【创建头部水平布局】QHBoxLayout将子控件沿水平方向从左到右排列
    // 参数headerWidget表示此布局绑定到headerWidget容器
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    
    // 【设置头部布局边距为0】headerWidget内部的标题和工具栏紧贴容器边缘
    // 视觉效果：headerLayout内的控件与headerWidget边界之间无额外空白
    // 若改为(8, 8, 8, 8)，则标题和工具栏会向内缩进，头部区域看起来变小
    headerLayout->setContentsMargins(0, 0, 0, 0);
    
    // 【设置头部布局控件间距】标题与工具栏之间保持8像素水平间距
    // 视觉效果："执行输出"标题和右侧的复制/清空按钮之间有8px的间隙
    // 若改为16px，则标题和按钮距离过远；若改为0px，则标题紧贴按钮
    headerLayout->setSpacing(8);

    // 【创建标题标签】QLabel显示静态文本"执行输出"
    // 参数headerWidget表示父窗口是头部容器
    m_titleLabel = new QLabel("执行输出", headerWidget);
    
    // 【设置标题标签样式】使用setStyleSheet直接应用QSS样式（不依赖外部样式表）
    // color: #f1f5f9 → 极浅灰白色文字（slate-100），在深色背景上高对比度清晰可读
    //   若改为#ffffff纯白，对比度过高，长时间阅读易造成视觉疲劳
    //   若改为#94a3b8灰色，则对比度不足，标题不够醒目
    // font-size: 14px → 14像素中等字号，比正文13px略大，形成层级对比
    //   若改为18px则标题过大，挤压工具栏空间；若改为12px则与按钮文字同大，失去层级
    // font-weight: 600 → 半加粗（semi-bold），比normal(400)粗，比bold(700)细，标题有分量但不突兀
    //   若改为400则与普通文字粗细相同，标题感消失；若改为700则过于粗重
    m_titleLabel->setStyleSheet(
        "color: #f1f5f9;"
        "font-size: 14px;"
        "font-weight: 600;"
    );
    
    // 【将标题加入头部布局】标题会位于左侧（水平布局的第一个控件）
    headerLayout->addWidget(m_titleLabel);
    
    // 【添加水平弹簧】addStretch()在标题和工具栏之间插入一个可伸缩的空白区域
    // 视觉效果：弹簧会将标题推向最左端，将后续添加的工具栏推向最右端，实现左右对齐
    // 若没有addStretch()，标题和工具栏会紧邻在一起，都挤在左侧
    headerLayout->addStretch();

    // 【创建工具栏】QToolBar提供操作按钮的容器
    // 参数headerWidget表示父窗口是头部容器
    m_toolBar = new QToolBar(headerWidget);
    
    // 【设置工具栏图标/按钮尺寸】setIconSize(QSize(16, 16))指定工具栏上按钮的默认尺寸为16x16像素
    // 由于本实现使用文本按钮（"复制"、"清空"），此设置影响按钮的最小尺寸基准
    // 若改为QSize(24, 24)，则按钮整体变大，头部区域高度被迫增加
    // 若改为QSize(12, 12)，则按钮过小，文字可能显示不全或难以点击
    m_toolBar->setIconSize(QSize(16, 16));
    
    // 【设置工具栏QSS样式】使用setStyleSheet为工具栏及其按钮定义外观
    // QToolBar部分：
    //   background: transparent → 工具栏背景完全透明，不显示任何背景色，与headerWidget背景融为一体
    //     若改为background: #1e293b，则工具栏会显示一个深蓝灰色矩形块，与头部背景产生色块分割
    //   spacing: 4px → 复制按钮和清空按钮之间的水平间距为4像素
    //     若改为8px则两按钮距离增大；若改为0px则两按钮紧贴在一起
    // QToolButton部分（默认状态）：
    //   background-color: transparent → 按钮默认无背景色，低调显示
    //   color: #94a3b8 → 默认文字颜色为中性灰色（slate-400），不抢注意力
    //     若改为#f1f5f9浅白色，则默认状态过于醒目，悬停时的反馈效果被弱化
    //   border: 1px solid #334155 → 1像素深灰边框（slate-700），勾勒出按钮可点击的轮廓
    //     若改为2px则边框过粗显得笨重；若去掉border则按钮像纯文本，用户难以识别可点击
    //     若改为border: 1px solid #6366f1，则默认状态就有靛蓝色边框，过于鲜艳
    //   border-radius: 6px → 6像素圆角，使矩形按钮四边柔和
    //     若改为0px则直角生硬；若改为10px则过于圆润像药丸形状
    //   padding: 4px 10px → 上下内边距4px，左右内边距10px；按钮内容（"复制"/"清空"）与边框之间有呼吸空间
    //     若改为8px 16px则按钮变大；若改为2px 6px则文字紧贴边框，显得拥挤
    //   font-size: 12px → 12像素小字，与标题14px形成字号层级对比
    //     若改为14px则与标题同大，失去层级；若改为10px则过小难读
    // QToolButton:hover部分（鼠标悬停状态）：
    //   background-color: #1e293b → 悬停时背景变为深蓝灰色（slate-800），给用户明确的交互反馈
    //     若改为#6366f1靛蓝色，则悬停效果过于鲜艳刺眼；若改为#0f172a则与背景色太接近，反馈不明显
    //   color: #f1f5f9 → 悬停时文字变为浅白色（slate-100），在深色背景上更清晰
    //     若保持#94a3b8灰色，则悬停前后对比度变化不足，反馈微弱
    m_toolBar->setStyleSheet(
        "QToolBar { background: transparent; spacing: 4px; }"
        "QToolButton {"
        "   background-color: transparent;"
        "   color: #94a3b8;"
        "   border: 1px solid #334155;"
        "   border-radius: 6px;"
        "   padding: 4px 10px;"
        "   font-size: 12px;"
        "}"
        "QToolButton:hover {"
        "   background-color: #1e293b;"
        "   color: #f1f5f9;"
        "}"
    );

    // 【添加复制动作到工具栏】addAction("复制")创建一个文本为"复制"的QAction并添加到工具栏
    // 返回值是QAction指针，保存到m_copyAction供后续连接信号使用
    // 工具栏会将此动作渲染为一个可点击的QToolButton按钮
    m_copyAction = m_toolBar->addAction("复制");
    
    // 【添加清空动作到工具栏】addAction("清空")创建"清空"按钮
    // m_clearAction保存返回的指针
    m_clearAction = m_toolBar->addAction("清空");

    // 【连接复制动作信号与槽】QAction::triggered信号在按钮被点击（或快捷键触发）时发射
    // connect语法：发送者(m_copyAction)，信号(&QAction::triggered)，接收者(this)，槽(&OutputPanel::onCopyAction)
    // 含义：用户点击"复制"按钮时，调用onCopyAction()将输出内容复制到剪贴板
    connect(m_copyAction, &QAction::triggered, this, &OutputPanel::onCopyAction);
    
    // 【连接清空动作信号与槽】用户点击"清空"按钮时，调用onClearAction()清除输出内容
    connect(m_clearAction, &QAction::triggered, this, &OutputPanel::onClearAction);

    // 【将工具栏加入头部布局】工具栏位于headerLayout的最右侧（因为前面有addStretch()弹簧）
    headerLayout->addWidget(m_toolBar);
    
    // 【将头部容器加入主布局】headerWidget作为主布局的第一个控件，位于最上方
    m_mainLayout->addWidget(headerWidget);

    // 【创建输出显示区】QTextEdit用于展示所有日志内容
    // 参数this表示父窗口是OutputPanel
    m_outputDisplay = new QTextEdit(this);
    
    // 【设置为只读】setReadOnly(true)禁止用户编辑输出内容，防止误删日志
    // 用户仍然可以选中文本、复制内容，但不能修改或删除
    m_outputDisplay->setReadOnly(true);
    
    // 【设置输出区对象名称】setObjectName用于QSS样式表通过#outputDisplay精确选择此控件
    // 外部样式表或ThemeManager可以通过此名称统一设置所有输出面板的样式
    m_outputDisplay->setObjectName("outputDisplay");
    
    // 【设置输出区QSS样式】定义深色主题下的输出区外观
    // background-color: #0f172a → 极深蓝黑色背景（slate-900），典型代码编辑器深色主题
    //   若改为#000000纯黑，则缺乏色彩层次，长时间阅读压抑
    //   若改为#1e293b（slate-800），则背景偏亮，与头部区域对比度不足
    // color: #e2e8f0 → 浅灰白色文字（slate-200），在深色背景上提供舒适的高对比度
    //   若改为#ffffff纯白，对比度过高，长时间阅读易造成视觉疲劳和眩光
    //   若改为#94a3b8灰色，则对比度不足，阅读吃力
    // border: 1px solid #334155 → 1像素深灰边框（slate-700），勾勒出输出区的边界
    //   若改为2px则边框过粗，占用空间且视觉笨重
    //   若去掉border，则输出区与周围背景融为一体，边界不清
    // border-radius: 8px → 8像素圆角，使输出区四边柔和，与整体圆角设计风格一致
    //   若改为0px则直角生硬；若改为16px则过于圆润，像大圆角卡片
    // padding: 12px → 内容与边缘之间12像素的内边距，确保文字不贴边
    //   若改为6px则文字过于靠近边缘，显得拥挤；若改为20px则内容缩进过多，浪费显示空间
    // font-family: 'Consolas', 'Monaco', 'Courier New', monospace → 等宽字体回退链
    //   Consolas是Windows系统自带的高品质等宽字体；Monaco是macOS经典等宽字体
    //   Courier New是跨平台通用等宽备用字体；monospace是CSS通用等宽字体族名称
    //   使用等宽字体保证代码和表格的对齐（如ASCII艺术、日志表格）
    // font-size: 13px → 13像素字号，比标准14px略小，可在有限空间显示更多内容
    //   若改为16px则显示行数减少；若改为11px则过小，长时间阅读困难
    // line-height: 1.6 → 行高为字号的1.6倍（约20.8像素），增加行间距使文本透气
    //   若改为1.0则行与行紧贴，阅读拥挤；若改为2.0则间距过大，一屏显示内容过少
    m_outputDisplay->setStyleSheet(
        "QTextEdit {"
        "   background-color: #0f172a;"
        "   color: #e2e8f0;"
        "   border: 1px solid #334155;"
        "   border-radius: 8px;"
        "   padding: 12px;"
        "   font-family: 'Consolas', 'Monaco', 'Courier New', monospace;"
        "   font-size: 13px;"
        "   line-height: 1.6;"
        "}"
    );
    
    // 【将输出显示区加入主布局】m_outputDisplay作为主布局的第二个控件，位于headerWidget下方
    // 由于QTextEdit的垂直尺寸策略是Expanding（自动扩展），它会占据所有剩余的垂直空间
    m_mainLayout->addWidget(m_outputDisplay);
}

/**
 * @brief 追加普通文本输出
 * @param text 要追加的文本字符串（支持HTML标签进行富文本渲染）
 *
 * @implementation
 * 1. 调用m_outputDisplay->append(text)将文本追加到QTextEdit末尾
 *    append()方法会自动在文本前添加换行（如果文档非空），并支持HTML解析
 * 2. 获取当前文本光标并移动到文档末尾，确保输出区自动滚动显示最新内容
 */
void OutputPanel::appendOutput(const QString& text)
{
    // 【追加文本到输出区】append()将text添加到QTextEdit文档末尾
    // 如果text包含HTML标签（如<b>、<span style="...">），QTextEdit会自动解析并渲染富文本效果
    // 如果text是纯文本，则直接按原样显示
    m_outputDisplay->append(text);
    
    // 【获取当前光标位置】textCursor()返回QTextEdit当前光标的副本
    QTextCursor cursor = m_outputDisplay->textCursor();
    
    // 【移动光标到文档末尾】movePosition(QTextCursor::End)将光标从当前位置移动到文档最后
    // 这是实现"自动滚动到最新消息"的关键：光标移动后QTextEdit会自动滚动视口确保光标可见
    cursor.movePosition(QTextCursor::End);
    
    // 【应用新的光标位置】setTextCursor(cursor)将光标重新设置回QTextEdit
    // 效果：如果追加的文本在可视区域下方，QTextEdit会自动向下滚动，使新内容进入视野
    // 若缺少此步骤，用户可能停留在旧日志位置，看不到最新输出
    m_outputDisplay->setTextCursor(cursor);
}

/**
 * @brief 追加步骤执行头信息
 * @param stepId 步骤编号（如1、2、3...）
 * @param description 步骤描述文本（如"读取文件内容"）
 *
 * @implementation
 * 1. 获取当前时间戳（HH:mm:ss格式）
 * 2. 格式化字符串："[HH:mm:ss] 步骤 N — 描述"
 * 3. 调用appendOutput()追加到输出区
 *
 * 输出示例："[14:32:08] 步骤 1 — 读取配置文件"
 * 时间戳帮助用户追踪每个步骤的执行时间点，便于调试和性能分析。
 */
void OutputPanel::appendStepHeader(int stepId, const QString& description)
{
    // 【获取当前时间戳】QDateTime::currentDateTime()获取系统当前日期时间
    // toString("HH:mm:ss")格式化为24小时制时:分:秒，如"14:32:08"
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    
    // 【格式化步骤头字符串】QString::arg()依次填充占位符
    // 格式："[14:32:08] 步骤 1 — 读取配置文件"
    // %1 = time（时间戳）
    // %2 = QString::number(stepId)（步骤编号转为字符串）
    // %3 = description（步骤描述）
    QString header = QString("[%1] 步骤 %2 — %3").arg(time, QString::number(stepId), description);
    
    // 【追加到输出区】调用appendOutput将格式化后的步骤头显示出来
    appendOutput(header);
}

/**
 * @brief 追加步骤执行输出
 * @param stepId 步骤编号（当前未使用，保留供未来扩展）
 * @param output Agent返回的输出文本（可能包含多行）
 *
 * @implementation
 * 1. 使用Q_UNUSED(stepId)消除"未使用参数"的编译器警告
 * 2. 将output中的所有换行符"\n"替换为"\n    "（换行后加4个空格缩进）
 * 3. 在整体文本前添加4个空格缩进
 * 4. 调用appendOutput()追加处理后的文本
 *
 * 缩进效果：使步骤输出在视觉上隶属于上方的步骤头，形成层级结构：
 *   [14:32:08] 步骤 1 — 读取文件
 *       文件内容第一行
 *       文件内容第二行
 *       ...
 */
void OutputPanel::appendStepOutput(int stepId, const QString& output)
{
    // 【消除未使用参数警告】Q_UNUSED宏告诉编译器stepId参数故意未被使用
    // 这是为了保留参数名以表明接口语义，同时避免编译器产生警告
    // 未来若需要按步骤过滤或关联输出，可直接使用此参数
    Q_UNUSED(stepId)
    
    // 【复制输出文本】QString是可写复制的（copy-on-write），此操作几乎无性能开销
    QString formatted = output;
    
    // 【替换换行符为缩进换行】将每个"\n"替换为"\n    "（换行符后加4个空格）
    // 效果：多行输出的每一行（除第一行外）前都有4空格缩进
    // 若output为"第一行\n第二行\n第三行"，替换后变为"第一行\n    第二行\n    第三行"
    formatted.replace("\n", "\n    ");
    
    // 【追加缩进后的输出】在整体前加4个空格，使第一行也有缩进
    // 最终效果：所有行都有至少4空格的缩进，与步骤头形成视觉层级
    appendOutput("    " + formatted);
}

/**
 * @brief 追加步骤执行结果
 * @param stepId 步骤编号（当前未使用，保留供未来扩展）
 * @param success true表示成功，false表示失败
 * @param message 结果描述（成功摘要或失败错误信息）
 *
 * @implementation
 * 1. 根据success选择状态标记：true→"[成功]"，false→"[失败]"
 * 2. 格式化字符串："[成功] 消息内容" 或 "[失败] 错误信息"
 * 3. 调用appendOutput()追加结果行
 * 4. 追加一个空行，与下一个步骤形成视觉分隔
 */
void OutputPanel::appendStepResult(int stepId, bool success, const QString& message)
{
    // 【消除未使用参数警告】stepId保留供未来扩展（如按步骤着色、步骤关联过滤）
    Q_UNUSED(stepId)
    
    // 【选择状态标记】三元运算符根据success布尔值选择对应的状态文本
    // true时：status = "[成功]"
    // false时：status = "[失败]"
    QString status = success ? "[成功]" : "[失败]";
    
    // 【格式化结果字符串】将状态标记和消息拼接
    // 示例（成功）："[成功] 文件读取完成，共1024字节"
    // 示例（失败）："[失败] 文件不存在: /path/to/file.txt"
    appendOutput(QString("%1 %2").arg(status, message));
    
    // 【追加空行】空行在视觉上分隔不同的步骤，使日志层次更清晰
    // 若缺少此空行，步骤结果与下一个步骤头会紧贴在一起，阅读困难
    appendOutput("");
}

/**
 * @brief 清空所有输出内容
 *
 * 调用QTextEdit::clear()删除输出显示区中的所有文本。
 * 效果：面板瞬间变为空白状态，适用于开始新任务前清理旧日志。
 * 注意：此操作不可撤销，clear()会直接清空内部文档缓冲区。
 */
void OutputPanel::clearOutput()
{
    m_outputDisplay->clear();
}

/**
 * @brief 清空操作处理槽函数
 *
 * @implementation
 * 1. 调用clearOutput()清除输出显示区的所有内容
 * 2. 发射clearRequested()信号，通知外部（如主窗口）清空操作已执行
 *    外部可在此信号的槽函数中记录操作日志或更新其他UI状态
 */
void OutputPanel::onClearAction()
{
    // 【清空输出内容】调用本类的clearOutput方法，将QTextEdit清空
    clearOutput();
    
    // 【发射清空请求信号】通知所有监听者：用户点击了清空按钮
    // 当前无外部连接，保留供未来扩展（如主窗口记录"用户于XX时清空输出"）
    emit clearRequested();
}

/**
 * @brief 复制操作处理槽函数
 *
 * @implementation
 * 1. 调用m_outputDisplay->toPlainText()获取输出区的纯文本内容（去除所有HTML标签）
 * 2. 使用QApplication::clipboard()->setText()将内容写入系统剪贴板
 * 3. 发射copyRequested()信号，通知外部复制操作已执行
 *
 * 用户可在其他应用（如记事本、聊天窗口）中使用Ctrl+V粘贴复制的内容。
 */
void OutputPanel::onCopyAction()
{
    // 【获取纯文本内容】toPlainText()将QTextEdit中的HTML内容转换为纯文本字符串
    // 所有HTML标签（如<div>、<span>）会被移除，只保留可见文本内容
    // 换行符会保留，因此复制到剪贴板后粘贴到其他应用时格式基本保持
    QString text = m_outputDisplay->toPlainText();
    
    // 【写入系统剪贴板】QApplication::clipboard()获取全局剪贴板对象
    // setText(text)将字符串写入剪贴板，覆盖之前的剪贴板内容
    // 此后用户可在任何支持粘贴的应用中使用Ctrl+V（或右键粘贴）获取此内容
    QApplication::clipboard()->setText(text);
    
    // 【发射复制请求信号】通知所有监听者：用户点击了复制按钮
    // 当前无外部连接，保留供未来扩展（如主窗口显示"已复制到剪贴板"提示气泡）
    emit copyRequested();
}
