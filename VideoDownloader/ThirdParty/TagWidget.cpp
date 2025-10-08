#include "TagWidget.h"
#include <QPainter>
#include <QFontMetrics>

TagWidget::TagWidget(const QString& text, qreal fontSize, QColor bgColor, QWidget* parent,
	bool hasIcon, const QString& svgPath, int iconSize)
	: QWidget(parent),
	m_text(text),
	m_bgColor(bgColor),
	m_textColor(DesignSystem::instance()->textColor()),
	m_iconSize(iconSize),
	m_fontSize(fontSize),
	m_hasIcon(hasIcon)
{
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setAttribute(Qt::WA_TranslucentBackground);

	// 只有在需要图标 + 路径非空时才加载 SVG
	if (m_hasIcon && !svgPath.trimmed().isEmpty())
	{
		m_svgRenderer = new QSvgRenderer(this);
		if (!m_svgRenderer->load(svgPath))
		{
			qWarning() << "SVG 加载失败:" << svgPath;
		}
	}
	else if (m_hasIcon)
	{
		qWarning() << "启用了图标，但未提供 SVG 路径。";
	}

	// 设置字体大小
	QFont f = font();
	f.setPointSizeF(m_fontSize);
	setFont(f);
}

QSize TagWidget::sizeHint() const
{
	QFontMetrics fm(font());
	int textWidth = fm.horizontalAdvance(m_text);
	int height = qMax(m_iconSize, fm.height()) + 4; // 上下留白

	int width = 8 + textWidth + 8; // 左右 padding + 文字

	if (m_hasIcon)
		width += m_iconSize + 6; // icon + spacing

	return QSize(width, height);
}

void TagWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制圆角矩形背景
	QRect bgRect = rect();
	painter.setBrush(m_bgColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(bgRect, 4, 4);

	int textX = 8;
	if (m_hasIcon)
	{
		int iconY = (height() - m_iconSize) / 2 + 2;
		QRect iconRect(8, iconY, m_iconSize, m_iconSize);
		m_svgRenderer->render(&painter, iconRect);
		textX = iconRect.right() + 6;
	}

	// 绘制文字
	painter.setPen(m_textColor);
	QFontMetrics fm(font());
	int textY = (height() + fm.ascent() - fm.descent()) / 2;
	painter.drawText(textX, textY, m_text);
}