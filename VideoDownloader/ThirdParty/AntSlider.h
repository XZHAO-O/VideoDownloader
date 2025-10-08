// AntSlider.h
#pragma once

#include <QSlider>
#include <QPropertyAnimation>
#include <QEvent>
#include <QGraphicsOpacityEffect>

class DownArrowTooltip : public QWidget
{
	Q_OBJECT
public:
	explicit DownArrowTooltip(QWidget* parent = nullptr);
	void setText(const QString& text);
	void showAt(const QPoint& globalPos);
	void fadeIn();
	void fadeOut();
private:
	QString m_text;
	QFont m_font;
	int arrowHeight = 7;
	int arrowWidth = 14;
	int padding = 10;
	int fixedHeight = 36;
	QPropertyAnimation* m_opacityAnimation = nullptr;
	bool isHide = false;

protected:
	void paintEvent(QPaintEvent* event) override;
};

class AntSlider : public QSlider
{
	Q_OBJECT
		Q_PROPERTY(qreal handleRadius READ handleRadius WRITE setHandleRadius)

public:

	explicit AntSlider(int min = 0, int max = 100, int value = 50, QWidget* parent = nullptr);

	qreal handleRadius() const { return m_handleRadius; }
	void setHandleRadius(qreal radius);

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	QSize sizeHint() const;
	void mousePressEvent(QMouseEvent* event);
signals:
	void handleMoved(const QPoint& centerInWidget);
private:
	qreal m_handleRadius = 7;
	QPropertyAnimation* m_radiusAnimation = nullptr;
	bool m_hovered = false;
	// tooltip
	DownArrowTooltip* m_tooltip = nullptr;
	// ---- 几何配置----
	const int m_ringWidth = 3;
	const int m_maxRadius = 8;
	int       m_margin = m_ringWidth + m_maxRadius;   // 左右空白

	// 统一的轨道矩形
	QRect trackRect() const
	{
		const int th = 6;
		const int ty = height() / 2 - th / 2;
		return { m_margin, ty, width() - 2 * m_margin, th };
	}
};
