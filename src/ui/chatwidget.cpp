/**
 * @file chatwidget.cpp
 * @brief 聊天界面控件实现 - ChatWidget类的具体功能实现
 *
 * @details
 * ChatWidget 提供基础的聊天 UI，包含：
 * - 只读的聊天消息显示区（QTextEdit，HTML 富文本渲染）
 * - 消息输入框（QLineEdit）和发送按钮（QPushButton）
 *
 * 核心设计要点：
 * 1. 用户消息和 AI 消息使用不同颜色区分，提升可读性
 *    - 用户消息：靛蓝色 (#6366f1) 加粗标题 + 浅蓝背景 (#e0e7ff) 圆角卡片
 *    - AI消息：红色 (#ef4444) 加粗标题 + 浅红背景 (#fef2f2) 圆角卡片
 *    - 颜色参数效果说明：#6366f1是偏紫的蓝色，给人科技、专业感；若改成#000000则变为纯黑，失去辨识度
 *    - 背景色参数效果说明：#e0e7ff是极淡的蓝色，若加深为#3b82f6则背景会过于刺眼，影响文字阅读
 * 2. 消息背景使用浅色圆角卡片（border-radius: 8px），使每条消息像气泡一样独立，提升视觉层次感
 *    - border-radius: 8px 表示四个角都磨成半径8像素的圆角；若改为0px则变成直角矩形，视觉生硬
 *    - border-radius: 16px则圆角过大，显得过于圆润甚至像圆形
 * 3. 支持点击按钮和按回车键两种发送方式，符合用户习惯
 * 4. 每次追加消息后自动滚动到底部，确保用户始终看到最新消息
 * 5. 所有消息内容经过 toHtmlEscaped() 转义，防止 HTML 注入攻击（如用户输入<script>标签不会被执行）
 */

#include "chatwidget.h"  // 【引入头文件】包含ChatWidget类的声明，使本文件知道类有哪些成员和方法
#include <QDateTime>     // 【引入日期时间类】用于获取当前系统时间，作为消息的时间戳显示在每条消息顶部

