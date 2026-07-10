/**
 * @file settingspanel.cpp
 * @brief LLM 设置面板实现 - SettingsPanel类的具体功能实现
 *
 * @details
 * SettingsPanel 提供 LLM（大语言模型）相关参数的图形化配置界面，
 * 包含 API 地址、API Key、模型名称、最大 Token 数和温度系数五个核心参数。
 *
 * 核心设计要点：
 * 1. 使用 QFormLayout 排列标签-输入控件对，标签列左对齐，输入控件列右对齐，形成整齐的表单视觉效果
 *    - QFormLayout自动处理两列的对齐和间距，比手动使用QGridLayout更简洁
 * 2. API Key 输入框使用 Password 回显模式（setEchoMode(QLineEdit::Password)），输入内容显示为黑色圆点
 *    - 视觉效果：旁人无法从屏幕上窥见真实密钥，提升安全性
 *    - 若改为Normal模式，则输入的密钥明文显示，存在安全风险
 * 3. 所有控件提供默认值（OpenAI 兼容接口配置），用户首次打开即可使用，无需手动填写
 * 4. 提供 getter/setter 方法，便于外部读取和初始化，符合封装原则
 * 5. 点击保存按钮发射 settingsChanged 信号，通知外部（如主窗口）进行持久化保存
 *    - 面板本身不直接读写文件，保持职责单一
 *
 * UI参数视觉效果详细说明：
 * - mainLayout->setContentsMargins(8, 8, 8, 8)：面板四周留8像素边距；若改为16px则内容与边框距离过大，显得空旷
 * - mainLayout->setSpacing(8)：标题、表单、按钮之间的垂直间距为8px；若改为16px则各区域间距过大，视觉松散
 * - formLayout->setSpacing(6)：每行表单（标签+输入框）之间的间距为6px；若改为12px则行距过大，表单显得稀疏
 * - titleLabel->setObjectName("settingsTitle")：设置对象名，便于QSS样式表通过#settingsTitle单独设置标题样式
 *   例如可设置更大字号（font-size: 18px）、加粗（font-weight: bold）、主色调（color: #6366f1）等
 * - m_apiUrlEdit->setPlaceholderText("API地址")：输入框为空时显示灰色占位文字"API地址"；
 *   若改为"https://..."则更具体，引导性更强；占位文字在输入内容后自动消失
 * - m_apiKeyEdit->setEchoMode(QLineEdit::Password)：输入的每个字符显示为黑色圆点（●●●）或星号（***），具体样式由系统主题决定
 * - m_maxTokensSpin->setRange(128, 8192)：限制最小值为128（低于此值模型可能无法生成有效回复），最大值为8192（防止资源耗尽）
 * - m_temperatureSpin->setSingleStep(0.1)：点击上下箭头每次增减0.1；若改为0.5则步进过大，无法精细调节
 */

#include "settingspanel.h"  // 【引入头文件】包含SettingsPanel类的声明，使本文件知道有哪些成员变量和方法需要实现

/**
 * @brief 构造函数 - 创建并初始化LLM设置面板的所有UI控件和布局
 * @param parent 父QWidget指针；若传入nullptr则此面板为独立窗口，否则嵌入父窗口
 *
 * @implementation
 * 布局结构（垂直方向，从上到下）：
 * mainLayout (QVBoxLayout) - 整个面板的顶级布局
 *   ├── titleLabel — "LLM 设置"标题标签（大号加粗文字）
 *   ├── formLayout (QFormLayout) - 表单布局，两列对齐
 *   │   ├── "API地址:"标签  : m_apiUrlEdit (默认 https://api.openai.com/v1/chat/completions)
 *   │   ├── "API Key:"标签  : m_apiKeyEdit (Password模式，留空提示)
 *   │   ├── "模型名称:"标签 : m_modelNameEdit (默认 gpt-4o-mini)
 *   │   ├── "最大Token数:"标签: m_maxTokensSpin (范围 128~8192，默认 2048)
 *   │   └── "温度:"标签     : m_temperatureSpin (范围 0.0~1.0，步进 0.1，默认 0.7)
 *   └── m_saveButton — "保存设置"按钮（居中或拉伸显示）
 *
 * 信号连接：保存按钮点击（QPushButton::clicked） → onSaveClicked()槽函数
 * 含义：用户点击保存按钮时，发射settingsChanged信号通知外部
 */
