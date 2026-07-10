#ifndef SETTINGSPANEL_H  // 【条件编译保护】若未定义SETTINGSPANEL_H宏，则编译以下代码；防止头文件被同一编译单元多次包含导致重定义错误
#define SETTINGSPANEL_H  // 【定义保护宏】定义SETTINGSPANEL_H标识符，标记此文件已被包含过一次

#include <QWidget>          // 【引入Qt基础控件类】QWidget是所有UI控件的基类，提供显示、布局、事件处理等基础能力；本类继承QWidget，因此必须包含此头文件
#include <QLineEdit>        // 【引入单行输入框类】QLineEdit提供单行文本输入、占位提示、密码掩码等功能；用于API地址、模型名称、API Key的输入
#include <QSpinBox>         // 【引入整数选择框类】QSpinBox提供带上下箭头的整数输入控件，可设置范围、步进、默认值；用于"最大Token数"参数配置
#include <QDoubleSpinBox>   // 【引入浮点数选择框类】QDoubleSpinBox与QSpinBox类似，但支持小数（双精度浮点数）；用于"温度系数"参数配置
#include <QPushButton>      // 【引入按钮类】QPushButton提供可点击按钮，支持文本标签和点击信号；用于"保存设置"按钮
#include <QFormLayout>      // 【引入表单布局类】QFormLayout以标签-输入控件成对的形式整齐排列，自动对齐标签列和输入控件列；用于LLM参数的标签和输入框对齐
#include <QVBoxLayout>      // 【引入垂直布局类】QVBoxLayout将子控件沿垂直方向排列；用于整体面板的上下结构（标题在上，表单在中，按钮在下）
#include <QLabel>           // 【引入标签类】QLabel用于显示静态文本或图片，不可编辑；用于面板标题"LLM 设置"

/**
 * @brief LLM设置面板类 - 提供大语言模型参数的图形化配置界面
 *
 * SettingsPanel将LLM相关的五个核心参数以表单形式呈现给用户：
 * - API地址：服务端接口URL
 * - API Key：访问凭证（密码掩码保护）
 * - 模型名称：使用的具体模型标识
 * - 最大Token数：生成文本的最大长度限制
 * - 温度系数：控制生成文本的随机性和创造性
 *
 * 设计特点：
 * 1. 使用QFormLayout实现标签和输入控件的自动对齐，界面整洁
 * 2. 提供getter/setter方法，便于外部（如主窗口、配置管理器）读取和修改值
 * 3. 点击保存按钮发射settingsChanged信号，通知外部进行持久化存储
 * 4. API Key输入框使用Password回显模式，保护敏感信息不被旁窥
 */
class SettingsPanel : public QWidget  // 【类声明】SettingsPanel继承QWidget，成为一个独立的UI面板组件；可嵌入主窗口的任意位置（如侧边栏、对话框、标签页）
{
    Q_OBJECT  // 【Qt元对象宏】启用信号与槽、运行时类型信息等Qt核心机制；必须放在类声明的私有区域首行

public:  // 【公有接口区】外部可访问的构造函数、析构函数、getter/setter方法

    // 【构造函数】explicit防止隐式类型转换；parent为父窗口指针
    // 若parent不为nullptr，面板会自动嵌入父窗口并在父窗口销毁时自动释放
    // 构造函数内部会创建所有UI控件（标题标签、5个输入控件、保存按钮）并建立布局
    explicit SettingsPanel(QWidget *parent = nullptr);

    // 【析构函数】对象销毁时自动调用；Qt父子对象机制会自动释放所有子控件，无需手动delete
    ~SettingsPanel();

    // 【获取API地址】读取API地址输入框的当前文本
    // 返回值：QString类型的URL字符串（如"https://api.openai.com/v1/chat/completions"）
    // 使用场景：主窗口在发送网络请求前调用此方法获取目标地址
    QString apiUrl() const;

    // 【获取模型名称】读取模型名称输入框的当前文本
    // 返回值：QString类型的模型标识（如"gpt-4o-mini"）
    // 使用场景：构造LLM请求体时填充model字段
    QString modelName() const;

    // 【获取最大Token数】读取整数选择框的当前数值
    // 返回值：int类型，范围128~8192（由setRange限定）
    // 使用场景：限制LLM生成回复的最大长度，防止产生过长内容消耗资源
    int maxTokens() const;