/**
 * @brief 构造函数 - 创建并初始化聊天界面的所有UI控件和布局
 * @param parent 父QWidget指针；若传入nullptr则此窗口为顶层独立窗口，否则嵌入父窗口
 *
 * @implementation
 * 布局结构（垂直方向，从上到下）：
 * m_mainLayout (QVBoxLayout) - 整个ChatWidget的顶级布局
 *   ├── m_chatDisplay  — 只读 QTextEdit（占据主要垂直空间，显示所有聊天消息）
 *   └── inputWidget    — 输入区域容器（QWidget，内部使用QHBoxLayout水平布局）
 *         ├── m_messageInput — QLineEdit（输入框，占位文本"输入消息..."，占据水平大部分空间）
 *         └── m_sendButton   — QPushButton（"发送"按钮，固定在输入框右侧）
 *
 * 信号-槽连接说明：
 * - 发送按钮点击（QPushButton::clicked信号） → onSendClicked()槽函数
 *   含义：当用户用鼠标左键点击"发送"按钮时，Qt会自动发射clicked信号，进而调用onSendClicked()处理发送逻辑
 * - 输入框回车键（QLineEdit::returnPressed信号） → onReturnPressed()槽函数
 *   含义：当输入框获得焦点且用户按下键盘回车键时，发射returnPressed信号，调用onReturnPressed()处理发送逻辑
 */
ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)  // 【初始化列表】调用父类QWidget的构造函数，传入parent参数；必须首先调用，建立Qt对象树父子关系
{
    // 【创建主布局】QVBoxLayout使子控件垂直堆叠排列；参数this表示此布局绑定到当前ChatWidget窗口
    m_mainLayout = new QVBoxLayout(this);
    
    // 【设置布局边距为0】setContentsMargins(左, 上, 右, 下)全部设为0
    // 视觉效果：布局内的控件会紧贴ChatWidget窗口的四周边缘，不留空白边距
    // 若改为setContentsMargins(10, 10, 10, 10)，则控件四周会出现10像素的空白内边距，整体内容会向内收缩
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 【设置布局控件间距为0】setSpacing(0)表示布局中相邻两个控件之间的默认间距为0像素
    // 视觉效果：聊天显示区和输入区域之间没有额外缝隙，紧密贴合
    // 若改为setSpacing(8)，则显示区和输入区之间会出现8像素的透明间隙，视觉上产生分隔感
    m_mainLayout->setSpacing(0);

    // 【创建聊天显示区】QTextEdit是富文本编辑器，支持HTML渲染
    // 参数this表示父窗口是ChatWidget，因此m_chatDisplay会随ChatWidget销毁而自动释放（Qt对象树机制）
    m_chatDisplay = new QTextEdit(this);
    
    // 【设置为只读】setReadOnly(true)禁止用户编辑显示区内容，防止误删历史消息
    // 用户仍然可以选中文本、复制内容，但不能输入或删除
    m_chatDisplay->setReadOnly(true);
    
    // 【设置对象名称】setObjectName用于在QSS样式表中通过#chatDisplay精确选择此控件
    // 例如QSS规则 "#chatDisplay { background: #f8fafc; }" 只会作用于这个QTextEdit，不会影响其他QTextEdit
    m_chatDisplay->setObjectName("chatDisplay");
    
    // 【将显示区添加到主布局】addWidget将控件加入布局；由于m_mainLayout是QVBoxLayout，m_chatDisplay会占据上方可用空间
    // 由于后续只添加了一个inputWidget，m_chatDisplay会自动拉伸占据所有剩余垂直空间（因为QTextEdit的默认尺寸策略是可拉伸的）
    m_mainLayout->addWidget(m_chatDisplay);

    // 【创建输入区域容器】QWidget是一个通用的容器控件，本身不可见，但可作为其他控件的父级和布局承载体
    // 将输入框和发送按钮放入同一个QWidget中，便于对输入区整体设置样式（如背景色、边框）
    QWidget* inputWidget = new QWidget(this);
    
    // 【创建输入区水平布局】QHBoxLayout使子控件沿水平方向（从左到右）排列
    // 参数inputWidget表示此布局绑定到inputWidget容器上
    QHBoxLayout* inputLayout = new QHBoxLayout(inputWidget);
    
    // 【设置输入区布局边距】四边各留8像素的内边距
    // 视觉效果：输入框和发送按钮不会紧贴容器边缘，四周有8px的空白呼吸空间，看起来更舒适
    // 若改为16px则空白过大，输入区显得臃肿；改为0px则控件贴边，显得拥挤
    inputLayout->setContentsMargins(8, 8, 8, 8);
    
    // 【设置输入区控件间距】相邻控件（输入框和发送按钮）之间留8像素间隙
    // 视觉效果：输入框和按钮之间有8px的间距，不会连在一起，便于区分两个可操作区域
    inputLayout->setSpacing(8);

    // 【创建消息输入框】QLineEdit是单行文本输入控件
    // 参数inputWidget表示父窗口是inputWidget，建立正确的父子对象树关系
    m_messageInput = new QLineEdit(inputWidget);
    
    // 【设置输入框对象名称】用于QSS样式表精确定位，如 "#messageInput { border: 1px solid #cbd5e1; }"
    m_messageInput->setObjectName("messageInput");
    
    // 【设置占位提示文本】当输入框为空且未获得焦点时，显示灰色提示文字"输入消息..."
    // 视觉效果：提示用户此处可以输入内容；输入任意字符后提示文字自动消失
    // 若改为"在此输入您的问题"，则提示内容变化，引导性更强
    m_messageInput->setPlaceholderText("输入消息...");
    
    // 【将输入框加入水平布局】addWidget默认将控件添加到布局末尾；输入框会在左侧，占据大部分水平空间
    // 因为QLineEdit的水平尺寸策略是Expanding（自动扩展），而QPushButton是Fixed（固定大小），所以输入框会拉伸占满剩余宽度
    inputLayout->addWidget(m_messageInput);

    // 【创建发送按钮】QPushButton显示文本"发送"
    // 参数inputWidget表示父窗口是inputWidget
    m_sendButton = new QPushButton("发送", inputWidget);
    
    // 【设置发送按钮对象名称】用于QSS样式表选择，如 "#sendButton { background: #6366f1; color: white; }"
    // 视觉效果：可将按钮设为靛蓝色背景+白色文字，使其在界面中突出显示
    m_sendButton->setObjectName("sendButton");
    
    // 【将发送按钮加入水平布局】按钮会位于输入框右侧（水平布局的添加顺序决定位置）
    // 按钮默认尺寸由文本"发送"的宽度和系统默认按钮高度决定
    inputLayout->addWidget(m_sendButton);

    // 【将输入区容器加入主布局】inputWidget作为主布局的第二个控件，会位于m_chatDisplay下方
    // 由于inputWidget的垂直尺寸策略是Fixed（固定高度），它只会占用必要的高度，剩余空间全部分配给m_chatDisplay
    m_mainLayout->addWidget(inputWidget);

    // 【连接信号与槽：按钮点击→发送处理】
    // connect语法说明：发送者对象(m_sendButton)，信号(&QPushButton::clicked)，接收者(this即ChatWidget)，槽(&ChatWidget::onSendClicked)
    // 含义：当用户点击"发送"按钮时，QPushButton会发射clicked信号，Qt会自动调用ChatWidget的onSendClicked()方法
    // 解耦优势：按钮不需要知道发送逻辑如何实现，只需通知"我被点击了"
    connect(m_sendButton, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    
    // 【连接信号与槽：回车键→发送处理】
    // QLineEdit::returnPressed信号在输入框获得焦点且用户按下回车键时发射
    // 连接到onReturnPressed槽函数，进而调用onSendClicked()实现统一的发送逻辑
    // 用户体验：支持键盘快捷操作，无需移动鼠标即可发送消息
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ChatWidget::onReturnPressed);
}

