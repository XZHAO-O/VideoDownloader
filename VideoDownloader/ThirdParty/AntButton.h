#pragma once

#include <QWidget>
#include <QPushButton>
#include <QEvent>
#include <QEnterEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QElapsedTimer>
#include <QSvgRenderer>
#include "Ripple.h"

class AntButton : public QPushButton
{
	Q_OBJECT
public:
	AntButton(QString btnText, qreal textSize, QWidget* parent);
	~AntButton();
	void setSvgIcon(const QString& iconPath);
protected:
	// 重写绘制逻辑：先绘制按钮本身，再绘制向外扩散的圆角矩形波纹
	void paintEvent(QPaintEvent* event) override;

	// 鼠标按下 / 释放 / 悬停 / 离开事件，用来启动/结束动画并改变按钮状态
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:

	// 用于动画的属性
	// 动画持续时间
	int animTime = 500;
	int m_radius;
	int m_margin;

	// 按钮当前状态
	bool m_hovered;
	bool m_pressed;

	// 图标
	QSvgRenderer* m_svgRenderer = nullptr;
	qreal m_scaleFactor = 0.65;	// 缩放系数

	// 按钮主色（可以在构造函数中或外部提供 setter 修改）
	QColor baseColor;

	// 防止短时间内多次点击
	QElapsedTimer m_clickTimer;
	int m_clickIntervalMs = 100; // 最小间隔，单位ms 跟着动画时间走

	// 存储多个波纹
	QList<Ripple*> m_ripples;
	Ripple* m_ripp = nullptr;
};
