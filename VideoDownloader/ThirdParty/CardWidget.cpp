#include "CardWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QEasingCurve>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

CardWidget::CardWidget(QString lastLogin, QString totalTime, QWidget* parent)
	: QWidget(parent),
	m_lastLogin(lastLogin),
	m_totalTime(totalTime)
{
	m_scaleAnimation = new QVariantAnimation(this);
	m_scaleAnimation->setDuration(200);
	m_scaleAnimation->setStartValue(1.0);
	m_scaleAnimation->setEndValue(1.2);
	m_scaleAnimation->setEasingCurve(QEasingCurve::OutCubic);

	connect(m_scaleAnimation, &QVariantAnimation::valueChanged, this, [=](const QVariant& value)
		{
			m_scaleFactor = value.toReal();
			update();
		});

	// 创建阴影效果
	auto* shadow = new QGraphicsDropShadowEffect(this);
	shadow->setBlurRadius(15);
	shadow->setOffset(0, 4);
	shadow->setColor(QColor(50, 50, 50, 150));
	setGraphicsEffect(shadow);
}

void CardWidget::setImageFile(const QString& path)
{
	m_pixmap = QPixmap(path);
	update();
}

void CardWidget::enterEvent(QEnterEvent* event)
{
	Q_UNUSED(event);
	m_scaleAnimation->setDirection(QAbstractAnimation::Forward);
	m_scaleAnimation->start();
}

void CardWidget::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	m_scaleAnimation->setDirection(QAbstractAnimation::Backward);
	m_scaleAnimation->start();
}

void CardWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::LosslessImageRendering);

	QRectF rect = this->rect().adjusted(4, 4, -4, -4);

	QPainterPath clipPath;
	clipPath.addRoundedRect(rect, 8, 8);
	painter.setClipPath(clipPath);

	// 缩放绘制图片部分（保持动画效果）
	painter.save();
	QPointF center = rect.center();
	painter.translate(center);
	painter.scale(m_scaleFactor, m_scaleFactor);
	painter.translate(-center);

	if (!m_pixmap.isNull())
	{
		painter.drawPixmap(rect.toRect(), m_pixmap);
	}
	painter.restore();

	// 动态计算遮罩高度（27%控件高）
	const qreal maskHeightRatio = 0.27;
	const int maskHeight = static_cast<int>(rect.height() * maskHeightRatio);
	QRectF maskRect(rect.left(), rect.bottom() - maskHeight, rect.width(), maskHeight);

	QLinearGradient gradient(maskRect.topLeft(), maskRect.bottomLeft());
	gradient.setColorAt(0.0, QColor(0, 0, 0, 30));
	gradient.setColorAt(1.0, QColor(0, 0, 0, 160));

	painter.setPen(Qt::NoPen);
	painter.setBrush(gradient);
	painter.drawRect(maskRect);

	// 文字绘制
	painter.setPen(Qt::white);
	QFont font = painter.font();
	font.setPointSize(10);
	painter.setFont(font);

	painter.drawText(maskRect.adjusted(8, 4, -8, -maskHeight / 2), Qt::AlignLeft | Qt::AlignVCenter, m_lastLogin);
	painter.drawText(maskRect.adjusted(8, maskHeight / 2 - 4, -8, -5), Qt::AlignLeft | Qt::AlignVCenter, m_totalTime);
}