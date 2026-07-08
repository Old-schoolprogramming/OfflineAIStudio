/**
 * @file iconhelper.cpp
 * @brief 矢量图标绘制辅助类实现
 *
 * @details
 * IconHelper 使用 QPainter 绘制 SVG 风格的矢量图标，
 * 所有图标均为代码绘制，零外部资源依赖，支持任意分辨率缩放。
 *
 * 核心设计要点：
 * 1. 通用渲染框架：render() 方法提供统一的画布创建、抗锯齿和画笔设置
 * 2. 每个图标方法接收 size 和 color 参数，返回 QPixmap
 * 3. 图标几何坐标均基于 QRect 的相对比例计算，确保任意尺寸下保持比例
 * 4. 画笔宽度根据尺寸自动缩放（size / 12.0），保证线条粗细一致
 */

#include "iconhelper.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <cmath>

/**
 * @brief 通用图标渲染框架
 * @param size 图标尺寸（宽高相同，单位：像素）
 * @param color 图标线条颜色
 * @param draw 绘制回调函数，接收 QPainter 指针和 QRect 绘制区域
 * @return 渲染完成的 QPixmap（带透明背景）
 *
 * @implementation
 * 1. 创建 size×size 的透明 QPixmap
 * 2. 初始化 QPainter，启用抗锯齿渲染
 * 3. 设置画笔：指定颜色、宽度（size/12.0）、圆角线帽和连接
 * 4. 不填充画刷（Qt::NoBrush），仅绘制线条
 * 5. 计算内边距为 size/10 的绘制区域 QRect
 * 6. 调用 draw 回调执行具体绘制逻辑
 * 7. 结束绘制并返回 QPixmap
 */
QPixmap IconHelper::render(int size, const QColor& color, void (*draw)(QPainter*, const QRect&))
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, size / 12.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QRect r(size / 10, size / 10, size - size / 5, size - size / 5);
    draw(&p, r);
    p.end();
    return pixmap;
}

/**
 * @brief 绘制应用 Logo 图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：正六边形外框 + 中心 "AI" 文字
 * - 六边形顶点基于尺寸比例计算（中心 0.5, 顶部 0.08, 左右 0.08/0.92）
 * - 使用 QPolygonF 定义六个顶点
 * - 文字使用粗体，字号为 size/4.5（最小 6pt），居中绘制
 */
QPixmap IconHelper::logo(int size, const QColor& color)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    qreal s = size;
    QPolygonF poly;
    poly << QPointF(s * 0.5, s * 0.08);
    poly << QPointF(s * 0.92, s * 0.35);
    poly << QPointF(s * 0.92, s * 0.65);
    poly << QPointF(s * 0.5, s * 0.92);
    poly << QPointF(s * 0.08, s * 0.65);
    poly << QPointF(s * 0.08, s * 0.35);
    p.setPen(QPen(color, s / 12.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    p.drawPolygon(poly);
    QFont font = p.font();
    font.setPointSizeF(qMax(6.0, s / 4.5));
    font.setBold(true);
    p.setFont(font);
    p.setPen(color);
    p.drawText(pixmap.rect(), Qt::AlignCenter, "AI");
    p.end();
    return pixmap;
}

/**
 * @brief 绘制消息气泡图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：圆角矩形 + 左下小尾巴
 * - 主体为横向圆角矩形（圆角半径为高度的 0.2 倍）
 * - 尾巴使用 QPolygonF 三点折线：从左下边缘向左下延伸再折回
 */
QPixmap IconHelper::message(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal m = r.width() * 0.08;
        QRectF b(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 3);
        p->drawRoundedRect(b, b.height() * 0.2, b.height() * 0.2);
        QPolygonF tail;
        tail << QPointF(b.left() + b.width() * 0.15, b.bottom());
        tail << QPointF(b.left() + b.width() * 0.05, b.bottom() + b.height() * 0.45);
        tail << QPointF(b.left() + b.width() * 0.35, b.bottom());
        p->drawPolyline(tail);
    });
}

/**
 * @brief 绘制芯片/处理器图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：外圆角矩形 + 内正方形 + 8 根引脚线
 * - 外框为圆角矩形（圆角半径为宽度的 0.15 倍）
 * - 内框为居中正方形（边长为外框的 0.5 倍）
 * - 8 根引脚线：上下各 3 根（中、左 0.25、右 0.75），左右各 1 根（中心）
 *   引脚延伸长度为外框高度的 0.25~0.35 倍
 */
