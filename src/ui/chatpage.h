/**
 * @file chatpage.h
 * @brief 对话页面 —— 主界面核心聊天组件
 *
 * 【整体功能】
 * ChatPage 是应用程序最核心的聊天交互页面，采用三栏式布局：
 * - 左侧边栏（260px固定宽度）：对话历史列表、新建对话按钮、状态显示
 * - 中间聊天区（自适应宽度）：消息气泡显示区、底部输入控制栏
 * - 右侧面板（最小280px）：输出日志、任务步骤进度展示
 *
 * 【消息显示设计】
 * 用户消息：居右对齐，蓝色气泡（#4F46E5），圆角16px（左下角2px尖角）
 * AI消息：居左对齐，灰色气泡（主题色），圆角16px（右下角2px尖角）
 * 所有消息支持HTML富文本渲染，可显示粗体、斜体、代码块、超链接等格式。
 *
 * 【打字机效果】
 * AI回复采用流式打字机效果，每30ms显示5个字符，模拟真实打字过程。
 */
#ifndef CHATPAGE_H  // 【预处理器】防止重复包含
#define CHATPAGE_H

#include <QWidget>           // Qt控件基类
#include <QTextEdit>         // 多行文本编辑/显示
#include <QPushButton>       // 按钮控件
#include <QVBoxLayout>       // 垂直布局
#include <QHBoxLayout>       // 水平布局
#include <QListWidget>       // 列表控件（对话历史）
#include <QLabel>            // 文本标签
#include <QSplitter>         // 分割器（可拖拽调整左右面板宽度）
#include <QScrollArea>       // 滚动区域
#include <QTextBrowser>      // 富文本浏览器（支持HTML渲染和超链接点击）
#include <QString>           // Qt字符串类
#include <QRegularExpression> // 正则表达式（用于Markdown转HTML）
#include <QTimer>            // 定时器（打字机效果）
#include <QComboBox>         // 下拉选择框（模型选择器）
#include <QStackedWidget>    // 堆叠控件（右侧面板切换空状态/输出/任务列表）

#include "core/task.h"       // 引入任务相关数据结构（TaskPlan、StepStatus等）

/**
 * @brief 对话页面类
 *
 * 【Q_OBJECT宏】启用Qt元对象系统（信号槽、属性系统等）
 * 【继承关系】继承QWidget，是一个完整的可视化页面组件
 */
