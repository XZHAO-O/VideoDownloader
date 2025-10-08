#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QEvent>
#include <QParallelAnimationGroup>

class AntTooltip : public QWidget
{
	Q_OBJECT
public:
	enum ArrowDir
	{
		ArrowLeft,
		ArrowTop,
		ArrowRight,
		ArrowBottom,
		None
	};
	AntTooltip(QString text, ArrowDir dir, QWidget* parent);
	~AntTooltip();
	QPoint arrowTipOffset() const;
	void setText(QString text)
	{
		m_text = text;
		update();
	}
	ArrowDir getArrowDirection() const { return m_arrowDirection; }
protected:
	void paintEvent(QPaintEvent*);
signals:
	void resized(int width, int height);
public:
private:
	QString m_text;
	QFont m_font;
	int margin = 6;					// 气泡框(圆角矩形)的外边距
	int arrowHeight = 14;			// 箭头高度 箭头垂直方向
	int arrowWidth = 7;				// 箭头宽度 箭头水平方向

	ArrowDir m_arrowDirection = ArrowDir::None;
};