/**
 * @brief 析构函数
 *
 * ChatWidget被销毁时自动调用。由于Qt的父子对象机制（QObject树）：
 * - ChatWidget(this)作为父对象，其所有子对象（m_chatDisplay、m_messageInput、m_sendButton、inputWidget等）会自动递归销毁
 * - 因此本析构函数无需手动delete任何子控件，保持为空即可
 * - 这种自动内存管理是Qt框架的核心优势之一，有效防止内存泄漏
 */
ChatWidget::~ChatWidget()
{
    // 此处无需编写任何代码；Qt对象树会自动释放所有子控件
}

/**
 * @brief 向聊天窗口追加一条消息气泡
 * @param sender 发送者名称（如 "用户" 或 "AI"），显示在消息气泡顶部
 * @param content 消息正文（纯文本，方法内部会自动进行HTML转义，防止恶意脚本注入）
 * @param isUser true=用户消息, false=AI 消息；决定消息气泡的颜色主题
 *
 * @implementation
 * 执行流程：
 * 1. 获取当前系统时间戳（HH:mm:ss 格式，如 14:32:08）
 * 2. 根据 isUser 选择不同颜色样式：
 *    - 用户消息：标题文字使用靛蓝色 (#6366f1) 加粗显示；背景使用浅蓝色 (#e0e7ff) 圆角卡片
 *    - AI消息：标题文字使用红色 (#ef4444) 加粗显示；背景使用浅红色 (#fef2f2) 圆角卡片
 * 3. 生成 HTML字符串，包含三个部分：
 *    - 外层div：设置背景色和圆角（气泡容器）
 *    - 内层span：发送者名称（带颜色加粗） + 时间戳（灰色小字）
 *    - 内容div：消息正文（经过toHtmlEscaped转义，防止HTML注入）
 * 4. 使用 QTextEdit::append() 将HTML追加到显示区末尾
 * 5. 移动 QTextCursor 到文档末尾，实现自动滚动到最新消息
 *
 * UI参数视觉效果详细说明：
 * - color: #6366f1 → 靛蓝色，给人现代科技感；若改为#ff0000则变为刺眼红色，不符合用户消息定位
 * - color: #ef4444 → 亮红色，用于AI标识具有醒目效果；若改为#22c55e绿色则与"系统成功"色混淆
 * - background-color: #e0e7ff → 极浅蓝，几乎白偏蓝；若改为#6366f1则背景太深，文字无法看清
 * - background-color: #fef2f2 → 极浅红，几乎白偏粉；若改为#ef4444则背景过深
 * - border-radius: 8px → 8像素圆角，使矩形四角变圆滑；若改为16px则圆角更大像药丸形；改为0px则完全直角
 * - padding: 8px → 气泡内部文字与边缘保持8像素距离；若改为16px则内容更靠内，气泡显得更大更空
 * - margin: 4px → 气泡与其他消息之间保持4像素外边距；若改为12px则消息间距过大，阅读流断裂
 * - font-weight: bold → 文字加粗，发送者名称更突出；若改为normal则名称与普通文字 weight 相同，层级感下降
 * - font-size: 12px → 时间戳使用12像素小字，低调不抢注意力；若改为16px则时间戳与主文字同大，喧宾夺主
 * - color: #9ca3af → 时间戳使用中性灰色；若改为#000000则过于醒目，分散用户对消息内容的注意力
 */
