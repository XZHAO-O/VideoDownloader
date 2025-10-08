#pragma once

#include <QWidget>

class TransparentMask : public QWidget
{
	Q_OBJECT
public:
	explicit TransparentMask(QWidget* parent);

signals:
	void clickedOutside(); // 点击遮罩区域发出信号

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void paintEvent(QPaintEvent* event);
};
