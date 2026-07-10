/**
 * @file agentselector.h
 * @brief Agent选择器头文件 —— 展示并选择系统中可用的AI智能代理
 *
 * 【整体功能】
 * AgentSelector 是一个垂直排列的面板组件，包含：
 * - 顶部标题"可用 Agent"（灰色小字）
 * - 下方列表展示所有已注册的Agent名称
 * 用户点击某个Agent名称后，会发射 agentSelected 信号通知外部。
 *
 * 【使用场景】
 * 通常嵌入在主窗口的侧边栏或配置页面中，让用户查看和切换当前使用的Agent。
 */

#ifndef AGENTSELECTOR_H  // 【条件编译保护】若未定义 AGENTSELECTOR_H 宏，则编译以下内容；防止头文件被同一编译单元多次包含导致重定义错误
#define AGENTSELECTOR_H  // 【定义保护宏】定义 AGENTSELECTOR_H 标识符，标记此文件已被包含过一次

#include <QWidget>      // 【引入Qt基础控件类】QWidget是所有UI控件的基类，提供显示、布局、事件处理等基础能力；本类继承QWidget，因此必须包含此头文件
#include <QListWidget>  // 【引入列表控件类】QListWidget展示可选项列表，支持图标、富文本、点击事件；用于显示Agent名称列表
#include <QVBoxLayout>  // 【引入垂直布局类】QVBoxLayout将子控件沿垂直方向排列；用于标题在上、列表在下的上下布局
#include <QLabel>       // 【引入标签类】QLabel显示静态文本；用作"可用 Agent"标题

class Agent;  // 【前向声明】告诉编译器存在名为 Agent 的类（定义在其他头文件中），避免在此处完整引入 agent.h 造成循环依赖

/**
 * @brief Agent选择器类 - 展示系统中可用的AI智能代理列表
 *
 * 【Q_OBJECT宏】启用Qt元对象系统（信号与槽、运行时类型信息等）；必须放在类声明的私有区域首行
 * 【继承关系】继承QWidget，成为一个独立的UI面板组件；可嵌入主窗口的任意位置
 *
 * 设计特点：
 * 1. 使用QListWidget展示Agent名称列表，每项可显示名称和悬停提示
 * 2. Agent指针存储在QListWidgetItem的Qt::UserRole数据中，便于点击时反查
 * 3. 点击列表项发射agentSelected信号，实现UI与业务逻辑的解耦
 */
class AgentSelector : public QWidget
{
    Q_OBJECT  // 【Qt元对象宏】必须放在类声明的私有区域首行；启用信号与槽、运行时类型信息、属性系统等核心机制；缺少此宏则signals/slots关键字无法使用

public:  // 【公有接口区】外部可访问的构造函数、析构函数和操作方法

    /**
     * @brief 构造函数
     * @param parent 父QWidget指针；若传入nullptr则此面板为独立窗口，否则嵌入父窗口
     *
     * 【初始化流程】构造函数内部会：
     * 1. 创建QVBoxLayout垂直布局
     * 2. 创建标题标签"可用 Agent"
     * 3. 创建QListWidget列表控件
     * 4. 连接列表点击信号到onItemClicked槽函数
     *
     * 【内存管理】由于parent参数传入this，所有子控件在AgentSelector销毁时会由Qt自动释放
     */
    explicit AgentSelector(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     *
     * 【Qt自动内存管理】本类所有子控件（m_titleLabel、m_agentList等）在创建时都指定了this作为父对象，
     * 因此当AgentSelector对象被销毁时，Qt会自动删除所有子控件，无需手动delete。
     */
    ~AgentSelector();

    /**
     * @brief 添加一个Agent到列表
     * @param agent Agent对象指针（非空）
     *
     * 【功能说明】在列表末尾新增一项，显示文本为agent->name()，
     * 悬停时显示agent->description()作为工具提示。
     * 同时将agent指针存入该项的Qt::UserRole数据中，供外部通过列表项反查Agent对象。
     *
     * 【使用场景】系统初始化时或动态加载Agent后，调用此方法将Agent显示到列表中。
     */
    void addAgent(Agent* agent);

    /**
     * @brief 清空Agent列表
     *
     * 【功能说明】删除列表中的所有项，恢复为空列表状态。
     * 【使用场景】重新加载Agent列表前，先清空旧内容。
     */
    void clearAgents();

    /**
     * @brief 获取当前选中的Agent名称
     * @return 选中项的文本（Agent名称）；若未选中任何项则返回空字符串""
     *
     * 【使用场景】主窗口需要知道用户当前选中了哪个Agent时调用此方法。
     */
    QString selectedAgentName() const;

signals:  // 【信号声明区】当用户交互发生时通知外部，实现UI与业务逻辑的解耦

    /**
     * @brief Agent被选中的信号
     * @param agentName 选中的Agent名称字符串
     *
     * 【触发时机】用户点击列表中的某个Agent名称时发射。
     * 【连接方式】外部主窗口或控制器使用connect将此信号关联到切换Agent的处理槽函数。
     * 【设计意义】AgentSelector只负责界面展示和选择通知，不负责实际的Agent切换逻辑。
     */
    void agentSelected(const QString& agentName);

private slots:  // 【私有槽函数区】只能由类内部或友元访问的槽函数；用于处理控件事件

    /**
     * @brief 列表项被点击的槽函数
     * @param item 被点击的QListWidgetItem指针
     *
     * 【信号连接】在构造函数中通过 connect(m_agentList, &QListWidget::itemClicked, this, &AgentSelector::onItemClicked) 建立关联
     * 【内部逻辑】若item非空，提取item->text()作为Agent名称，发射agentSelected信号通知外部。
     */
    void onItemClicked(QListWidgetItem* item);

private:  // 【私有成员区】封装内部UI控件指针，外部不可直接访问

    QListWidget* m_agentList;   // 【Agent列表控件指针】QListWidget实例；展示所有Agent名称；支持点击选择和悬停提示；点击时触发onItemClicked槽函数
    QLabel* m_titleLabel;       // 【标题标签指针】显示"可用 Agent"文字；可通过QSS设置字号、颜色、粗细；默认使用系统默认字体和颜色
    QVBoxLayout* m_mainLayout;  // 【主布局指针】QVBoxLayout实例，垂直排列标题和列表；setContentsMargins(8,8,8,8)表示四边留8像素内边距，改大则内容与边缘距离增大
};

#endif  // 【结束条件编译】对应开头的#ifndef，结束头文件保护区域