SettingsPanel::SettingsPanel(QWidget *parent)
    : QWidget(parent)  // 【初始化列表】调用父类QWidget构造函数，传入parent建立Qt对象树关系；parent决定面板的嵌入位置和生命周期
{
    // 【创建顶级垂直布局】QVBoxLayout使子控件沿垂直方向依次排列
    // 参数this表示此布局绑定到当前SettingsPanel面板
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 【设置布局四周边距】setContentsMargins(左, 上, 右, 下) = (8, 8, 8, 8)
    // 视觉效果：面板内的控件与面板边框之间保持8像素的空白内边距，避免控件紧贴边缘显得拥挤
    // 若改为setContentsMargins(16, 16, 16, 16)，则空白翻倍，内容区域向内收缩，整体显得空旷
    // 若改为setContentsMargins(0, 0, 0, 0)，则标题、输入框会直接贴到面板边缘，视觉压抑
    mainLayout->setContentsMargins(8, 8, 8, 8);
    
    // 【设置布局控件间距】setSpacing(8)表示布局中相邻两个控件之间的默认垂直间距为8像素
    // 视觉效果：标题、表单区域、保存按钮三者之间有8px的均匀间隙，层次分明但不疏离
    // 若改为setSpacing(16)，则各区域之间距离过大，面板内容显得稀疏分散
    // 若改为setSpacing(0)，则标题紧贴表单、表单紧贴按钮，没有呼吸空间，视觉压抑
    mainLayout->setSpacing(8);

    // 【创建标题标签】QLabel用于显示静态文本"LLM 设置"
    // 参数this表示父窗口是SettingsPanel，建立正确的父子对象树关系
    QLabel* titleLabel = new QLabel("LLM 设置", this);
    
    // 【设置标题对象名称】setObjectName用于QSS样式表通过#settingsTitle精确定位此标签
    // 样式表示例："#settingsTitle { font-size: 18px; font-weight: bold; color: #1e293b; }"
    // 效果：标题显示为18像素大号加粗深蓝色文字，在面板顶部突出显示
    // 若将font-size改为24px则标题过大，可能挤压下方表单空间
    titleLabel->setObjectName("settingsTitle");
    
    // 【将标题加入主布局】标题会位于面板最上方
    mainLayout->addWidget(titleLabel);

    // 【创建表单布局】QFormLayout自动将标签和输入控件排列成两列，标签左对齐，输入控件右对齐
    // 相比QGridLayout，QFormLayout更简洁，专为表单场景设计，自动处理标签和输入框的伙伴关系
    QFormLayout* formLayout = new QFormLayout();
    
    // 【设置表单项间距】setSpacing(6)表示每行表单（标签+输入框组合）之间的垂直间距为6像素
    // 视觉效果：五行参数之间保持6px的均匀间隙，紧凑但不拥挤
    // 若改为setSpacing(12)，则行距过大，表单纵向拉长，可能需要滚动才能看到全部内容
    // 若改为setSpacing(0)，则各行紧密堆叠，没有分隔感，难以区分不同参数
    formLayout->setSpacing(6);

    // 【创建API地址输入框】QLineEdit默认显示OpenAI兼容接口地址
    // 参数"https://api.openai.com/v1/chat/completions"作为默认文本显示在输入框中
    // 用户可以直接使用此默认值，也可以清空后输入自定义地址（如本地部署的Ollama地址）
    m_apiUrlEdit = new QLineEdit("https://api.openai.com/v1/chat/completions", this);
    
    // 【设置API地址占位提示】当输入框为空且未获得焦点时，显示灰色提示文字"API地址"
    // 视觉效果：灰色半透明文字（通常color: #9ca3af）提示用户此处应填写API地址
    // 若用户输入内容后删除至空，提示文字会重新出现
    // 若改为"例如 http://localhost:11434"，则更具引导性，帮助用户理解填写格式
    m_apiUrlEdit->setPlaceholderText("API地址");
    
    // 【将API地址行加入表单】addRow(标签文本, 控件指针)自动创建标签并排列到左侧，输入框排列到右侧
    // 标签"API地址:"使用系统默认字体，右对齐，与输入框的左侧保持固定距离
    formLayout->addRow("API地址:", m_apiUrlEdit);

    // 【创建API Key输入框】QLineEdit不设置默认文本，初始为空
    // API Key是敏感凭证，不同用户拥有不同密钥，因此不提供默认值
    m_apiKeyEdit = new QLineEdit(this);
    
    // 【设置API Key占位提示】提示用户此处填写API密钥，留空表示不使用
    // 视觉效果：灰色提示文字告知用户此字段可选
    m_apiKeyEdit->setPlaceholderText("API Key (留空则不使用)");
    
    // 【设置密码回显模式】setEchoMode(QLineEdit::Password)将输入的每个字符显示为掩码符号（如圆点●或星号*）
    // 安全效果：旁观者无法从屏幕上读取真实密钥，防止信息泄露
    // 若改为QLineEdit::Normal，则输入的密钥明文显示，旁边的人可直接看到，存在严重安全隐患
    // 若改为QLineEdit::NoEcho，则完全不显示任何字符（连圆点都不显示），用户不知道自己输入了什么，体验差
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    
    // 【将API Key行加入表单】标签"API Key:"在左，密码输入框在右
    formLayout->addRow("API Key:", m_apiKeyEdit);

    // 【创建模型名称输入框】默认显示"gpt-4o-mini"，这是OpenAI提供的高性价比小型模型
    // 用户可根据实际使用的模型进行修改（如"gpt-4o"、"claude-3-sonnet"、"qwen2-7b"等）
    m_modelNameEdit = new QLineEdit("gpt-4o-mini", this);
    
    // 【设置模型名称占位提示】提示用户此处填写模型标识
    m_modelNameEdit->setPlaceholderText("模型名称");
    
    // 【将模型名称行加入表单】标签"模型名称:"在左，输入框在右
    formLayout->addRow("模型名称:", m_modelNameEdit);

    // 【创建最大Token数选择框】QSpinBox专用于整数输入，带上下箭头按钮
    // 参数this表示父窗口是SettingsPanel
    m_maxTokensSpin = new QSpinBox(this);
    
    // 【设置Token数范围】setRange(128, 8192)限制最小值为128，最大值为8192
    // 范围意义：128是生成短回复的最低合理值（再低可能无法输出完整句子）
    // 8192是常见上下文窗口的上限（如gpt-4o-mini支持128K，但输出通常限制在4K~8K）
    // 若将上限改为32768，则支持更长的生成，但某些模型可能不支持，且消耗更多资源
    // 若将下限改为1，则可能导致模型输出被截断为无意义片段
    m_maxTokensSpin->setRange(128, 8192);
    
    // 【设置Token数默认值】setValue(2048)表示打开面板时默认选中2048
    // 2048是平衡值：足够生成详细回复，又不会过长；适合大多数对话场景
    // 若改为4096，则默认允许更长的回复；改为512则回复更简短
    m_maxTokensSpin->setValue(2048);
    
    // 【将最大Token数行加入表单】标签"最大Token数:"在左，选择框在右
    formLayout->addRow("最大Token数:", m_maxTokensSpin);

    // 【创建温度系数选择框】QDoubleSpinBox支持小数输入，适合连续值参数
    // 温度是LLM的核心采样参数，控制输出随机性
    m_temperatureSpin = new QDoubleSpinBox(this);
    
    // 【设置温度范围】setRange(0.0, 1.0)限制温度在0到1之间
    // 温度含义：0.0表示贪婪解码（每次选概率最高的词，输出最确定）
    // 1.0表示完全随机采样（输出最具创造性但可能不连贯）
    // 若上限改为2.0，则随机性过强，输出可能难以理解；但某些模型（如OpenAI）支持0~2
    m_temperatureSpin->setRange(0.0, 1.0);
    
    // 【设置温度步进值】setSingleStep(0.1)表示点击上下箭头每次增减0.1
    // 步进意义：0.1是精细调节的合理粒度（如0.7→0.8差异明显但可控）
    // 若改为0.5，则只能从0.0跳到0.5再到1.0，缺少中间值，调节过于粗糙
    // 若改为0.01，则步进过细，需要点击多次才有明显变化
    m_temperatureSpin->setSingleStep(0.1);
    
    // 【设置温度默认值】setValue(0.7)表示默认温度为0.7
    // 0.7是业界常用默认值：既有一定创造性，又不会过于随机失控
    // 代码生成场景可设为0.2~0.3（更确定），创意写作可设为0.8~1.0（更发散）
    m_temperatureSpin->setValue(0.7);
    
    // 【将温度行加入表单】标签"温度:"在左，浮点选择框在右
    formLayout->addRow("温度:", m_temperatureSpin);

    // 【将表单布局加入主布局】QFormLayout作为子布局嵌入QVBoxLayout
    // 表单区域会占据标题和保存按钮之间的所有垂直空间
    mainLayout->addLayout(formLayout);

    // 【创建保存按钮】QPushButton显示文本"保存设置"
    // 这是面板唯一的操作按钮，用户填写完参数后点击以确认变更
    m_saveButton = new QPushButton("保存设置", this);
    
    // 【设置保存按钮对象名称】用于QSS样式表选择，如"#saveButton { background: #22c55e; color: white; }"
    // 视觉效果：可将保存按钮设为绿色背景+白色文字，传达"确认/成功"的心理暗示
    // 若使用蓝色(#6366f1)则与主色调统一；使用橙色(#f97316)则突出但不突兀
    m_saveButton->setObjectName("saveButton");
    
    // 【将保存按钮加入主布局】按钮位于表单下方
    // 默认情况下按钮宽度由文本长度决定，可通过QSS设置min-width使其更宽
    mainLayout->addWidget(m_saveButton);

    // 【连接信号与槽：保存按钮点击→保存处理】
    // connect语法：发送者(m_saveButton)，信号(&QPushButton::clicked)，接收者(this)，槽(&SettingsPanel::onSaveClicked)
    // 含义：用户点击"保存设置"按钮时，发射clicked信号，进而调用onSaveClicked()槽函数
    // 解耦设计：按钮只负责通知"被点击了"，具体保存逻辑（写文件、通知其他模块）由槽函数处理
    connect(m_saveButton, &QPushButton::clicked, this, &SettingsPanel::onSaveClicked);
}

