#include "MaskWidget.h"
#include <QMouseEvent>

MaskWidget::MaskWidget(int w, int h, QWidget* parent)
	: QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	resize(w, h);
	alphaAnim = new QPropertyAnimation(this, "m_alpha");
	alphaAnim->setStartValue(0.0);
	alphaAnim->setEndValue(0.4);
	alphaAnim->setDuration(300);
	alphaAnim->setEasingCurve(QEasingCurve::OutCubic);
	hide();

	connect(alphaAnim, &QPropertyAnimation::finished, this, [this]()
		{
			if (isHide)
			{
				hide();
			}
		});
}

MaskWidget::~MaskWidget()
{
}

void MaskWidget::showAnim()
{
	isHide = false;
	show();
	alphaAnim->setDirection(QAbstractAnimation::Forward);
	alphaAnim->start();
}

void MaskWidget::hideAnim()
{
	isHide = true;
	alphaAnim->setDirection(QAbstractAnimation::Backward);
	alphaAnim->start();
}

void MaskWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		emit clicked();
	}
	e->accept();  // 阻止事件继续传递
}

void MaskWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(Qt::NoPen);

	QColor color(0, 0, 0);
	color.setAlphaF(m_alpha);
	p.setBrush(color);

	p.drawRect(rect());
}