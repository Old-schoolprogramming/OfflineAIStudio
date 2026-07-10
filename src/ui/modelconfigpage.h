/**
 * @file modelconfigpage.h
 * @brief 模型配置页面头文件
 *
 * 【整体功能】
 * 本文件声明了 ModelConfigPage 类，它是应用程序中用于配置 AI 模型连接的设置页面。
 * 页面上有：连接状态显示区、API 地址输入框、模型名称输入框、已安装模型下拉选择框、
 * 推理后端单选按钮组（CPU/CUDA/Metal）、Temperature/TopP/MaxTokens 等高级参数调节框，
 * 以及已安装模型卡片网格。
 *
 * 【页面布局说明】
 * 整个页面放在一个 QScrollArea（滚动区域）中，当内容超出屏幕高度时可以上下滚动查看。
 * 内容从上到下依次是：连接状态卡片 → 模型设置组 → 高级参数组 → 底部操作按钮 → 已安装模型网格。
 */

#ifndef MODELCONFIGPAGE_H  // 【预处理器指令】防止头文件被重复包含；若未定义 MODELCONFIGPAGE_H 则继续编译
#define MODELCONFIGPAGE_H  // 【预处理器指令】定义 MODELCONFIGPAGE_H 宏，标记此文件已被包含

// 引入 Qt 框架提供的各类 UI 控件头文件
#include <QWidget>          // 【Qt基类】所有可视化控件的基类；本页面继承自 QWidget
#include <QLineEdit>        // 【输入控件】单行文本输入框；用于输入 API 地址和模型名称
#include <QSpinBox>         // 【输入控件】整数数值调节框；用于 MaxTokens、上下文长度等整数参数
#include <QDoubleSpinBox>   // 【输入控件】浮点数值调节框；用于 Temperature、TopP 等小数参数
#include <QPushButton>      // 【按钮控件】普通按压按钮；用于"测试连接"、"保存"、"恢复默认"
#include <QComboBox>        // 【选择控件】下拉选择框；用于显示从服务器获取的已安装模型列表
#include <QVBoxLayout>      // 【布局管理】垂直布局；将子控件从上到下排列
#include <QLabel>           // 【显示控件】文本标签；用于显示标题、提示文字、状态信息
#include <QGridLayout>      // 【布局管理】网格布局；将控件按行和列排列（高级参数区域使用）
#include <QGroupBox>        // 【容器控件】分组框；可将相关控件视觉上分组（带边框和标题）
#include <QScrollArea>      // 【容器控件】滚动区域；当内容超出可视范围时显示滚动条
#include <QEvent>           // 【事件系统】Qt 事件基类；用于事件过滤（如鼠标点击模型卡片）

/**
 * @brief 模型配置页面类
 *
 * 【继承关系】继承自 QWidget，因此本身是一个可视化的矩形窗口/页面
 * 【Q_OBJECT 宏】必须添加，用于启用 Qt 的元对象系统（信号槽、反射等）
 *
 * 页面包含以下功能模块：
 * 1. 连接状态卡片 — 显示当前是否已连接到模型服务器
 * 2. 模型设置 — API 端点地址、模型名称、推理后端选择
 * 3. 高级参数 — Temperature、TopP、MaxTokens 等调节
 * 4. 已安装模型网格 — 以卡片形式展示服务器上已安装的模型
 */
