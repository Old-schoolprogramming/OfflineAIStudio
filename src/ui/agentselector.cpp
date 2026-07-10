/**
 * @file agentselector.cpp
 * @brief Agent 选择器控件实现
 *
 * @details
 * AgentSelector 提供可用 Agent 的列表展示和选择功能，
 * 基于 QListWidget 实现，每项显示 Agent 名称，悬停显示描述。
 *
 * 核心设计要点：
 * 1. Agent 指针存储在 QListWidgetItem 的 Qt::UserRole 数据中，便于后续获取
 * 2. 点击列表项时发射 agentSelected 信号，传递 Agent 名称
 * 3. 支持动态添加和清空 Agent 列表
 * 4. 提供 selectedAgentName() 查询当前选中项
 */

#include "agentselector.h"  // 【引入头文件】包含AgentSelector类的声明，使本文件知道有哪些成员和方法需要实现
#include "agent.h"          // 【引入Agent类】使用Agent::name()和Agent::description()方法获取Agent信息

/**
 * @brief 构造函数
 * @param parent 父 QWidget
 *
 * 【初始化流程详解】
 * 1. 创建QVBoxLayout垂直布局，四边留8像素内边距，控件间距8像素
 * 2. 创建标题标签"可用 Agent"，设置对象名以便QSS样式表定位
 * 3. 创建QListWidget列表控件，设置对象名以便QSS样式表定位
 * 4. 连接列表点击信号到onItemClicked槽函数
 *
 * 【布局结构】
 * AgentSelector (this)
 * └── m_mainLayout (QVBoxLayout)
 *     ├── m_titleLabel — "可用 Agent"标题
 *     └── m_agentList — QListWidget（Agent列表，占据剩余垂直空间）
 */
AgentSelector::AgentSelector(QWidget *parent)
    : QWidget(parent)  // 【初始化列表】调用父类QWidget构造函数，传入parent建立Qt对象树关系
{
    // 【创建主布局】QVBoxLayout使子控件垂直堆叠排列；参数this表示此布局绑定到当前AgentSelector面板
    m_mainLayout = new QVBoxLayout(this);

    // 【设置布局四周边距】setContentsMargins(左, 上, 右, 下) = (8, 8, 8, 8)
    // 视觉效果：面板内的控件与面板边框之间保持8像素的空白内边距，避免控件紧贴边缘显得拥挤
    // 若改为setContentsMargins(16, 16, 16, 16)，则空白翻倍，内容区域向内收缩，整体显得空旷
    // 若改为setContentsMargins(0, 0, 0, 0)，则标题和列表会直接贴到面板边缘，视觉压抑
    m_mainLayout->setContentsMargins(8, 8, 8, 8);

    // 【设置布局控件间距】setSpacing(8)表示标题与列表之间的垂直间距为8像素
    // 视觉效果：标题和列表之间有8px的均匀间隙，层次分明但不疏离
    // 若改为setSpacing(16)，则标题与列表距离过大，视觉上断开连接
    // 若改为setSpacing(0)，则标题紧贴列表，没有呼吸空间
    m_mainLayout->setSpacing(8);

    // 【创建标题标签】QLabel显示静态文本"可用 Agent"
    // 参数this表示父窗口是AgentSelector，建立正确的父子对象树关系
    m_titleLabel = new QLabel("可用 Agent", this);

    // 【设置标题对象名称】setObjectName用于在QSS样式表中通过#agentTitle精确选择此标签
    // 例如QSS规则 "#agentTitle { font-size: 14px; font-weight: bold; color: #94a3b8; }" 可将其设为灰色加粗小字
    // 若未设置对象名，则QSS只能通过类型选择器QLabel设置样式，影响所有QLabel
    m_titleLabel->setObjectName("agentTitle");

    // 【将标题加入主布局】标题会位于面板最上方
    m_mainLayout->addWidget(m_titleLabel);

    // 【创建Agent列表】QListWidget展示可选项列表，每项显示Agent名称
    // 参数this表示父窗口是AgentSelector
    m_agentList = new QListWidget(this);

    // 【设置列表对象名称】用于QSS样式表通过#agentList精确定位此列表
    // 例如可设置列表背景色、项悬停效果、选中效果等：
    // "QListWidget#agentList { background: transparent; border: none; }"
    // "QListWidget#agentList::item:hover { background-color: #1e293b; }"
    m_agentList->setObjectName("agentList");

    // 【将列表加入主布局】列表会位于标题下方
    // 由于QListWidget的垂直尺寸策略是Expanding（自动扩展），它会占据所有剩余的垂直空间
    m_mainLayout->addWidget(m_agentList);

    // 【连接信号与槽：列表点击→选择处理】
    // connect语法说明：发送者对象(m_agentList)，信号(&QListWidget::itemClicked)，接收者(this即AgentSelector)，槽(&AgentSelector::onItemClicked)
    // 含义：当用户点击列表中的某一项时，QListWidget会发射itemClicked信号，Qt自动调用AgentSelector的onItemClicked()方法
    // 解耦优势：列表不需要知道选择后如何处理，只需通知"某项被点击了"
    connect(m_agentList, &QListWidget::itemClicked, this, &AgentSelector::onItemClicked);
}