class ChatPage : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     *
     * 【初始化流程】
     * 1. 创建打字机定时器（30ms间隔）和自动保存定时器（3000ms间隔）
     * 2. 调用 setupUI() 构建三栏布局
     * 3. 调用 applyTheme() 应用主题样式
     * 4. 连接 ThemeManager 主题变化信号
     */
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();

    /**
     * @brief 添加一条聊天消息到显示区域
     * @param sender 发送者名称（"用户"或"AI"）
     * @param content 消息内容（纯文本，内部自动做HTML转义）
     * @param isUser true表示用户消息（居右蓝色气泡），false表示AI消息（居左灰色气泡）
     * @param saveToConversation true表示同时保存到当前对话历史中
     *
     * 【视觉效果】消息以HTML表格形式插入，确保左右对齐正确。
     * 用户消息背景为靛蓝色(#4F46E5)，AI消息背景为次级背景色。
     * 每条消息上方显示发送者名称和时间戳（HH:mm）。
     */
    void addMessage(const QString& sender, const QString& content, bool isUser = false, bool saveToConversation = true);

    /**
     * @brief 追加消息片段（流式输出）
     * @param chunk 本次接收到的文本片段
     *
     * 【使用场景】AI模型采用流式响应（SSE）时，每收到一段文字就调用此函数。
     * 内部会将片段累积到 m_currentAiMessage，并触发打字机定时器逐字显示。
     */
    void appendMessageChunk(const QString& chunk);

    /**
     * @brief 结束当前AI消息
     *
     * 【调用时机】当AI流式响应结束时调用。
     * 停止打字机定时器，将完整消息保存到对话历史，清空临时缓冲区。
     */
    void finishCurrentMessage();

    /** @brief 追加HTML富文本到输出面板（用于系统日志、计划摘要等） */
    void appendOutput(const QString& text);

    /** @brief 追加实时日志消息（LLM响应、Agent调用、系统状态） */
    void appendLog(const QString& html);

    /** @brief 追加步骤头部信息（步骤编号+描述） */
    void appendStepHeader(int stepId, const QString& description);

    /** @brief 追加步骤执行输出日志 */
    void appendStepOutput(int stepId, const QString& output);

    /** @brief 追加步骤执行结果（成功/失败） */
    void appendStepResult(int stepId, bool success, const QString& message);

    /** @brief 清空聊天显示区域 */
    void clearChat();

    /** @brief 清空输出面板 */
    void clearOutput();

    /**
     * @brief 设置当前执行的计划
     * @param plan 从Orchestrator接收到的TaskPlan
     */
    void setPlan(const TaskPlan& plan);

    /**
     * @brief 更新指定步骤的执行状态
     * @param stepId 步骤编号
     * @param status 新状态（Pending/Running/Completed/Failed）
     */
    void updateStepStatus(int stepId, StepStatus status);

    /** @brief 清除当前计划显示 */
    void clearPlan();

    /**
     * @brief 添加文件卡片到聊天区
     * @param name 文件名
     * @param path 文件路径
     * @param isDir 是否为文件夹
     * @param size 文件大小（字节）
     * @param modified 修改时间字符串
     */
    void addFileCard(const QString& name, const QString& path, bool isDir = false, qint64 size = 0, const QString& modified = "");

    /** @brief 获取当前选中的文件列表 */
    QStringList selectedFiles() const;

    /** @brief 清空已选文件列表 */
    void clearSelectedFiles();

    /**
     * @brief 设置输入框可用状态
     * @param enabled true表示可用，false表示禁用（变灰色不可输入）
     */
    void setInputEnabled(bool enabled);

    /**
     * @brief 显示/隐藏停止按钮
     * @param show true显示停止按钮并隐藏发送按钮，false反之
     *
     * 【使用场景】用户发送消息后，显示"停止"按钮（红色），允许中断AI生成；
     * AI生成完毕后，恢复显示发送按钮（蓝色）。
     */
    void showStopButton(bool show);

    /**
     * @brief 加载对话历史列表
     * @param conversations JSON格式的对话列表数据
     */
    void loadConversations(const QJsonArray& conversations);

    /**
     * @brief 加载指定对话的消息内容
     * @param index 对话在列表中的索引
     */
    void loadConversationMessages(int index);

signals:
    /**
     * @brief 用户点击发送按钮时发射
     * @param message 用户输入的文本内容
     *
     * 【信号连接】主窗口连接此信号到 LLM 客户端，将消息发送到 AI 模型。
     */
    void sendMessage(const QString& message);

    /** @brief 用户点击停止按钮时发射 */
    void stopClicked();

    /** @brief 用户点击新建对话按钮时发射 */
    void newChatClicked();

    /** @brief 用户选中某条对话历史时发射 */
    void conversationSelected(int index);

    /** @brief 用户选择文件时发射 */
    void filesSelected(const QStringList& files);

    /** @brief 用户删除对话时发射 */
    void deleteConversation(int index);

private slots:
    void onSendClicked();           ///< 发送按钮点击处理
    void onStopClicked();           ///< 停止按钮点击处理
    void onNewChatClicked();        ///< 新建对话按钮点击处理
    void onConversationClicked(QListWidgetItem* item); ///< 对话历史点击处理
    void onThemeChanged();          ///< 主题切换时重新应用样式
    void onTypingTimer();           ///< 打字机定时器回调（每30ms执行一次）
    void onAutoSaveTimer();         ///< 自动保存定时器回调（每3000ms执行一次）
    void onAttachButtonClicked();   ///< 附件按钮点击处理（打开文件选择对话框）
    void onRemoveFileClicked();     ///< 移除已选文件

