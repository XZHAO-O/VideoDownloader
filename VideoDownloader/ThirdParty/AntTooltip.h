#pragma once

#include <QWidget>

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
	int arrowHeight = 0;			// 箭头高度设为0，表示没有箭头
	int arrowWidth = 0;				// 箭头宽度设为0，表示没有箭头

	ArrowDir m_arrowDirection = ArrowDir::None;
};