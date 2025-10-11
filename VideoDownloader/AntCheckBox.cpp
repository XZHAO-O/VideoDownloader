#include "AntCheckBox.h"
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyle>
#include <QPainterPath>

AntCheckBox::AntCheckBox(QWidget* parent)
	: QCheckBox(parent)
{
	init();
}

AntCheckBox::AntCheckBox(const QString& text, QWidget* parent)
	: QCheckBox(text, parent)
{
	init();
}

void AntCheckBox::init()
{
	m_animation = new QPropertyAnimation(this, "innerRatio", this);
	m_animation->setDuration(200);
	m_animation->setEasingCurve(QEasingCurve::OutCubic);

	// 使用stateChanged信号而不是toggled，以支持三态
	connect(this, &QCheckBox::checkStateChanged, this, &AntCheckBox::onStateChanged);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_borderColorDefault = DesignSystem::instance()->borderColor();
			m_borderColorHover = DesignSystem::instance()->borderColorHover();
			m_fillColor = DesignSystem::instance()->primaryColor();
			update();
		});

	// 设置鼠标跟踪
	setMouseTracking(true);

	// 设置合适的固定大小
	setFixedSize(sizeHint());
}

void AntCheckBox::setInnerRatio(qreal ratio)
{
	if (!qFuzzyCompare(m_innerRatio, ratio)) {
		m_innerRatio = ratio;
		update();
	}
}

QRect AntCheckBox::getCheckBoxRect() const
{
	int squareY = (height() - m_squareSize) / 2;
	return QRect(m_marginLeft, squareY, m_squareSize, m_squareSize);
}

void AntCheckBox::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QRect checkBoxRect = getCheckBoxRect();
	QPointF center = checkBoxRect.center();

	// 根据状态选边框颜色
	QColor borderColor = m_borderColorDefault;
	if (!isEnabled()) {
		borderColor = DesignSystem::instance()->disabledColor();
	}
	else if (m_pressed) {
		borderColor = m_fillColor;
	}
	else if (m_hovered) {
		borderColor = m_borderColorHover;
	}

	// 绘制外部方形
	painter.setPen(QPen(borderColor, 1.5));
	painter.setBrush(Qt::NoBrush);
	painter.drawRoundedRect(checkBoxRect, 2, 2);

	if (checkState() != Qt::Unchecked) {
		// 计算内部填充区域
		qreal innerSize = m_squareSize * m_innerRatio;
		QRectF innerSquare(center.x() - innerSize / 2, center.y() - innerSize / 2,
			innerSize, innerSize);

		painter.setPen(Qt::NoPen);
		painter.setBrush(m_fillColor);
		painter.drawRoundedRect(innerSquare, 1, 1);

		// 绘制对勾或横线
		if (m_innerRatio > 0.4) {
			painter.setPen(QPen(Qt::white, 2));
			painter.setBrush(Qt::NoBrush);

			if (checkState() == Qt::Checked) {
				// 对勾的路径
				QPainterPath checkPath;
				checkPath.moveTo(center.x() - innerSize * 0.3, center.y() - innerSize * 0.1);
				checkPath.lineTo(center.x() - innerSize * 0.1, center.y() + innerSize * 0.2);
				checkPath.lineTo(center.x() + innerSize * 0.3, center.y() - innerSize * 0.2);
				painter.drawPath(checkPath);
			}
			else if (checkState() == Qt::PartiallyChecked) {
				// 横线表示部分选中
				painter.drawLine(center.x() - innerSize * 0.3, center.y(),
					center.x() + innerSize * 0.3, center.y());
			}
		}
	}

	// 绘制文字（如果有）
	if (!text().isEmpty()) {
		painter.setPen(palette().color(QPalette::WindowText));
		int textX = m_marginLeft + m_squareSize + m_spacing;
		QRect textRect(textX, 0, width() - textX, height());
		painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
	}
}

void AntCheckBox::onStateChanged(int state)
{
	Q_UNUSED(state);

	m_animation->stop();
	if (checkState() != Qt::Unchecked) {
		m_animation->setStartValue(m_innerRatio);
		m_animation->setEndValue(0.6);  // 最大内方形比例
	}
	else {
		m_animation->setStartValue(m_innerRatio);
		m_animation->setEndValue(0.0);
	}
	m_animation->start();
}

QSize AntCheckBox::sizeHint() const
{
	QFontMetrics fm(font());
	int rightMargin = 2;
	int textWidth = fm.horizontalAdvance(text());
	int textHeight = fm.height();
	int height = qMax(m_squareSize, textHeight) + 6;
	int width = m_marginLeft + m_squareSize + m_spacing + textWidth + rightMargin;

	// 确保有最小宽度，即使没有文本
	if (text().isEmpty()) {
		width = m_marginLeft + m_squareSize + rightMargin + 4;
	}

	return QSize(width, height);
}

void AntCheckBox::enterEvent(QEnterEvent* event)
{
	m_hovered = true;
	update();
	QCheckBox::enterEvent(event);
}

void AntCheckBox::leaveEvent(QEvent* event)
{
	m_hovered = false;
	m_pressed = false;
	update();
	QCheckBox::leaveEvent(event);
}

void AntCheckBox::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_pressed = true;
		update();

		// 调用基类处理点击
		QCheckBox::mousePressEvent(event);
	}
}

void AntCheckBox::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_pressed = false;
		update();
		QCheckBox::mouseReleaseEvent(event);
	}
}