private:
    void setupUI();                 ///< 构建整体三栏布局
    void setupSidebar();            ///< 构建左侧对话历史侧边栏
    void setupChatArea();           ///< 构建中间聊天区域
    void setupRightPanel();         ///< 构建右侧输出/任务面板
    void applyTheme();              ///< 应用当前主题样式表（QSS）
    void updateConversationList();  ///< 刷新对话历史列表显示
    void syncCurrentConversation(); ///< 同步当前聊天内容到对话列表

    // ========== 侧边栏组件 ==========
    QWidget* m_sidebar;             // 【侧边栏容器】固定宽度260px，暗色背景
    QListWidget* m_conversationList; // 【对话历史列表】显示所有过往对话的标题、预览、时间
    QLabel* m_sidebarTitle;         // 【"对话记录"标题】12px加粗，顶部显示
    QPushButton* m_newChatButton;   // 【"+ 新建"按钮】蓝色小按钮，点击创建新对话
    QLabel* m_statusLabel;          // 【底部状态标签】显示"离线模式 · 本地运行"和绿色状态点

    // ========== 聊天区域组件 ==========
    QWidget* m_chatArea;            // 【聊天区域容器】中间主要内容区
    QTextBrowser* m_chatDisplay;    // 【消息显示区】支持HTML渲染、超链接点击、自动滚动
    QTextEdit* m_messageInput;      // 【消息输入框】多行文本编辑，支持Shift+Enter换行，Enter发送
    QPushButton* m_sendButton;      // 【发送按钮】蓝色圆形图标按钮（纸飞机图标）
    QPushButton* m_stopButton;      // 【停止按钮】红色矩形按钮，AI生成时显示
    QPushButton* m_attachButton;    // 【附件按钮】回形针图标，点击选择文件上传
    QPushButton* m_codeButton;      // 【代码模式按钮】代码图标，切换代码输入模式
    QComboBox* m_modelSelector;     // 【模型选择器】下拉框，选择当前使用的AI模型

    // ========== 右侧面板组件 ==========
    QSplitter* m_mainSplitter;      // 【主分割器】可拖拽调整中间聊天区和右侧面板宽度比例
    QWidget* m_rightPanel;          // 【右侧面板容器】最小宽度280px
    QStackedWidget* m_rightStack;   // 【堆叠控件】在"空状态"/"输出面板"/"任务列表面板"之间切换
    QWidget* m_outputPanel;         // 【输出面板占位】实际为 OutputPanel 类型（前向声明避免头文件循环依赖）
    QWidget* m_taskListPanel;       // 【任务列表面板占位】实际为 TaskListPanel 类型

    // ========== 打字机效果相关 ==========
    QTimer* m_typingTimer;          // 【打字机定时器】间隔30ms，每次显示5个字符
    QString m_pendingText;          // 【待显示文本】当前正在打字机输出的完整AI消息
    int m_pendingIndex;             // 【当前显示位置】已显示到第几个字符

    // ========== 自动保存定时器 ==========
    QTimer* m_autoSaveTimer;        // 【自动保存定时器】间隔3000ms，自动同步对话标题和时间

    TaskPlan m_currentPlan;         // 【当前执行的计划】保存Planner生成的任务计划
    int m_currentConversationIndex; // 【当前选中的对话索引】-1表示无选中
    QString m_currentAiMessage;     // 【当前AI消息缓存】流式输出时累积的完整文本
    QStringList m_selectedFiles;    // 【当前选中的文件列表】用户通过附件按钮选择的文件

    /**
     * @brief 对话数据结构
     *
     * 【用途】在内存中保存每个对话的标题、时间戳和消息列表，
     * 用于侧边栏显示和持久化保存。
     */
    struct Conversation {
        QString title;              // 【对话标题】默认取第一条用户消息的前20字
        QString timestamp;          // 【时间戳】格式 yyyy-MM-dd HH:mm:ss
        QStringList messages;       // 【消息列表】每条格式为 "用户: xxx" 或 "AI: xxx"
        bool isActive;              // 【是否当前活跃】控制侧边栏高亮显示
    };
    QList<Conversation> m_conversations; // 【所有对话历史】内存中的对话列表
};

#endif // CHATPAGE_H