void ChatWidget::addMessage(const QString& sender, const QString& content, bool isUser)
{
    // 【获取当前时间戳】QDateTime::currentDateTime()获取系统当前日期时间
    // toString("HH:mm:ss")格式化为24小时制的时:分:秒字符串，如"14:32:08"
    // 这个时间戳会显示在每条消息的右上角，标识消息发送时间
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    
    // 【选择标题文字样式】isUser为true时用靛蓝色(#6366f1)，否则用红色(#ef4444)
    // font-weight: bold使发送者名称加粗显示，在视觉上形成消息头部
    // 若去掉font-weight: bold，发送者名称与消息正文粗细相同，消息层级感降低
    QString style = isUser ? "color: #6366f1; font-weight: bold;" : "color: #ef4444; font-weight: bold;";
    
    // 【选择背景样式】isUser为true时用浅蓝背景(#e0e7ff)，否则用浅红背景(#fef2f2)
    // border-radius: 8px设置圆角半径；8px是一个适中的值，使矩形边缘柔和但不过分圆滑
    // padding: 8px设置内边距，确保文字不紧贴气泡边缘，提升阅读舒适度
    // margin: 4px设置外边距，使相邻消息气泡之间有轻微间隔，视觉上彼此独立
    // 若将border-radius改为0px，气泡变为直角矩形，视觉风格趋于生硬严肃
    // 若将padding改为4px，文字紧贴边缘，显得拥挤；改为16px则内容缩进过多，浪费空间
    QString bgStyle = isUser ? "background-color: #e0e7ff; border-radius: 8px; padding: 8px; margin: 4px;" 
                             : "background-color: #fef2f2; border-radius: 8px; padding: 8px; margin: 4px;";
    
    // 【追加消息头部HTML】使用QString::arg()方法依次填充占位符%1、%2、%3、%4
    // %1 = bgStyle（气泡背景样式）
    // %2 = style（发送者名称颜色加粗样式）
    // %3 = sender（发送者名称文本，如"用户"）
    // %4 = time（时间戳文本，如"14:32:08"）
    // 最终生成的HTML示例（用户消息）：
    // <div style='background-color: #e0e7ff; border-radius: 8px; padding: 8px; margin: 4px;'>
    //   <span style='color: #6366f1; font-weight: bold;'>用户</span>
    //   <span style='color: #9ca3af; font-size: 12px;'>14:32:08</span>
    // </div>
    m_chatDisplay->append(QString("<div style='%1'><span style='%2'>%3</span> <span style='color: #9ca3af; font-size: 12px;'>%4</span></div>")
                          .arg(bgStyle, style, sender, time));
    
    // 【追加消息正文HTML】content.toHtmlEscaped()将纯文本中的特殊HTML字符转义
    // 例如："<script>alert(1)</script>" 会被转义为 "&lt;script&gt;alert(1)&lt;/script&gt;"
    // 这是重要的安全防护措施，防止XSS（跨站脚本）攻击或意外破坏HTML结构
    // padding-left: 8px和padding-right: 8px使正文左右缩进，与发送者名称对齐
    // <br>在正文后插入空行，使下一条消息与前一条有一定垂直间隔
    m_chatDisplay->append(QString("<div style='padding-left: 8px; padding-right: 8px;'>%1</div><br>").arg(content.toHtmlEscaped()));
    
    // 【获取文本光标对象】QTextCursor用于程序化控制QTextEdit中的光标位置和选区
    // textCursor()返回当前显示区的光标对象（副本）
    QTextCursor cursor = m_chatDisplay->textCursor();
    
    // 【将光标移动到文档末尾】movePosition(QTextCursor::End)将光标从当前位置移动到文档最后
    // 这是实现"自动滚动到最新消息"的关键步骤
    cursor.movePosition(QTextCursor::End);
    
    // 【应用光标位置】setTextCursor(cursor)将移动后的光标重新设置回QTextEdit
    // 效果：QTextEdit的视口会自动滚动，确保光标位置（即文档末尾）在可视区域内
    // 若缺少此行，虽然消息已追加，但用户可能停留在历史消息位置，看不到最新回复
    m_chatDisplay->setTextCursor(cursor);
}

