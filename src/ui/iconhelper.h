/**
 * @file iconhelper.h
 * @brief 图标辅助类头文件
 *
 * 提供基于QPainter的SVG风格图标绘制功能，包含常用图标（logo、message、chip、layers、moon、sun、
 * search、check、cloudUpload、refresh、file、more、send、stop、attach、code、plus等）的静态绘制方法。
 * 每个方法接收尺寸和颜色参数，返回QPixmap对象，便于在Qt界面中直接使用。
 */

#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QPixmap>
#include <QColor>

/**
 * @brief 图标辅助类
 *
 * 使用QPainter绘制SVG风格图标的工具类，所有图标方法均为静态，无需实例化即可调用。
 * 内部通过render方法统一处理抗锯齿、设备像素比等绘制细节。
 */
class IconHelper
{
public:
    /**
     * @brief 绘制Logo图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap logo(int size, const QColor& color);

    /**
     * @brief 绘制消息/对话图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap message(int size, const QColor& color);

    /**
     * @brief 绘制芯片/AI图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap chip(int size, const QColor& color);

    /**
     * @brief 绘制图层图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap layers(int size, const QColor& color);

    /**
     * @brief 绘制月亮/暗色模式图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap moon(int size, const QColor& color);

    /**
     * @brief 绘制太阳/亮色模式图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap sun(int size, const QColor& color);

    /**
     * @brief 绘制搜索图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap search(int size, const QColor& color);

    /**
     * @brief 绘制勾选图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap check(int size, const QColor& color);

    /**
     * @brief 绘制云端上传图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap cloudUpload(int size, const QColor& color);

    /**
     * @brief 绘制刷新图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap refresh(int size, const QColor& color);

    /**
     * @brief 绘制文件图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap file(int size, const QColor& color);

    /**
     * @brief 绘制更多选项图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap more(int size, const QColor& color);

    /**
     * @brief 绘制发送图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap send(int size, const QColor& color);

    /**
     * @brief 绘制停止图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap stop(int size, const QColor& color);

    /**
     * @brief 绘制附件图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap attach(int size, const QColor& color);

    /**
     * @brief 绘制代码图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap code(int size, const QColor& color);

    /**
     * @brief 绘制加号/添加图标
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @return 绘制完成的QPixmap
     */
    static QPixmap plus(int size, const QColor& color);

private:
    /**
     * @brief 通用图标渲染方法
     * @param size 图标尺寸（宽高相同）
     * @param color 图标颜色
     * @param draw 绘制回调函数，接收QPainter和绘制区域参数
     * @return 绘制完成的QPixmap
     */
    static QPixmap render(int size, const QColor& color, void (*draw)(QPainter*, const QRect&));
};

#endif
