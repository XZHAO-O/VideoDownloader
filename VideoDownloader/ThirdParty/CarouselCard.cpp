#include "CarouselCard.h"
#include <QColor>
#include <QPainterPath>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include "TagWidget.h"

CarouselCard::CarouselCard(const QString& imagePath, QWidget* parent)
	: QWidget(parent), m_imagePath(imagePath)
{
	m_pixmap = QPixmap(m_imagePath);
	setAttribute(Qt::WA_TranslucentBackground);

	// 为卡片添加阴影效果
	QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
	shadowEffect->setBlurRadius(20);
	shadowEffect->setOffset(5, 5);
	shadowEffect->setColor(QColor(0, 0, 0, 150));

	// 设置阴影效果
	this->setGraphicsEffect(shadowEffect);
}

CarouselCard::~CarouselCard()
{
}

void CarouselCard::addTab(QString text, QColor bgColor)
{
	// 左上角添加标签
	TagWidget* tag = new TagWidget(text, 9.5, bgColor, this);
	tag->move(20, 20);
}

void CarouselCard::enableUrlBtn(QUrl url)
{
	QSize size(130, 22);
	btn = new QToolButton(this);
	QIcon icon(":/Imgs/rightArrowBtn.svg");
	btn->setIcon(icon);
	btn->setIconSize(QSize(22, 22));
	btn->setFixedSize(size);
	btn->setText("  查看更多");		// 设置文字
	btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); // 图标在左，文字在右
	btn->setCursor(Qt::PointingHandCursor);
	QFont font;
	font.setPointSize(9);
	btn->setFont(font);
	btn->setStyleSheet(R"(
    QToolButton {
        color: white;
        border: none;
        background: transparent;
		padding: 1px;
	 })");

	connect(btn, &QToolButton::clicked, this, [url]()
		{
			// 写业务逻辑
			QDesktopServices::openUrl(url);
		});
}

void CarouselCard::setTextContent(QString text)
{
	textLabel = new QLabel(text, this);
	textLabel->setWordWrap(true);
	textLabel->setFixedSize(w, h);
	textLabel->setStyleSheet("QLabel { color: white; }");
	textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	QFont font;
	font.setWeight(QFont::Black);
	textLabel->setFont(font);
}

void CarouselCard::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 先绘制圆角背景蒙版
	QPainterPath path;
	path.addRoundedRect(rect(), 15, 15);
	painter.setClipPath(path);

	// 绘制局部
	QPixmap scaledPix = m_pixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	QPoint centerOffset = QPoint((scaledPix.width() - width()) / 2, (scaledPix.height() - height()) / 2);
	//从 scaledPix 中截取出和控件一样大的一个区域（从 centerOffset 开始），并完整绘制到控件上。
	painter.drawPixmap(rect(), scaledPix, QRect(centerOffset, size()));
}

void CarouselCard::resizeEvent(QResizeEvent* event)
{
	if (btn)
	{
		int x = 20;
		int y = height() - btn->size().height() - 20;
		btn->move(x, y);
	}
	if (textLabel)
	{
		textLabel->move(20, height() / 2 + 25);
	}
}