/**
 * @file iconhelper.h
 * @brief 图标辅助类头文件 —— 提供基于QPainter的SVG风格图标绘制功能
 *
 * 【整体功能】
 * IconHelper 是一个纯静态工具类（无需实例化），提供17种常用图标的绘制方法。
 * 每个方法接收尺寸和颜色参数，返回QPixmap对象，可直接用于Qt界面的QLabel、QPushButton等控件。
 *
 * 【使用方式】
 * QPixmap icon = IconHelper::logo(24, QColor("#3B82F6"));
 * label->setPixmap(icon);
 *
 * 【设计特点】
 * 1. 所有方法均为static，无需创建IconHelper实例
 * 2. 内部通过render()统一处理抗锯齿、设备像素比等绘制细节
 * 3. 图标使用QPainter的矢量绘制API（drawRect、drawEllipse、drawPath等），任意缩放不失真
 * 4. 返回的QPixmap支持高DPI显示（自动乘以设备像素比）
 *
 * 【支持的图标列表】
 * - logo(24, color)      → 品牌Logo（圆形内含方块）
 * - message(24, color)   → 消息/对话（对话气泡）
 * - chip(24, color)      → 芯片/AI（处理器形状）
 * - layers(24, color)    → 图层（叠层方块）
 * - moon(24, color)      → 月亮/暗色模式（月牙）
 * - sun(24, color)       → 太阳/亮色模式（圆形+光芒）
 * - search(24, color)    → 搜索（放大镜）
 * - check(24, color)     → 勾选（对勾）
 * - cloudUpload(24,color)→ 云端上传（云+向上箭头）
 * - refresh(24, color)   → 刷新（环形箭头）
 * - file(24, color)      → 文件（文档页）
 * - more(24, color)      → 更多选项（三个竖点）
 * - send(24, color)      → 发送（纸飞机/箭头）
 * - stop(24, color)      → 停止（方块）
 * - attach(24, color)    → 附件（回形针）
 * - code(24, color)      → 代码（尖括号）
 * - plus(24, color)      → 加号/添加（十字）
 */

#ifndef ICONHELPER_H  // 【条件编译保护】若未定义 ICONHELPER_H 宏，则编译以下内容；防止头文件被同一编译单元多次包含导致重定义错误
#define ICONHELPER_H  // 【定义保护宏】定义 ICONHELPER_H 标识符，标记此文件已被包含过一次

#include <QPixmap>  // 【引入像素图类】QPixmap是Qt的图像容器类，用于在屏幕上显示图片；本类所有图标方法返回QPixmap对象
#include <QColor>   // 【引入颜色类】QColor表示RGB/ARGB颜色值；本类所有图标方法接收QColor参数指定图标颜色

/**
 * @brief 图标辅助类 —— 使用QPainter绘制SVG风格图标的工具类
 *
 * 【类设计说明】
 * - 所有图标方法均为静态(static)，无需创建IconHelper实例即可调用
 * - 内部通过render()统一处理抗锯齿、设备像素比等绘制细节
 * - 图标使用QPainter的矢量绘制API，任意缩放不失真
 * - 返回的QPixmap自动适配高DPI屏幕（Retina/4K等）
 *
 * 【参数说明】
 * - size：图标宽度和高度（像素）；值越大图标越大；推荐值16/20/24/32/48
 * - color：图标颜色（QColor）；可使用QColor("#3B82F6")、QColor(59,130,246)等方式构造
 *   · 改#ff0000则图标变红色，改#00ff00则变绿色，改#000000则变黑色
 */
class IconHelper
{
public:  // 【公有接口区】所有静态图标绘制方法，外部可直接调用

    /**
     * @brief 绘制Logo图标 —— 品牌标识（圆形内含方块）
     * @param size 图标尺寸（宽度和高度相同，单位：像素）；改大则图标变大，改小则变小
     * @param color 图标颜色（QColor）；改其他颜色值可改变Logo颜色
     * @return 绘制完成的QPixmap；可直接用于QLabel::setPixmap()或QPushButton::setIcon()
     *
     * 【视觉效果】一个圆形轮廓内含一个小方块，类似应用Logo的简约风格
     * 【使用场景】应用标题栏Logo、关于页面品牌标识、任务栏图标等
     */
    static QPixmap logo(int size, const QColor& color);

    /**
     * @brief 绘制消息/对话图标 —— 对话气泡
     * @param size 图标尺寸（宽高相同）；改大则图标变大
     * @param color 图标颜色；改#3B82F6变蓝色，改#94A3B8变灰色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个圆角对话气泡，底部有小三角指向左下
     * 【使用场景】聊天页面标签图标、消息通知图标、未读消息提示等
     */
    static QPixmap message(int size, const QColor& color);

    /**
     * @brief 绘制芯片/AI图标 —— 处理器/电路板形状
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个方形芯片轮廓，四周有引脚线条，内部有电路细节
     * 【使用场景】模型配置页面标签图标、AI功能入口、硬件相关设置等
     */
    static QPixmap chip(int size, const QColor& color);

    /**
     * @brief 绘制图层图标 —— 叠层方块
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】两个或三个叠在一起的方块，有层次感
     * 【使用场景】技能导入页面标签图标、多层级菜单、堆叠内容展示等
     */
    static QPixmap layers(int size, const QColor& color);

