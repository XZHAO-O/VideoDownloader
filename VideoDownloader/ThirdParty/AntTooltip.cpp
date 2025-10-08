#include "AntTooltip.h"
#include "QVBoxLayout"
#include <QLabel>
#include <QLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <QPainter>
#include "DesignSystem.h"

AntTooltip::AntTooltip(QString text, ArrowDir dir, QWidget* parent)
	: QWidget(parent), m_arrowDirection(dir), m_text(text)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

	QFont font;
	font.setPointSize(11);
	m_font = font;
	QFontMetrics metrics(m_font);

	const int maxTextWidth = 300;  // 最大文本宽度限制
	QRect textBounding = metrics.boundingRect(0, 0, maxTextWidth, 1000, Qt::TextWordWrap, m_text);

	// 加额外 padding 使整体更美观
	const int Padding = 16;
	int paddedTextWidth = textBounding.width() + Padding * 2;
	int paddedTextHeight = textBounding.height() + Padding * 2;

	// 计算总宽高，考虑箭头方向额外宽度或高度
	int tiptoolWidth = paddedTextWidth + (m_arrowDirection == ArrowLeft || m_arrowDirection == ArrowRight ? arrowWidth : 0);
	int tiptoolHeight = paddedTextHeight + (m_arrowDirection == ArrowTop || m_arrowDirection == ArrowBottom ? arrowHeight : 0);

	// 设置控件尺寸
	resize(tiptoolWidth, tiptoolHeight);
}

AntTooltip::~AntTooltip()
{
}

QPoint AntTooltip::arrowTipOffset() const
{
	switch (m_arrowDirection) {
	case ArrowLeft:
		return QPoint(0, height() / 2);
	case ArrowRight:
		return QPoint(width(), height() / 2);
	case ArrowTop:
		return QPoint(width() / 2, 0);
	case ArrowBottom:
		return QPoint(width() / 2, height());
	default:
		return QPoint(width() / 2, height() / 2);
	}
}

