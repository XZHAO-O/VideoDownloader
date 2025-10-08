#include "TransparentMask.h"
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QPainter>

TransparentMask::TransparentMask(QWidget* parent)
	: QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setObjectName("ComboBoxMask");

	// 设置为全屏遮罩
	if (parent)
	{
		resize(parent->width(), parent->height());
	}
	hide();
}

void TransparentMask::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
	emit clickedOutside();  // 通知外部关闭弹窗等操作
	event->accept();		// 阻止事件继续传递
}

void TransparentMask::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	// 调试用给个背景颜色 方便查看遮罩是否覆盖
	//QPainter painter(this);
	//painter.setRenderHint(QPainter::Antialiasing, true);

	//// 绘制一个半透明灰色背景
	//QColor maskColor(0, 0, 200, 80);
	//painter.fillRect(rect(), maskColor);
}