/**
 * @brief 清空所有聊天内容
 *
 * 调用QTextEdit::clear()方法，删除显示区中的所有文本内容
 * 效果：界面瞬间变为空白，如同刚打开新会话
 * 应用场景：用户点击"新建对话"或"清空记录"按钮时调用
 * 注意：此操作不可撤销，clear()会直接清空缓冲区，不会进入剪贴板
 */
void ChatWidget::clearChat()
{
    m_chatDisplay->clear();
}

/**
 * @brief 发送按钮点击事件处理槽函数
 *
 * @implementation
 * 执行流程：
 * 1. 调用m_messageInput->text()获取输入框当前文本（QString类型）
 * 2. 调用trimmed()去除字符串首尾的所有空白字符（空格、制表符、换行符）
 * 3. 使用isEmpty()判断处理后的字符串是否为空
 * 4. 若非空：
 *    - 使用emit关键字发射sendMessage(message)信号，通知外部逻辑层（如主窗口、网络模块）有新消息待发送
 *    - 调用m_messageInput->clear()清空输入框，为下一次输入做准备
 * 5. 若为空（如用户只输入了空格）：不做任何操作，避免发送空白消息
 *
 * 为什么使用trimmed()：
 * - 防止用户误触发送按钮导致只发送空格或换行
 * - 提升用户体验，自动清理不小心输入的前导/尾随空白
 */
void ChatWidget::onSendClicked()
{
    // 【读取并清理输入文本】text()获取QLineEdit当前内容；trimmed()移除首尾空白
    // 例如用户输入"  你好  "，trimmed后变为"你好"
    QString message = m_messageInput->text().trimmed();
    
    // 【判断消息是否有效】isEmpty()在字符串长度为0时返回true
    // 若用户未输入任何内容，或只输入了空格/制表符，trimmed后变为空字符串，此时不执行发送
    if (!message.isEmpty()) {
        // 【发射发送信号】emit是Qt关键字，表示发射信号
        // sendMessage(message)通知所有连接到该信号的外部对象：用户提交了一条新消息
        // 解耦设计：ChatWidget只管UI，实际的网络请求由外部监听者（如MainWindow或Agent）处理
        emit sendMessage(message);
        
        // 【清空输入框】clear()将QLineEdit内容设为空字符串
        // 视觉效果：输入框瞬间变为空白，光标回到起始位置，用户可立即输入下一条消息
        // 若缺少此行，用户发送后旧文本仍留在框内，需要手动删除，体验极差
        m_messageInput->clear();
    }
}

/**
 * @brief 输入框回车键按下事件处理槽函数
 *
 * @implementation
 * 此槽函数不实现独立逻辑，而是直接调用onSendClicked()
 * 这样无论是鼠标点击按钮还是键盘按回车，都走同一套发送处理流程
 * 避免代码重复，确保两种触发方式的行为完全一致
 *
 * 信号触发条件：
 * - QLineEdit必须具有键盘焦点（用户点击了输入框或Tab切换进入）
 * - 用户按下键盘上的Enter/Return键（主键盘区或数字小键盘区）
 */
void ChatWidget::onReturnPressed()
{
    // 【委托给统一发送处理】直接调用onSendClicked()，复用已有的发送逻辑
    // 代码复用优势：若未来修改发送逻辑（如添加敏感词过滤），只需修改onSendClicked一处即可
    onSendClicked();
}