void AntTooltip::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(Qt::NoPen);

	const int radius = 6;
	QRect rectBubble;  // 气泡矩形区域（不包括箭头）

	QPainterPath path;

	// 判断三角形的宽高
	if (m_arrowDirection == ArrowDir::ArrowLeft || m_arrowDirection == ArrowDir::ArrowRight)
	{
		arrowHeight = 14;
		arrowWidth = 7;
	}
	else
	{
		arrowHeight = 7;
		arrowWidth = 14;
	}

	switch (m_arrowDirection) {
	case ArrowDir::ArrowLeft: {
		rectBubble = QRect(arrowWidth, 0, width() - arrowWidth, height());
		QRectF r = rectBubble.adjusted(margin, margin, -margin, -margin);

		int ax = r.left();
		int ay = (height() - arrowHeight) / 2;
		int ah = arrowHeight;

		path.moveTo(ax, ay);
		path.lineTo(ax - arrowWidth, ay + ah / 2);
		path.lineTo(ax, ay + ah);

		// 左下圆角
		path.lineTo(ax, r.bottom() - radius);
		path.quadTo(r.left(), r.bottom(), r.left() + radius, r.bottom());

		// 右下圆角
		path.lineTo(r.right() - radius, r.bottom());
		path.quadTo(r.right(), r.bottom(), r.right(), r.bottom() - radius);

		// 右上圆角
		path.lineTo(r.right(), r.top() + radius);
		path.quadTo(r.right(), r.top(), r.right() - radius, r.top());

		// 左上圆角
		path.lineTo(r.left() + radius, r.top());
		path.quadTo(r.left(), r.top(), r.left(), r.top() + radius);

		path.lineTo(ax, ay);
		break;
	}
	case ArrowDir::ArrowRight: {
		rectBubble = QRect(0, 0, width() - arrowWidth, height());
		QRectF r = rectBubble.adjusted(margin, margin, -margin, -margin);

		int ax = r.right();
		int ay = (rectBubble.height() - arrowHeight) / 2;
		int ah = arrowHeight;

		path.moveTo(ax, ay);
		path.lineTo(ax + arrowWidth, ay + ah / 2);
		path.lineTo(ax, ay + ah);

		// 右下圆角
		path.lineTo(ax, r.bottom() - radius);
		path.quadTo(r.right(), r.bottom(), r.right() - radius, r.bottom());

		// 左下圆角
		path.lineTo(r.left() + radius, r.bottom());
		path.quadTo(r.left(), r.bottom(), r.left(), r.bottom() - radius);

		// 左上圆角
		path.lineTo(r.left(), r.top() + radius);
		path.quadTo(r.left(), r.top(), r.left() + radius, r.top());

		// 右上圆角
		path.lineTo(r.right() - radius, r.top());
		path.quadTo(r.right(), r.top(), r.right(), r.top() + radius);

		path.lineTo(ax, ay);
		break;
	}
	case ArrowDir::ArrowTop: {
		rectBubble = QRect(0, arrowHeight, width(), height() - arrowHeight);
		QRectF r = rectBubble.adjusted(margin, margin, -margin, -margin);

		int ax = (r.width() / 2);
		int ay = r.top();

		path.moveTo(ax, ay);
		path.lineTo(ax + arrowWidth / 2, ay - arrowHeight);
		path.lineTo(ax + arrowWidth, ay);

		// 右上圆角
		path.lineTo(r.right() - radius, ay);
		path.quadTo(r.right(), ay, r.right(), ay + radius);

		// 右下圆角
		path.lineTo(r.right(), r.bottom() - radius);
		path.quadTo(r.right(), r.bottom(), r.right() - radius, r.bottom());

		// 左下圆角
		path.lineTo(r.left() + radius, r.bottom());
		path.quadTo(r.left(), r.bottom(), r.left(), r.bottom() - radius);

		// 左上圆角
		path.lineTo(r.left(), ay + radius);
		path.quadTo(r.left(), ay, r.left() + radius, ay);

		path.lineTo(ax, ay);
		break;
	}
	case ArrowDir::ArrowBottom: {
		rectBubble = QRect(0, 0, width(), height() - arrowHeight);
		QRectF r = rectBubble.adjusted(margin, margin, -margin, -margin);

		int ax = (r.width() / 2);
		int ay = r.bottom();

		path.moveTo(ax, ay);
		path.lineTo(ax + arrowWidth / 2, ay + arrowHeight);
		path.lineTo(ax + arrowWidth, ay);

		// 右下圆角
		path.lineTo(r.right() - radius, ay);
		path.quadTo(r.right(), ay, r.right(), ay - radius);

		// 右上圆角
		path.lineTo(r.right(), r.top() + radius);
		path.quadTo(r.right(), r.top(), r.right() - radius, r.top());

		// 左上圆角
		path.lineTo(r.left() + radius, r.top());
		path.quadTo(r.left(), r.top(), r.left(), r.top() + radius);

		// 左下圆角
		path.lineTo(r.left(), ay - radius);
		path.quadTo(r.left(), ay, r.left() + radius, ay);

		path.lineTo(ax, ay);
		break;
	}
	}

	// 填充气泡主体
	p.setBrush(DesignSystem::instance()->currentTheme().toolTipBgColor);
	p.drawPath(path);

	// 设置字体与颜色
	p.setFont(m_font);
	p.setPen(DesignSystem::instance()->currentTheme().toolTipTextColor);  // 深灰色文本，更柔和

	// 计算文本区域（在 rectBubble 内部减去 margin）
	int padding = 8;  // 内边距
	QRect textRect = rectBubble.adjusted(margin + padding, margin + padding, -margin - padding, -margin - padding);
	// 绘制文本，不支持自动换行
	p.drawText(textRect, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignVCenter, m_text);
}