    // 【获取温度系数】读取浮点数选择框的当前数值
    // 返回值：double类型，范围0.0~1.0（由setRange限定）
    // 温度值含义：0.0表示完全确定性输出（每次相同），1.0表示完全随机；0.7为平衡值
    // 使用场景：控制生成文本的创造性和多样性
    double temperature() const;

    // 【获取API Key】读取API Key输入框的当前文本
    // 返回值：QString类型，可能为空字符串（表示不使用API Key）
    // 安全说明：虽然输入框显示为密码掩码（圆点），但此方法返回原始明文；调用者应注意不要明文打印到日志
    QString apiKey() const;

    // 【设置API地址】将API地址输入框的内容设置为指定URL
    // 参数url：要显示的URL字符串；若url为空字符串则输入框显示空白
    // 使用场景：从配置文件加载设置后，调用此方法初始化界面显示
    void setApiUrl(const QString& url);

    // 【设置模型名称】将模型名称输入框的内容设置为指定名称
    // 参数name：模型标识字符串（如"gpt-4o-mini"）
    // 使用场景：加载保存的配置或切换预设模型时调用
    void setModelName(const QString& name);

    // 【设置最大Token数】将整数选择框的值设置为指定数量
    // 参数tokens：Token数量（需在128~8192范围内，超出范围会被自动截断到边界值）
    // 使用场景：根据模型能力或用户需求调整生成长度限制
    void setMaxTokens(int tokens);

    // 【设置温度系数】将浮点数选择框的值设置为指定温度
    // 参数temp：温度值（需在0.0~1.0范围内，超出会被截断）
    // 使用场景：根据任务类型调整（创意写作用高温0.8~1.0，代码生成用低温0.0~0.3）
    void setTemperature(double temp);

    // 【设置API Key】将API Key输入框的内容设置为指定密钥
    // 参数apiKey：API密钥字符串；若为空字符串则清空输入框
    // 安全说明：设置后输入框仍显示为密码掩码（圆点），不会明文暴露
    void setApiKey(const QString& apiKey);

signals:  // 【信号声明区】当设置发生变更时通知外部，实现UI与业务逻辑的解耦

    // 【设置变更信号】用户点击"保存设置"按钮时发射此信号
    // 无参数：外部监听者可通过调用上述getter方法获取最新值
    // 连接方式：主窗口或配置管理器使用connect将此信号关联到保存配置到文件的槽函数
    // 设计意义：面板本身不直接操作文件或网络，只负责收集用户输入并通知外部"值已准备好"
    void settingsChanged();

private slots:  // 【私有槽函数区】只能由类内部或友元访问的槽函数

    // 【保存按钮点击槽】用户点击"保存设置"按钮时触发
    // 内部逻辑：发射settingsChanged()信号，通知外部当前所有参数已就绪
    // 与信号的连接：在构造函数中通过 connect(m_saveButton, &QPushButton::clicked, this, &SettingsPanel::onSaveClicked) 建立
    void onSaveClicked();

private:  // 【私有成员区】封装内部UI控件指针，外部不可直接访问，只能通过公有getter/setter操作

    QLineEdit* m_apiUrlEdit;       // 【API地址输入框指针】QLineEdit实例；显示和编辑API服务端地址；默认值为"https://api.openai.com/v1/chat/completions"
    QLineEdit* m_modelNameEdit;    // 【模型名称输入框指针】QLineEdit实例；显示和编辑模型标识；默认值为"gpt-4o-mini"
    QLineEdit* m_apiKeyEdit;       // 【API Key输入框指针】QLineEdit实例；显示为密码掩码模式（圆点），保护敏感信息；默认值为空字符串
    QSpinBox* m_maxTokensSpin;     // 【最大Token数选择框指针】QSpinBox实例；带上下箭头按钮，范围128~8192，默认值2048；步进值为1
    QDoubleSpinBox* m_temperatureSpin; // 【温度系数选择框指针】QDoubleSpinBox实例；范围0.0~1.0，步进0.1，默认值0.7；支持小数输入
    QPushButton* m_saveButton;     // 【保存按钮指针】QPushButton实例；显示文本"保存设置"；点击后触发onSaveClicked槽函数
};

#endif  // 【结束条件编译】对应开头的#ifndef，结束头文件保护区域
