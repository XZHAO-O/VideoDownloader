#include "AntTooltip.h"

#include <QPainterPath>
#include <QTextDocument>

#include "DesignSystem.h"

AntTooltip::AntTooltip(QString text, ArrowDir dir, QWidget* parent)
	: QWidget(parent), m_arrowDirection(dir), m_text(text)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

	// 设置更小的字体
	QFont font;
	font.setPointSize(9);  // 缩小字体大小
	m_font = font;
	QFontMetrics metrics(m_font);

	// 使用 QTextDocument 计算文本尺寸
	QTextDocument doc;
	doc.setDefaultFont(m_font);
	doc.setPlainText(m_text);

	// 设置文本宽度，-1表示不限制（计算单行宽度）
	doc.setTextWidth(-1);
	qreal idealWidth = doc.idealWidth();

	// 限制最大宽度为300像素
	const int maxTextWidth = 300;
	if (idealWidth > maxTextWidth) {
		doc.setTextWidth(maxTextWidth);
	}

	// 获取文档大小
	QSizeF docSize = doc.size();

	// 内边距
	const int Padding = 10;
	int paddedTextWidth = qCeil(docSize.width()) + Padding * 2;
	int paddedTextHeight = qCeil(docSize.height()) + Padding * 2;

	// 最小尺寸
	if (paddedTextWidth < 80) paddedTextWidth = 80;
	if (paddedTextHeight < 40) paddedTextHeight = 40;

	// 设置控件尺寸
	resize(paddedTextWidth, paddedTextHeight);
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

	const int radius = 4;  // 圆角半径
	QRect rectBubble = QRect(0, 0, width(), height());  // 整个控件都是气泡区域

	QPainterPath path;

	// 创建圆角矩形路径（没有箭头）
	QRectF r = rectBubble.adjusted(margin, margin, -margin, -margin);

	// 绘制圆角矩形
	path.moveTo(r.left() + radius, r.top());
	path.lineTo(r.right() - radius, r.top());
	path.quadTo(r.right(), r.top(), r.right(), r.top() + radius);
	path.lineTo(r.right(), r.bottom() - radius);
	path.quadTo(r.right(), r.bottom(), r.right() - radius, r.bottom());
	path.lineTo(r.left() + radius, r.bottom());
	path.quadTo(r.left(), r.bottom(), r.left(), r.bottom() - radius);
	path.lineTo(r.left(), r.top() + radius);
	path.quadTo(r.left(), r.top(), r.left() + radius, r.top());
	path.closeSubpath();

	// 填充气泡主体
	p.setBrush(DesignSystem::instance()->currentTheme().toolTipBgColor);
	p.drawPath(path);

	// 设置字体与颜色
	p.setFont(m_font);
	p.setPen(DesignSystem::instance()->currentTheme().toolTipTextColor);

	// 计算文本区域（在 rectBubble 内部减去 margin）
	int padding = 8;  // 内边距
	QRect textRect = rectBubble.adjusted(margin + padding, margin + padding, -margin - padding, -margin - padding);

	// 使用QTextOption确保文本居中
	QTextOption textOption;
	textOption.setAlignment(Qt::AlignCenter);
	textOption.setWrapMode(QTextOption::WordWrap);

	// 绘制文本，支持自动换行并居中
	p.drawText(textRect, m_text, textOption);
}