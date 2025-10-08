#include "Win11CheckButton.h"

Win11CheckButton::Win11CheckButton(QWidget* parent)
	: QCheckBox(parent), m_progress(0.0)
{
	m_anim = new QPropertyAnimation(this, "progress", this);
	m_anim->setDuration(300);
	m_anim->setEasingCurve(QEasingCurve::InOutCubic);

	connect(this, &QCheckBox::toggled, this, [this](bool checked) {
		m_anim->stop();
		if (checked) {
			m_anim->setStartValue(m_progress);
			m_anim->setEndValue(1.0);
		}
		else {
			m_anim->setStartValue(m_progress);
			m_anim->setEndValue(0.0);
		}
		m_anim->start();
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			themeColor = DesignSystem::instance()->primaryColor();
		});
}

Win11CheckButton::~Win11CheckButton()
{
}

void Win11CheckButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// 基础参数
	int w = width();
	int h = height();

	QFontMetrics fm(font());
	int textHeight = fm.height();
	int ascent = fm.ascent(); // 字体基线
	int boxMarginLeft = 4;

	// 计算 boxRect 位置：底部对齐 baseline
	int y = (h - textHeight) / 2 + (ascent - boxSize + 1); // +1 防止偏低
	QRect boxRect(boxMarginLeft, y, boxSize, boxSize);
	int radius = boxSize / 4;

	auto theme = DesignSystem::instance()->currentTheme();

	// 边框颜色
	QColor borderColor = isChecked() ? themeColor
		: (isEnabled() ? theme.checkBoxBorderEnableColor : theme.checkBoxBorderDisableColor);
	p.setPen(QPen(borderColor, 1.5));
	p.setBrush(isChecked() ? themeColor : theme.checkBoxBgColor);  // 修改为默认白色填充
	p.drawRoundedRect(boxRect, radius, radius);

	// 对勾动画绘制
	if (m_progress > 0.0) {
		QPen checkPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
		p.setPen(checkPen);
		p.setBrush(Qt::NoBrush);

		QPainterPath checkPath;
		QPointF start = boxRect.topLeft() + QPointF(boxSize * 0.25, boxSize * 0.55);
		QPointF mid = boxRect.topLeft() + QPointF(boxSize * 0.45, boxSize * 0.75);
		QPointF end = boxRect.topLeft() + QPointF(boxSize * 0.75, boxSize * 0.35);

		checkPath.moveTo(start);
		checkPath.lineTo(mid);
		checkPath.lineTo(end);

		qreal totalLen = approximatePathLength(checkPath);
		qreal drawLen = totalLen * m_progress;
		p.drawPath(strokePathPortion(checkPath, drawLen));
	}

	// 绘制文字
	p.setPen(isEnabled() ? theme.checkBoxTextColor : theme.checkBoxTextDisableColor);
	int textX = boxRect.right() + boxSpacing;
	QRect textRect(textX, -1, w - textX - 4, h);
	p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
}

QSize Win11CheckButton::sizeHint() const
{
	QFontMetrics fm(font());
	int textWidth = fm.horizontalAdvance(text());
	int height = qMax(boxSize, fm.height()) + 4;
	int width = boxMargin + boxSize + boxSpacing + textWidth + boxMargin;
	return QSize(width, height);
}

qreal Win11CheckButton::approximatePathLength(const QPainterPath& path)
{
	qreal length = 0;
	QPointF lastPoint;
	for (int i = 0; i < path.elementCount(); ++i) {
		QPainterPath::Element e = path.elementAt(i);
		QPointF pt(e.x, e.y);
		if (i > 0)
			length += QLineF(lastPoint, pt).length();
		lastPoint = pt;
	}
	return length;
}

QPainterPath Win11CheckButton::strokePathPortion(const QPainterPath& path, qreal length)
{
	QPainterPath result;
	qreal accumulatedLength = 0;
	QPointF lastPoint;

	for (int i = 0; i < path.elementCount(); ++i) {
		QPainterPath::Element e = path.elementAt(i);
		QPointF pt(e.x, e.y);
		if (i == 0) {
			result.moveTo(pt);
			lastPoint = pt;
			continue;
		}

		qreal segmentLength = QLineF(lastPoint, pt).length();
		if (accumulatedLength + segmentLength <= length) {
			result.lineTo(pt);
			accumulatedLength += segmentLength;
		}
		else {
			qreal remain = length - accumulatedLength;
			if (remain > 0) {
				QLineF line(lastPoint, pt);
				line.setLength(remain);
				result.lineTo(line.p2());
			}
			break;
		}
		lastPoint = pt;
	}
	return result;
}