QPixmap IconHelper::chip(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal m = r.width() * 0.12;
        QRectF outer(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
        p->drawRoundedRect(outer, outer.width() * 0.15, outer.width() * 0.15);
        qreal im = outer.width() * 0.25;
        QRectF inner(outer.left() + im, outer.top() + im, outer.width() - im * 2, outer.height() - im * 2);
        p->drawRect(inner);
        qreal pins[8][4] = {
            {0.5, 0, 0.5, -0.35}, {0.5, 1, 0.5, 1.35},
            {0, 0.5, -0.35, 0.5}, {1, 0.5, 1.35, 0.5},
            {0.25, 0, 0.25, -0.25}, {0.75, 0, 0.75, -0.25},
            {0.25, 1, 0.25, 1.25}, {0.75, 1, 0.75, 1.25}
        };
        for (int i = 0; i < 8; ++i) {
            p->drawLine(
                outer.left() + outer.width() * pins[i][0], outer.top() + outer.height() * pins[i][1],
                outer.left() + outer.width() * pins[i][2], outer.top() + outer.height() * pins[i][3]
            );
        }
    });
}

/**
 * @brief 绘制图层堆叠图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：3 层菱形（平行四边形视角）纵向堆叠
 * - 每层为横向菱形：中心点对称，半宽 0.35×width，半高 0.11×height
 * - 三层分别位于纵向 0.18、0.46、0.74 位置
 * - 使用 QPolygonF 四点绘制闭合菱形
 */
QPixmap IconHelper::layers(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x();
        qreal w = r.width() * 0.7;
        qreal h = r.height() * 0.22;
        for (int i = 0; i < 3; ++i) {
            qreal y = r.top() + r.height() * (0.18 + i * 0.28);
            QPolygonF poly;
            poly << QPointF(cx, y - h * 0.6);
            poly << QPointF(cx + w * 0.5, y);
            poly << QPointF(cx, y + h * 0.6);
            poly << QPointF(cx - w * 0.5, y);
            p->drawPolygon(poly);
        }
    });
}

/**
 * @brief 绘制月亮/夜间模式图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：圆形减去偏移小圆（月牙效果）
 * - 主圆：半径为宽度的 0.38 倍，完整 360° 圆弧
 * - 切割圆：向右偏移 0.35×rad，半径为 0.7×rad，绘制 60°~300° 弧
 * - 使用 QPainterPath::subtracted() 布尔减法生成月牙路径
 */
QPixmap IconHelper::moon(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x();
        qreal cy = r.center().y();
        qreal rad = r.width() * 0.38;
        QPainterPath path;
        path.moveTo(cx + rad, cy);
        path.arcTo(cx - rad, cy - rad, rad * 2, rad * 2, 0, 360);
        QPainterPath cut;
        qreal off = rad * 0.35;
        cut.moveTo(cx + off + rad * 0.7, cy);
        cut.arcTo(cx + off - rad * 0.7, cy - rad * 0.7, rad * 1.4, rad * 1.4, 60, 240);
        path = path.subtracted(cut);
        p->drawPath(path);
    });
}

/**
 * @brief 绘制太阳/日间模式图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：中心圆 + 8 根放射状光芒线
 * - 中心圆：半径为宽度的 0.22 倍
 * - 8 根光芒线：角度间隔 45°，内端距圆心 1.5×rad，外端 2.3×rad
 * - 使用 cos/sin 计算端点坐标
 */
QPixmap IconHelper::sun(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x();
        qreal cy = r.center().y();
        qreal rad = r.width() * 0.22;
        p->drawEllipse(QPointF(cx, cy), rad, rad);
        for (int i = 0; i < 8; ++i) {
            qreal angle = i * 45 * 3.14159 / 180;
            qreal inner = rad * 1.5;
            qreal outer = rad * 2.3;
            p->drawLine(
                cx + cos(angle) * inner, cy + sin(angle) * inner,
                cx + cos(angle) * outer, cy + sin(angle) * outer
            );
        }
    });
}

/**
 * @brief 绘制搜索/放大镜图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：圆形镜片 + 45° 斜向下手柄
 * - 圆圈中心向左上偏移宽度的 0.08 倍，半径为 0.32×width
 * - 手柄从圆圈右下边缘沿 45° 方向延伸，长度为半径的 0.7 倍
 */
QPixmap IconHelper::search(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x() - r.width() * 0.08;
        qreal cy = r.center().y() - r.height() * 0.08;
        qreal rad = r.width() * 0.32;
        p->drawEllipse(QPointF(cx, cy), rad, rad);
        qreal angle = 45 * 3.14159 / 180;
        p->drawLine(cx + cos(angle) * rad, cy + sin(angle) * rad,
                    cx + cos(angle) * rad * 1.7, cy + sin(angle) * rad * 1.7);
    });
}