/**
 * @brief 析构函数
 *
 * SettingsPanel被销毁时自动调用。Qt的父子对象机制会自动释放所有子控件
 * （titleLabel、formLayout中的所有输入框、m_saveButton等），无需手动delete。
 * 保持为空即可，这是Qt内存管理的标准做法。
 */
SettingsPanel::~SettingsPanel()
{
    // Qt对象树自动管理内存，此处无需编写代码
}

/**
 * @brief 获取API地址
 * @return 当前API地址输入框中的文本字符串
 *
 * 实现说明：直接调用m_apiUrlEdit->text()获取QLineEdit的当前内容
 * 返回值可能是用户修改后的新地址，也可能是默认的OpenAI地址
 */
QString SettingsPanel::apiUrl() const
{
    return m_apiUrlEdit->text();
}

/**
 * @brief 获取模型名称
 * @return 当前模型名称输入框中的文本字符串
 *
 * 实现说明：直接调用m_modelNameEdit->text()获取内容
 * 使用场景：构造LLM请求时填充model字段
 */
QString SettingsPanel::modelName() const
{
    return m_modelNameEdit->text();
}

/**
 * @brief 获取API Key
 * @return 当前API Key输入框中的文本字符串（明文）
 *
 * @note 虽然输入框显示为密码掩码（圆点），但此方法返回原始明文字符串
 * 调用者应注意安全：不要将返回值打印到日志或显示在UI上
 * 若用户未输入，返回空字符串，表示请求时不携带Authorization头
 */