    /**
     * @brief 绘制月亮/暗色模式图标 —— 月牙形状
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个弯弯的月牙（或带阴影的月亮）
     * 【使用场景】主题切换按钮（切换到暗色模式）、夜间模式指示器等
     */
    static QPixmap moon(int size, const QColor& color);

    /**
     * @brief 绘制太阳/亮色模式图标 —— 圆形+光芒
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个圆形太阳，周围有放射状光芒线条
     * 【使用场景】主题切换按钮（切换到亮色模式）、日间模式指示器等
     */
    static QPixmap sun(int size, const QColor& color);

    /**
     * @brief 绘制搜索图标 —— 放大镜
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个圆形镜片加斜向下方的手柄，经典放大镜造型
     * 【使用场景】搜索框左侧图标、搜索按钮、查找功能入口等
     */
    static QPixmap search(int size, const QColor& color);

    /**
     * @brief 绘制勾选图标 —— 对勾/勾选标记
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个从左下向右上再向右下的对勾形状
     * 【使用场景】复选框选中状态、操作成功提示、已完成标记等
     */
    static QPixmap check(int size, const QColor& color);

    /**
     * @brief 绘制云端上传图标 —— 云+向上箭头
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一朵轮廓云，下方或中间有一个向上的箭头
     * 【使用场景】技能导入页面上传区图标、文件上传按钮、云同步功能等
     */
    static QPixmap cloudUpload(int size, const QColor& color);

    /**
     * @brief 绘制刷新图标 —— 环形箭头
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个带箭头的环形或弧线，表示循环/刷新
     * 【使用场景】刷新按钮、重新加载、重置操作、同步状态指示等
     */
    static QPixmap refresh(int size, const QColor& color);

    /**
     * @brief 绘制文件图标 —— 文档页
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个带折角的文档页面轮廓
     * 【使用场景】文件列表、附件展示、文档管理、导出文件按钮等
     */
    static QPixmap file(int size, const QColor& color);

    /**
     * @brief 绘制更多选项图标 —— 三个竖点/横点
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】三个等间距排列的圆点（通常垂直或水平）
     * 【使用场景】上下文菜单按钮、更多操作入口、溢出菜单等
     */
    static QPixmap more(int size, const QColor& color);

    /**
     * @brief 绘制发送图标 —— 纸飞机/箭头
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个指向右上方的纸飞机或三角形箭头
     * 【使用场景】聊天页面发送按钮、邮件发送、消息投递等
     */
    static QPixmap send(int size, const QColor& color);

    /**
     * @brief 绘制停止图标 —— 方块
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个实心或空心的正方形
     * 【使用场景】停止生成按钮、终止操作、录音/播放停止等
     */
    static QPixmap stop(int size, const QColor& color);

    /**
     * @brief 绘制附件图标 —— 回形针
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个倾斜的回形针/曲别针形状
     * 【使用场景】附件上传按钮、文件关联、添加附件等
     */
    static QPixmap attach(int size, const QColor& color);

    /**
     * @brief 绘制代码图标 —— 尖括号
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一对尖括号"</>"或"<>"，代表代码/编程
     * 【使用场景】代码助手入口、开发者模式、代码片段展示等
     */
    static QPixmap code(int size, const QColor& color);

    /**
     * @brief 绘制加号/添加图标 —— 十字
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     *
     * 【视觉效果】一个等臂十字"+"
     * 【使用场景】新建按钮、添加项目、展开更多、放大操作等
     */
    static QPixmap plus(int size, const QColor& color);

private:  // 【私有成员区】内部辅助方法，外部不可直接访问

    /**
     * @brief 通用图标渲染方法 —— 所有图标方法的底层绘制引擎
     * @param size 图标尺寸（宽高相同）；决定返回QPixmap的宽度和高度（已乘以设备像素比）
     * @param color 图标颜色；作为画笔(QPen)和画刷(QBrush)的颜色
     * @param draw 绘制回调函数指针；接收QPainter指针和绘制区域QRect参数
     *             各图标方法在此回调中实现具体的矢量绘制逻辑（drawRect、drawEllipse等）
     * @return 绘制完成的QPixmap；图像已启用抗锯齿，适配高DPI屏幕
     *
     * 【内部处理流程】
     * 1. 获取屏幕设备像素比（DPR），计算实际绘制尺寸 = size * DPR
     * 2. 创建QPixmap（实际尺寸，带Alpha通道）
     * 3. 创建QPainter，启用抗锯齿（setRenderHint(QPainter::Antialiasing)）
     * 4. 设置画笔颜色（QPen）和画刷颜色（QBrush）
     * 5. 调用draw回调函数进行具体绘制
     * 6. 设置设备像素比（setDevicePixelRatio(DPR)）
     * 7. 返回QPixmap
     *
     * 【为什么需要DPR处理】
     * 在高DPI屏幕（如Retina MacBook、4K显示器）上，一个逻辑像素对应多个物理像素。
     * 若直接按size绘制，图标会模糊。通过乘以DPR绘制，再设置setDevicePixelRatio，
     * Qt会自动在显示时缩放到正确大小，保持图标清晰锐利。
     */
    static QPixmap render(int size, const QColor& color, void (*draw)(QPainter*, const QRect&));
};

#endif  // 【结束条件编译】对应开头的#ifndef，结束头文件保护区域