/**
 * @brief 绘制对勾/确认图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：折线对勾（V 形右侧上扬）
 * - 三点折线：左下 0.2×width → 中下 0.42×width, 0.72×height → 右上 0.8×width, 0.28×height
 * - 使用 QPolygonF 三点绘制，不闭合
 */
QPixmap IconHelper::check(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        QPolygonF poly;
        poly << QPointF(r.left() + r.width() * 0.2, r.top() + r.height() * 0.5);
        poly << QPointF(r.left() + r.width() * 0.42, r.top() + r.height() * 0.72);
        poly << QPointF(r.left() + r.width() * 0.8, r.top() + r.height() * 0.28);
        p->drawPolyline(poly);
    });
}

/**
 * @brief 绘制云上传图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：三朵重叠椭圆云 + 向上箭头
 * - 云体由三个不同位置的椭圆组成，模拟云朵轮廓
 * - 箭头为垂直线 + 顶部两侧斜线（V 形箭头）
 */
QPixmap IconHelper::cloudUpload(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x();
        qreal cy = r.center().y() + r.height() * 0.1;
        qreal w = r.width() * 0.7;
        qreal h = r.height() * 0.35;
        p->drawEllipse(QPointF(cx - w * 0.2, cy - h * 0.3), w * 0.35, h * 0.6);
        p->drawEllipse(QPointF(cx + w * 0.15, cy - h * 0.25), w * 0.3, h * 0.55);
        p->drawEllipse(QPointF(cx, cy + h * 0.1), w * 0.4, h * 0.5);
        p->drawLine(cx, cy - h * 0.3, cx, cy - h * 1.1);
        p->drawLine(cx, cy - h * 1.1, cx - w * 0.15, cy - h * 0.85);
        p->drawLine(cx, cy - h * 1.1, cx + w * 0.15, cy - h * 0.85);
    });
}

/**
 * @brief 绘制刷新/循环箭头图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：开口圆弧 + 三角形箭头
 * - 圆弧：从 30° 开始，逆时针 300°，形成接近闭合的环（顶部开口）
 * - 箭头三角形位于圆弧末端右上方，指向圆弧切线方向
 */
QPixmap IconHelper::refresh(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x();
        qreal cy = r.center().y();
        qreal rad = r.width() * 0.32;
        p->drawArc(QRectF(cx - rad, cy - rad, rad * 2, rad * 2), 30 * 16, 300 * 16);
        QPolygonF arrow;
        arrow << QPointF(cx + rad * 0.7, cy - rad * 0.6);
        arrow << QPointF(cx + rad * 1.0, cy - rad * 0.9);
        arrow << QPointF(cx + rad * 0.5, cy - rad * 1.0);
        p->drawPolygon(arrow);
    });
}

/**
 * @brief 绘制文件/文档图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：带折角的文档轮廓
 * - 主体为五边形：左上 → 右上（缩进 0.65×width） → 右下 → 左下 → 闭合
 * - 折角线：从顶部 0.65×width 处垂直向下到 0.35×height，再水平向右到右边
 */
QPixmap IconHelper::file(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal m = r.width() * 0.1;
        QRectF b(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
        QPolygonF poly;
        poly << QPointF(b.left(), b.top());
        poly << QPointF(b.left() + b.width() * 0.65, b.top());
        poly << QPointF(b.right(), b.top() + b.height() * 0.35);
        poly << QPointF(b.right(), b.bottom());
        poly << QPointF(b.left(), b.bottom());
        p->drawPolygon(poly);
        p->drawLine(b.left() + b.width() * 0.65, b.top(), b.left() + b.width() * 0.65, b.top() + b.height() * 0.35);
        p->drawLine(b.left() + b.width() * 0.65, b.top() + b.height() * 0.35, b.right(), b.top() + b.height() * 0.35);
    });
}

/**
 * @brief 绘制更多/三点菜单图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：水平排列的三个等距圆点
 * - 圆点半径为宽度的 0.1 倍
 * - 中心间距为宽度的 0.22 倍
 */
QPixmap IconHelper::more(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal rad = r.width() * 0.1;
        for (int i = -1; i <= 1; ++i) {
            p->drawEllipse(QPointF(r.center().x() + i * r.width() * 0.22, r.center().y()), rad, rad);
        }
    });
}

/**
 * @brief 绘制发送/纸飞机图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：三角箭头 + 底部横线
 * - 上折线：从右侧 0.1×width 处指向中心左侧 0.15×width，再折向右下 0.1×width
 * - 底部横线：从左侧 0.15×width 水平延伸到右侧 0.35×width（与折线中点对齐）
 */