QString SettingsPanel::apiKey() const
{
    return m_apiKeyEdit->text();
}

/**
 * @brief 获取最大Token数
 * @return 当前QSpinBox中选中的整数值（范围128~8192）
 *
 * 实现说明：调用m_maxTokensSpin->value()获取当前数值
 * 使用场景：发送LLM请求时设置max_tokens参数，限制生成长度
 */
int SettingsPanel::maxTokens() const
{
    return m_maxTokensSpin->value();
}

/**
 * @brief 获取温度系数
 * @return 当前QDoubleSpinBox中选中的浮点数值（范围0.0~1.0）
 *
 * @note 温度值控制生成文本的随机性：
 * - 0.0~0.3：低温度，输出确定性强，适合代码、数学、事实问答
 * - 0.4~0.7：中温度，平衡创造性和连贯性，适合一般对话
 * - 0.8~1.0：高温度，输出更具创造性和多样性，适合创意写作、头脑风暴
 * 使用场景：发送LLM请求时设置temperature参数
 */
double SettingsPanel::temperature() const
{
    return m_temperatureSpin->value();
}

/**
 * @brief 设置API地址
 * @param url 要显示在API地址输入框中的URL字符串
 *
 * 实现说明：调用m_apiUrlEdit->setText(url)将输入框内容设置为指定字符串
 * 若url为空字符串，则输入框显示空白（此时占位提示"API地址"会显示）
 * 使用场景：从配置文件加载已保存的地址后，调用此方法初始化界面
 */
