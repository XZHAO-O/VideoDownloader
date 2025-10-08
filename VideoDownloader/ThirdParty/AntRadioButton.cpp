// antradiobutton.cpp
#include "AntRadioButton.h"
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyle>
#include <QPainterPath>

AntRadioButton::AntRadioButton(QWidget* parent)
	: QRadioButton(parent)
{
	m_animation = new QPropertyAnimation(this, "innerRatio", this);
	m_animation->setDuration(350);
	m_animation->setEasingCurve(QEasingCurve::OutCubic);

	connect(this, &QRadioButton::toggled, this, &AntRadioButton::onToggled);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_borderColorDefault = DesignSystem::instance()->borderColor();
			m_borderColorHover = DesignSystem::instance()->borderColorHover();
			m_fillColor = DesignSystem::instance()->primaryColor();
			update();
		});
}

void AntRadioButton::setInnerRatio(qreal ratio)
{
	if (!qFuzzyCompare(m_innerRatio, ratio)) {
		m_innerRatio = ratio;
		update();
	}
}

void AntRadioButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	int circleRadius = m_circleDiameter / 2;
	int circleY = (height() - m_circleDiameter) / 2;

	QPointF center(m_marginLeft + circleRadius, circleY + circleRadius);

	// 根据悬浮状态选边框颜色
	QColor borderColor = isEnabled()
		? (m_hovered ? m_borderColorHover : m_borderColorDefault)
		: DesignSystem::instance()->disabledColor();

	painter.setPen(QPen(borderColor, 1.5));
	painter.setBrush(Qt::NoBrush);
	QRectF outerCircle(center.x() - circleRadius, center.y() - circleRadius,
		m_circleDiameter, m_circleDiameter);
	painter.drawEllipse(outerCircle);

	if (isChecked()) {
		// 填充颜色用主题色
		QPainterPath outerPath;
		outerPath.addEllipse(outerCircle);

		qreal innerRadius = circleRadius * m_innerRatio;
		QRectF innerCircle(center.x() - innerRadius, center.y() - innerRadius,
			innerRadius * 2, innerRadius * 2);

		QPainterPath innerPath;
		innerPath.addEllipse(innerCircle);

		QPainterPath ringPath = outerPath.subtracted(innerPath);

		painter.setPen(Qt::NoPen);
		painter.setBrush(m_fillColor);
		painter.drawPath(ringPath);
	}

	// 文字颜色保持默认黑色
	painter.setPen(palette().color(QPalette::WindowText));

	int textX = m_marginLeft + m_circleDiameter + m_spacing;
	QRect textRect(textX, 0, width() - textX, height());

	painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
}

void AntRadioButton::onToggled(bool checked)
{
	m_animation->stop();
	if (checked) {
		m_animation->setStartValue(m_innerRatio);
		m_animation->setEndValue(0.5);  // 最大内圆半径比例
	}
	else {
		m_animation->setStartValue(m_innerRatio);
		m_animation->setEndValue(0.0);
	}
	m_animation->start();
}

QSize AntRadioButton::sizeHint() const
{
	QFontMetrics fm(font());

	int rightMargin = 2;

	int textWidth = fm.horizontalAdvance(text());
	int textHeight = fm.height();

	int height = qMax(m_circleDiameter, textHeight) + 4;
	int width = m_marginLeft + m_circleDiameter + m_spacing + textWidth + rightMargin;

	return QSize(width, height);
}

void AntRadioButton::enterEvent(QEnterEvent* event)
{
	m_hovered = true;
	update(); // 触发重绘
	QRadioButton::enterEvent(event);
}

void AntRadioButton::leaveEvent(QEvent* event)
{
	m_hovered = false;
	update();
	QRadioButton::leaveEvent(event);
}