QPixmap IconHelper::send(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        QPolygonF poly;
        poly << QPointF(r.right() - r.width() * 0.1, r.top() + r.height() * 0.15);
        poly << QPointF(r.left() + r.width() * 0.15, r.center().y());
        poly << QPointF(r.right() - r.width() * 0.1, r.bottom() - r.height() * 0.15);
        p->drawPolyline(poly);
        p->drawLine(r.left() + r.width() * 0.15, r.center().y(), r.right() - r.width() * 0.35, r.center().y());
    });
}

/**
 * @brief 绘制停止/方块图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：实心矩形边框
 * - 矩形边距为宽度的 0.25 倍，居中放置
 */
QPixmap IconHelper::stop(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal m = r.width() * 0.25;
        p->drawRect(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
    });
}

/**
 * @brief 绘制附件/回形针图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：长竖线 + 上下两个半圆弧
 * - 竖线从顶部 0.15×height 延伸到中心下方
 * - 上圆弧：中心偏下，绘制 0°~180°（上半圆）
 * - 下圆弧：中心更偏下，绘制 180°~360°（下半圆）
 * - 整体形成回形针/曲别针形状
 */
QPixmap IconHelper::attach(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal cx = r.center().x();
        qreal cy = r.center().y() + r.height() * 0.08;
        qreal w = r.width() * 0.15;
        qreal h = r.height() * 0.35;
        p->drawLine(cx, r.top() + r.height() * 0.15, cx, cy + h);
        p->drawArc(QRectF(cx - w, cy - h, w * 2, h * 2), 0, 180 * 16);
        p->drawArc(QRectF(cx - w, cy, w * 2, h * 2), 180 * 16, 180 * 16);
    });
}

/**
 * @brief 绘制代码/尖括号图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：左尖括号 + 右尖括号 + 中间斜线
 * - 左侧 "<"：两点折线，从 0.3×width 指向 0.1×width 中心再折回 0.3×width
 * - 右侧 ">"：镜像对称
 * - 中间斜线：从左上到右下，宽度 0.16×width，表示代码分隔符
 */
QPixmap IconHelper::code(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        p->drawLine(r.left() + r.width() * 0.3, r.top() + r.height() * 0.25,
                    r.left() + r.width() * 0.1, r.center().y());
        p->drawLine(r.left() + r.width() * 0.1, r.center().y(),
                    r.left() + r.width() * 0.3, r.bottom() - r.height() * 0.25);
        p->drawLine(r.right() - r.width() * 0.3, r.top() + r.height() * 0.25,
                    r.right() - r.width() * 0.1, r.center().y());
        p->drawLine(r.right() - r.width() * 0.1, r.center().y(),
                    r.right() - r.width() * 0.3, r.bottom() - r.height() * 0.25);
        p->drawLine(r.center().x() - r.width() * 0.08, r.top() + r.height() * 0.15,
                    r.center().x() + r.width() * 0.08, r.bottom() - r.height() * 0.15);
    });
}

/**
 * @brief 绘制加号/添加图标
 * @param size 图标尺寸
 * @param color 线条颜色
 * @return 渲染完成的 QPixmap
 *
 * @implementation
 * 几何构成：垂直线 + 水平线十字交叉
 * - 垂直线：从顶部 0.2×height 到底部 0.2×height，位于中心 x
 * - 水平线：从左侧 0.2×width 到右侧 0.2×width，位于中心 y
 */
QPixmap IconHelper::plus(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        p->drawLine(r.center().x(), r.top() + r.height() * 0.2, r.center().x(), r.bottom() - r.height() * 0.2);
        p->drawLine(r.left() + r.width() * 0.2, r.center().y(), r.right() - r.width() * 0.2, r.center().y());
    });
}

QPixmap IconHelper::minus(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        p->drawLine(r.left() + r.width() * 0.2, r.center().y(), r.right() - r.width() * 0.2, r.center().y());
    });
}

QPixmap IconHelper::square(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal m = r.width() * 0.2;
        p->drawRect(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
    });
}

QPixmap IconHelper::copy(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal w = r.width();
        qreal h = r.height();
        qreal m = w * 0.15;
        QRectF front(r.left() + m, r.top() + h * 0.25, w - m * 2.5, h - m * 2.5);
        QRectF back(r.left() + w * 0.25, r.top() + m, w - m * 2.5, h - m * 2.5);
        p->drawRect(back);
        p->drawRect(front);
    });
}

QPixmap IconHelper::xmark(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        p->drawLine(r.left() + r.width() * 0.25, r.top() + r.height() * 0.25,
                    r.right() - r.width() * 0.25, r.bottom() - r.height() * 0.25);
        p->drawLine(r.right() - r.width() * 0.25, r.top() + r.height() * 0.25,
                    r.left() + r.width() * 0.25, r.bottom() - r.height() * 0.25);
    });
}