/**
 * @brief 析构函数
 *
 * 【Qt自动内存管理】AgentSelector(this)作为父对象，其所有子对象（m_titleLabel、m_agentList等）
 * 会自动递归销毁。因此本析构函数无需手动delete任何子控件，保持为空即可。
 */
AgentSelector::~AgentSelector()
{
    // 此处无需编写任何代码；Qt对象树会自动释放所有子控件
}

/**
 * @brief 向列表中添加一个 Agent
 * @param agent Agent 对象指针（非空）
 *
 * 【执行流程】
 * 1. 创建QListWidgetItem，文本设为agent->name()，父控件为m_agentList
 * 2. 设置工具提示为agent->description()，鼠标悬停时显示描述气泡
 * 3. 将agent指针存入item的Qt::UserRole数据中，供外部通过item反查Agent对象
 *
 * 【数据存储说明】Qt::UserRole是QListWidgetItem的自定义数据角色，
 * 不会显示在界面上，但可通过item->data(Qt::UserRole)读取。
 * 使用QVariant::fromValue(agent)将Agent指针包装为QVariant类型存储。
 */
void AgentSelector::addAgent(Agent* agent)
{
    // 【创建列表项】new QListWidgetItem(显示文本, 父列表)
    // 显示文本为agent->name()，如"代码助手"、"文档生成器"等
    // 父列表为m_agentList，使该项自动添加到列表末尾
    QListWidgetItem* item = new QListWidgetItem(agent->name(), m_agentList);

    // 【设置工具提示】setToolTip设置鼠标悬停时显示的提示文本
    // 当用户将鼠标指针停留在该项上几秒后，会弹出气泡显示agent->description()
    // 例如："用于生成高质量代码的AI智能代理"
    item->setToolTip(agent->description());

    // 【存储Agent指针】将agent指针存入该项的自定义数据中
    // QVariant::fromValue(agent)将原始指针包装为QVariant，便于Qt的数据模型存储
    // 外部可通过 QListWidgetItem* item; Agent* a = item->data(Qt::UserRole).value<Agent*>(); 反查Agent对象
    item->setData(Qt::UserRole, QVariant::fromValue(agent));
}

/**
 * @brief 清空所有 Agent 列表项
 *
 * 【功能说明】调用QListWidget::clear()删除列表中的所有项。
 * 效果：列表瞬间变为空白状态，所有QListWidgetItem对象被销毁。
 * 注意：此操作会同时清除项上存储的Qt::UserRole数据（Agent指针）。
 */
void AgentSelector::clearAgents()
{
    m_agentList->clear();
}

/**
 * @brief 获取当前选中的 Agent 名称
 * @return 选中项的文本（Agent名称）；若未选中任何项则返回空字符串""
 *
 * 【实现说明】调用QListWidget::currentItem()获取当前选中项指针。
 * 若列表支持多选，currentItem()返回当前焦点所在的项（不一定是唯一选中项）。
 * 若没有任何项被选中，currentItem()返回nullptr，此时返回空字符串。
 */
QString AgentSelector::selectedAgentName() const
{
    // 【获取当前选中项】currentItem()返回当前高亮/选中的列表项指针
    // 若用户从未点击过任何项，或clear()后未选择，返回nullptr
    QListWidgetItem* item = m_agentList->currentItem();

    // 【安全检查】若item不为nullptr，返回该项的显示文本；否则返回空字符串
    // item->text()返回构造时传入的agent->name()，如"代码助手"
    if (item) {
        return item->text();
    }

    // 【未选中情况】返回空字符串，表示当前没有选择任何Agent
    return "";
}

/**
 * @brief 列表项点击事件处理
 * @param item 被点击的 QListWidgetItem
 *
 * 【信号连接】在构造函数中通过 connect(m_agentList, &QListWidget::itemClicked, this, &AgentSelector::onItemClicked) 建立关联
 * 【触发条件】用户用鼠标左键点击列表中的某一项时触发
 * 【内部逻辑】若item非空，提取item->text()作为Agent名称，发射agentSelected信号通知外部
 *
 * 【参数安全性】item参数由Qt自动传入，理论上不会为nullptr，
 * 但进行非空检查是良好的防御性编程习惯。
 */
void AgentSelector::onItemClicked(QListWidgetItem* item)
{
    // 【非空检查】确保item有效，避免空指针解引用导致程序崩溃
    // 虽然Qt通常不会传入nullptr，但此检查增加了代码的健壮性
    if (item) {
        // 【发射选择信号】emit是Qt关键字，表示发射信号
        // agentSelected(item->text())通知所有连接到此信号的外部对象：
        // "用户选择了名为item->text()的Agent"
        // 外部监听者（如主窗口）可在槽函数中切换当前使用的Agent
        emit agentSelected(item->text());
    }
}