class ModelConfigPage : public QWidget
{
    Q_OBJECT  // 【Qt宏】启用信号槽、属性系统等元对象特性；必须有，否则信号槽无法使用

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针，默认为 nullptr（表示无父窗口，或由外部布局管理）
     *
     * 【调用时机】当主窗口创建设置页面时调用，如 new ModelConfigPage(this)
     * 【内部动作】构造函数中会依次调用 setupUI() 创建界面、applyTheme() 应用颜色主题
     */
    explicit ModelConfigPage(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 【Qt内存管理】所有使用"父对象"方式创建的子控件（如 new QLabel(this)）
     * 会在本对象销毁时由 Qt 自动释放，无需手动 delete。
     */
    ~ModelConfigPage();

    /**
     * @brief 事件过滤器
     * @param obj 发生事件的目标对象
     * @param event 事件对象（鼠标点击、键盘按下等）
     * @return true 表示事件已处理，不再继续传递；false 表示继续正常处理
     *
     * 【用途】拦截模型卡片的鼠标点击事件，实现点击卡片自动填充模型名称的功能
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief 获取当前配置的 API 地址
     * @return API URL 字符串（用户输入框中的内容）
     *
     * 【使用场景】主窗口调用此函数获取用户填写的地址，传给网络客户端进行连接测试
     */
    QString apiUrl() const;

    /**
     * @brief 获取当前选中的模型名称
     * @return 模型名称字符串
     *
     * 【逻辑说明】如果"已安装的模型"下拉框已启用且有选中项，优先返回下拉框内容；
     * 否则返回手动输入框中的文本。这样允许用户既可以从列表选择，也可以手动输入。
     */
    QString modelName() const;

    /**
     * @brief 动态填充已安装模型列表
     * @param modelNames 从 API 服务器获取到的模型名称列表
     *
     * 【调用时机】当"测试连接"按钮点击后，主窗口通过网络请求获取到服务器上的模型列表，
     * 然后调用此函数将模型名称填入下拉选择框，并刷新下方模型卡片网格。
     */
    void populateInstalledModels(const QStringList& modelNames);

    /**
     * @brief 更新已安装模型网格显示
     * @param modelNames 模型名称列表
     *
     * 【视觉效果】根据传入的模型名称重新生成下方卡片网格，每张卡片显示模型名称、类型和状态。
     */
    void updateModelGrid(const QStringList& modelNames);

    /**
     * @brief 获取当前已安装模型的名称列表
     * @return 模型名称字符串列表
     */
    QStringList installedModelNames() const;

    /**
     * @brief 更新连接状态显示
     * @param connected true 表示连接成功，false 表示未连接或连接失败
     * @param modelDetail 模型详情文本（如 "Qwen-7B · 本地部署"），仅在连接成功时显示
     *
     * 【视觉效果】连接成功时：状态圆点变绿色、状态文字变"模型已连接"、徽章变绿色"已连接"；
     * 连接失败时：状态圆点变灰色、状态文字变"未连接"、徽章变灰色"未连接"。
     */
    void updateConnectionStatus(bool connected, const QString& modelDetail = "");

    /**
     * @brief 设置测试连接按钮的可用状态
     * @param enabled true 表示按钮可以点击，false 表示按钮变灰不可点击
     *
     * 【使用场景】点击测试连接后，在等待网络响应期间将按钮禁用，防止用户重复点击。
     */
    void setTestButtonEnabled(bool enabled);

    /**
     * @brief 获取最大生成 Token 数
     * @return MaxTokens 数值（整数）
     *
     * 【参数含义】AI 模型一次最多生成多少个词元（token）；值越大回答可能越长，但速度越慢。
     */
    int maxTokens() const;

    /**
     * @brief 获取 Temperature 采样温度
     * @return Temperature 数值（0.0 ~ 2.0 之间的小数）
     *
     * 【参数含义】控制 AI 输出的随机程度。值越低（接近0）回答越确定/保守；
     * 值越高（如1.0以上）回答越随机/有创意。常用值为 0.7。
     */
    double temperature() const;

    /**
     * @brief 获取 TopP 核采样参数
     * @return TopP 数值（0.0 ~ 1.0 之间的小数）
     *
     * 【参数含义】另一种控制随机性的参数；常用值为 0.9，表示只从概率累积前90%的词中采样。
     */
    double topP() const;

    /**
     * @brief 获取上下文长度
     * @return 上下文长度数值（整数，单位：token）
     *
     * 【参数含义】模型能"记住"的最大对话长度；超过此长度后最早的消息会被遗忘。
     */
    int contextLength() const;

    /**
     * @brief 获取 GPU 显存限制
     * @return GPU 显存限制数值（单位：GB）
     *
     * 【参数含义】限制模型推理时可使用的显存上限；根据显卡实际显存调整，防止溢出。
     */
    int gpuMemoryLimit() const;

    /**
     * @brief 获取并行线程数
     * @return 并行线程数（整数）
     *
     * 【参数含义】模型推理时使用的 CPU/GPU 并行计算线程数；通常设为物理核心数或略少。
     */
    int parallelThreads() const;

    /**
     * @brief 获取当前选中的推理后端
     * @return 后端名称字符串（"CPU" / "CUDA" / "Metal"）
     *
     * 【用途】告诉底层推理框架使用哪种硬件加速方式运行模型。
     */
    QString backend() const;

    /**
     * @brief 设置 API 地址
     * @param url API URL 字符串
     *
     * 【使用场景】程序启动时从配置文件中读取上次保存的地址，自动填入输入框。
     */
    void setApiUrl(const QString& url);

    /**
     * @brief 设置模型名称
     * @param name 模型名称字符串
     *
     * 【使用场景】程序启动时从配置文件中恢复上次使用的模型名称。
     */
    void setModelName(const QString& name);

    /**
     * @brief 设置最大生成 Token 数
     * @param tokens MaxTokens 数值
     */
    void setMaxTokens(int tokens);

    /**
     * @brief 设置 Temperature 采样温度
     * @param temp Temperature 数值
     */
    void setTemperature(double temp);

signals:
    /**
     * @brief 设置发生变更时发出的信号
     *
     * 【信号槽机制说明】这是 Qt 的"信号"声明。当用户点击"保存配置"按钮时，
     * 此信号会被发射（emit）。主窗口或其他组件可以通过 connect() 连接此信号，
     * 在信号触发时执行保存到文件、同步到网络客户端等操作。
     *
     * 【连接示例】
     * connect(modelConfigPage, &ModelConfigPage::settingsChanged,
     *         mainWindow, &MainWindow::onSettingsChanged);
     */
    void settingsChanged();

    /**
     * @brief 测试连接按钮被点击时发出的信号
     *
     * 【触发时机】用户点击界面上的"测试连接"按钮后发射。
     * 【接收方】通常由主窗口接收，然后调用网络客户端尝试连接 API 地址验证可用性。
     */
    void testConnectionClicked();

    /**
     * @brief 恢复默认设置按钮被点击时发出的信号
     *
     * 【触发时机】用户点击"恢复默认"按钮后发射。
     * 【接收方】主窗口接收后可选择是否弹出确认对话框，然后重置所有参数。
     */
    void restoreDefaultsClicked();

public slots:
    /**
     * @brief 保存按钮点击槽函数
     *
     * 【槽函数说明】槽函数是 Qt 中用于接收信号的函数。当保存按钮的 clicked() 信号发射时，
     * 此槽函数被自动调用。内部会发射 settingsChanged() 信号通知外部。
     */
    void onSaveClicked();

    /**
     * @brief 测试连接按钮点击槽函数
     *
     * 【信号连接】在 setupUI() 中通过 connect(m_testConnectionBtn, &QPushButton::clicked,
     * this, &ModelConfigPage::onTestConnectionClicked) 将按钮点击与本槽函数绑定。
     */
    void onTestConnectionClicked();

    /**
     * @brief 恢复默认按钮点击槽函数
     *
     * 【内部逻辑】将 API 地址、模型名称等所有输入框重置为程序预设的默认值，
     * 然后发射 restoreDefaultsClicked() 信号。
     */
    void onRestoreDefaultsClicked();

    /**
     * @brief 推理后端选择变更槽函数
     * @param button 被选中的单选按钮指针
     *
     * 【触发方式】QButtonGroup 中某个 QRadioButton 被选中时会触发此槽函数。
     */
    void onBackendSelected(QAbstractButton* button);

    /**
     * @brief 主题变更响应槽函数
     *
     * 【触发方式】当用户切换暗色/亮色主题时，ThemeManager 会发射 themeChanged 信号，
     * 本页面通过 connect() 连接该信号到此槽函数，从而重新应用颜色样式。
     */
    void onThemeChanged();

private:
    /**
     * @brief 初始化整体界面布局
     *
     * 【内部流程】创建主滚动区域 → 创建内容容器 → 依次调用 setupConnectionCard()、
     * setupModelSettings()、setupAdvancedSettings()、setupModelGrid() 创建各区域 →
     * 添加底部保存/恢复按钮栏 → 将所有内容放入滚动区域。
     */
    void setupUI();

    /**
     * @brief 构建连接状态卡片
     *
     * 【视觉效果】一个带背景色的圆角矩形卡片，左侧有绿色/灰色状态圆点和文字说明，
     * 右侧是"测试连接"按钮。卡片宽度填满整个内容区。
     */
    void setupConnectionCard();

    /**
     * @brief 构建模型设置区域
     *
     * 【包含内容】
     * - "模型设置"标题（15px 粗体）
     * - 模型端点输入框（带"未连接/已连接"状态徽章）
     * - 模型名称手动输入框
     * - 已安装模型下拉选择框（初始禁用，测试连接成功后启用）
     * - 推理后端单选按钮组（CPU / CUDA / Metal）
     */
    void setupModelSettings();

    /**
     * @brief 构建高级参数设置区域
     *
     * 【包含内容】在一个圆角卡片内，使用 2 列网格布局放置：
     * Temperature、TopP、MaxTokens、Context Length、GPU 显存限制、并行线程数。
     * 每个参数都有标签文字和对应的数值调节框。
     */
    void setupAdvancedSettings();

    /**
     * @brief 构建已安装模型网格
     *
     * 【视觉效果】在页面最下方，以网格形式排列模型卡片（每行最多3张）。
     * 每张卡片显示模型名称、大小、类型和状态徽章。
     */
    void setupModelGrid();

    /**
     * @brief 应用当前主题样式
     *
     * 【QSS样式表说明】本函数从 ThemeManager 获取当前主题的颜色值，
     * 拼接成一大段 CSS 风格的样式字符串，然后调用 setStyleSheet() 应用到页面。
     * 样式表中定义了各种控件的背景色、文字色、边框、圆角、内边距等视觉效果。
     */
    void applyTheme();

    // ========== 成员变量声明（以下指针都在构造函数中通过 new 创建）==========

    QScrollArea* m_scrollArea;          // 【滚动容器】整个页面的滚动区域；当窗口高度不够时显示垂直滚动条
    QWidget* m_contentWidget;           // 【内容面板】滚动区域内部的实际内容容器；所有子控件都放在这里
    QWidget* m_connectionCard;          // 【连接状态卡片】显示连接状态（已连接/未连接）的圆角矩形面板
    QWidget* m_modelSettingsGroup;      // 【模型设置分组】包含 API 地址、模型名称、后端选择等控件的区域
    QWidget* m_advancedGroup;           // 【高级参数分组】包含 Temperature/TopP 等调节控件的区域
    QWidget* m_installedModelsGroup;    // 【已安装模型分组】包含模型卡片网格的区域

    // --- 连接状态相关控件 ---
    QLabel* m_statusLabel;              // 【状态标题标签】显示"未连接"或"模型已连接"；14px 中等粗细字体
    QLabel* m_statusDetail;             // 【状态详情标签】显示"请配置模型端点并测试连接"或模型详情；12px 小字
    QLabel* m_statusDot;                // 【状态指示圆点】一个 10×10 像素的小圆点；绿色表示已连接，灰色表示未连接
    QLabel* m_connectedBadge;           // 【连接状态徽章】显示在 API 地址输入框右侧的胶囊形状标签（"未连接"/"已连接"）
    QPushButton* m_testConnectionBtn;   // 【测试连接按钮】点击后发射 testConnectionClicked() 信号

    // --- 模型设置相关控件 ---
    QLineEdit* m_apiUrlEdit;            // 【API地址输入框】用户在此输入本地模型服务的地址，如 http://localhost:8080/v1
    QLineEdit* m_modelNameEdit;         // 【模型名称输入框】手动输入模型名称；测试连接成功后，可从下拉框选择覆盖此内容
    QComboBox* m_modelCombo;            // 【已安装模型下拉框】测试连接通过后动态填充服务器返回的模型列表
    QButtonGroup* m_backendGroup;       // 【推理后端按钮组】管理 CPU/CUDA/Metal 三个单选按钮，确保同一时间只能选一个

    // --- 高级参数调节控件 ---
    QDoubleSpinBox* m_temperatureSpin;  // 【Temperature调节框】范围 0~2，步进 0.1，默认值 0.7；控制输出随机性
    QDoubleSpinBox* m_topPSpin;         // 【TopP调节框】范围 0~1，步进 0.1，默认值 0.9；核采样参数
    QSpinBox* m_maxTokensSpin;          // 【MaxTokens调节框】范围 1~32768，默认值 4096；最大生成词元数
    QSpinBox* m_contextLengthSpin;      // 【上下文长度调节框】范围 1~32768，默认值 8192；对话记忆长度
    QSpinBox* m_gpuMemorySpin;          // 【GPU显存调节框】范围 1~128，默认值 8；单位 GB
    QSpinBox* m_parallelThreadsSpin;    // 【并行线程调节框】范围 1~64，默认值 4；推理线程数

    // --- 底部操作按钮 ---
    QPushButton* m_saveBtn;             // 【保存配置按钮】点击后发射 settingsChanged 信号
    QPushButton* m_restoreBtn;          // 【恢复默认按钮】点击后将所有参数恢复为预设默认值

    /**
     * @brief 模型信息结构体
     *
     * 【用途】存储从服务器获取或本地预置的每个模型的元数据，
     * 用于在下方网格中生成模型卡片。
     */
    struct ModelInfo {
        QString name;       // 【模型名称】如 "Qwen-7B-Instruct"
        QString size;       // 【模型大小】如 "7B"、"13B" 或 "未知"
        QString type;       // 【模型类型】如 "代码生成"、"通用对话"、"数学推理"
        QString status;     // 【模型状态】如 "已安装"、"运行中"
        bool isRunning;     // 【是否运行中】true 表示当前正在服务中
    };
    QList<ModelInfo> m_models; // 【模型列表】存储当前要展示在网格中的所有模型信息
};

#endif // MODELCONFIGPAGE_H
