#pragma once

#include <QPushButton>
#include <QEnterEvent>
#include <QParallelAnimationGroup>
#include <QSvgRenderer>

#include "Ripple.h"

class AntButton : public QPushButton
{
	Q_OBJECT

public:
	// 按钮模式枚举
	enum ButtonMode
	{
		Filled = 0,      // 填充模式（默认）
		Outlined         // 描边模式
	};

	AntButton(QString btnText, qreal textSize, QWidget* parent);
	~AntButton();

	void setSvgIcon(const QString& iconPath);

	// 设置按钮模式
	void setButtonMode(ButtonMode mode);
	ButtonMode buttonMode() const { return m_buttonMode; }

	// 设置描边宽度
	void setStrokeWidth(int width);
	int strokeWidth() const { return m_strokeWidth; }

	// 设置按钮颜色（如果不设置，则使用主题色）
	void setButtonColor(const QColor& color);
	QColor buttonColor() const { return m_buttonColor; }

	// 设置文字颜色（如果不设置，则根据模式自动选择）
	void setTextColor(const QColor& color);
	QColor textColor() const { return m_textColor; }

protected:
	// 重写绘制逻辑
	void paintEvent(QPaintEvent* event) override;

	// 鼠标事件
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	// 动画属性
	int animTime = 500;
	int m_radius;
	int m_margin;

	// 按钮模式
	ButtonMode m_buttonMode = Filled;

	// 描边相关属性
	int m_strokeWidth = 1;

	// 自定义颜色（如果为无效颜色，则使用主题色）
	QColor m_buttonColor;
	QColor m_textColor;

	// 按钮当前状态
	bool m_hovered;
	bool m_pressed;

	// 图标
	QSvgRenderer* m_svgRenderer = nullptr;
	qreal m_scaleFactor = 0.65;

	// 防止短时间内多次点击
	QElapsedTimer m_clickTimer;
	int m_clickIntervalMs = 100;

	// 存储多个波纹
	QList<Ripple*> m_ripples;
	Ripple* m_ripp = nullptr;
};