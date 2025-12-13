#include "AntTooltip.h"

#include <QPainterPath>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include "DesignSystem.h"

AntTooltip::AntTooltip(QString text, ArrowDir dir, QWidget* parent)
	: QWidget(parent), m_arrowDirection(dir), m_text(text)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

	// 设置字体
	QFont font;
	font.setPointSize(9);
	m_font = font;

	// 重新计算尺寸
	updateSize();
}

AntTooltip::~AntTooltip()
{
}

void AntTooltip::updateSize()
{
	QTextDocument doc;
	doc.setDefaultFont(m_font);
	doc.setPlainText(m_text);
	doc.setDocumentMargin(0);  // 清除文档边距

	// 设置文本宽度
	doc.setTextWidth(-1);  // 先不限制宽度，计算理想宽度
	qreal idealWidth = doc.idealWidth();

	// 限制最大宽度为300像素
	const int maxTextWidth = 300;
	if (idealWidth > maxTextWidth) {
		doc.setTextWidth(maxTextWidth);
	}
	else {
		// 如果是单行文本，使用理想宽度
		doc.setTextWidth(idealWidth);
	}

	// 获取文档大小（这是准确的多行文本尺寸）
	QSizeF docSize = doc.documentLayout()->documentSize();

	// 计算内边距
	int padding = 8;  // 文本内边距
	int totalHorizontalPadding = (margin + padding) * 2;
	int totalVerticalPadding = (margin + padding) * 2;

	// 计算控件大小
	int width = qCeil(docSize.width()) + totalHorizontalPadding;
	int height = qCeil(docSize.height()) + totalVerticalPadding;

	// 最小尺寸
	const int minWidth = 80;
	const int minHeight = 40;
	if (width < minWidth) width = minWidth;
	if (height < minHeight) height = minHeight;

	// 设置控件尺寸
	resize(width, height);

	// 发出尺寸变化信号
	emit resized(width, height);
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

	// 计算文本区域（在 rectBubble 内部减去 margin 和 padding）
	int padding = 8;  // 内边距
	QRect textRect = rectBubble.adjusted(margin + padding, margin + padding,
		-margin - padding, -margin - padding);

	// 简单方法：使用QPainter的drawText，配合QTextOption实现自动换行
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::WordWrap);
	textOption.setAlignment(Qt::AlignCenter);  // 如果你想要文本居中

	// 绘制文本
	p.drawText(textRect, m_text, textOption);
}