void SettingsPanel::setApiUrl(const QString& url)
{
    m_apiUrlEdit->setText(url);
}

/**
 * @brief 设置模型名称
 * @param name 要显示在模型名称输入框中的字符串
 *
 * 实现说明：调用m_modelNameEdit->setText(name)设置内容
 * 使用场景：加载配置、切换预设模型时调用
 */
void SettingsPanel::setModelName(const QString& name)
{
    m_modelNameEdit->setText(name);
}

/**
 * @brief 设置API Key
 * @param apiKey 要显示在API Key输入框中的密钥字符串
 *
 * 实现说明：调用m_apiKeyEdit->setText(apiKey)设置内容
 * 即使传入明文，输入框仍显示为密码掩码（圆点），保护视觉隐私
 * 若传入空字符串，则清空输入框，表示不使用API Key
 * 使用场景：从安全存储（如系统密钥环）加载已保存的密钥后初始化界面
 */
void SettingsPanel::setApiKey(const QString& apiKey)
{
    m_apiKeyEdit->setText(apiKey);
}

/**
 * @brief 设置最大Token数
 * @param tokens 要设置的最大Token数量（需在128~8192范围内）
 *
 * 实现说明：调用m_maxTokensSpin->setValue(tokens)
 * 若tokens超出范围（如<128或>8192），QSpinBox会自动截断到边界值
 * 例如传入100会被自动调整为128，传入10000会被调整为8192
 * 使用场景：根据模型规格或用户偏好调整生成长度限制
 */
void SettingsPanel::setMaxTokens(int tokens)
{
    m_maxTokensSpin->setValue(tokens);
}

/**
 * @brief 设置温度系数
 * @param temp 要设置的温度值（需在0.0~1.0范围内）
 *
 * 实现说明：调用m_temperatureSpin->setValue(temp)
 * 若temp超出范围，QDoubleSpinBox会自动截断到边界值
 * 使用场景：根据任务类型预设温度（如代码生成设为0.2，创意写作设为0.9）
 */
void SettingsPanel::setTemperature(double temp)
{
    m_temperatureSpin->setValue(temp);
}

/**
 * @brief 保存按钮点击事件处理槽函数
 *
 * @implementation
 * 发射settingsChanged()信号，通知所有监听此外部对象：
 * "用户已确认设置变更，当前所有参数值已就绪，请进行持久化保存"
 *
 * 本面板不直接执行文件写入或网络操作，保持职责单一：
 * - 面板职责：收集用户输入、提供默认值、验证输入范围（由QSpinBox/QDoubleSpinBox自动处理）
 * - 外部职责：将值保存到配置文件、应用新配置到运行中的LLM客户端
 *
 * 外部监听者可通过以下getter获取最新值：
 * - apiUrl()、apiKey()、modelName()、maxTokens()、temperature()
 */
void SettingsPanel::onSaveClicked()
{
    // 【发射设置变更信号】emit关键字触发settingsChanged信号
    // 所有通过connect连接到此信号的槽函数都会被Qt自动调用
    // 这是一个无参数信号，监听者需主动调用getter方法获取具体数值
    emit settingsChanged();
}
