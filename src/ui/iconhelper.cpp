#include "iconhelper.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>

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

QPixmap IconHelper::more(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal rad = r.width() * 0.1;
        for (int i = -1; i <= 1; ++i) {
            p->drawEllipse(QPointF(r.center().x() + i * r.width() * 0.22, r.center().y()), rad, rad);
        }
    });
}

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

QPixmap IconHelper::stop(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        qreal m = r.width() * 0.25;
        p->drawRect(r.left() + m, r.top() + m, r.width() - m * 2, r.height() - m * 2);
    });
}

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

QPixmap IconHelper::plus(int size, const QColor& color)
{
    return render(size, color, [](QPainter* p, const QRect& r) {
        p->drawLine(r.center().x(), r.top() + r.height() * 0.2, r.center().x(), r.bottom() - r.height() * 0.2);
        p->drawLine(r.left() + r.width() * 0.2, r.center().y(), r.right() - r.width() * 0.2, r.center().y());
    });
}
