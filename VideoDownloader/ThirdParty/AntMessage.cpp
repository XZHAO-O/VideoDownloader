#include "AntMessage.h"
#include <QPainter>
#include <QTimer>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <QSvgRenderer>
#include <QFontMetrics>
#include <QFont>
#include "DesignSystem.h"

AntMessage::AntMessage(QWidget* parent, Type type, const QString& message)
	: QWidget(parent),
	m_type(type),
	m_message(message),
	m_customOpacity(0.0),
	m_isExit(false),
	m_timeoutOccurred(false),
	m_startPos(QPoint(0, 0)),
	m_endPos(QPoint(0, 0))
{
	setAttribute(Qt::WA_TranslucentBackground);
	initResources();
	setupTimer();
}

AntMessage::~AntMessage() {}

void AntMessage::initResources()
{
	int padding = 18;
	int iconWidth = 20;
	int spacing = 10;

	// 使用字体测量文字宽度
	QFont font;
	font.setPointSizeF(10.5);
	QFontMetrics fm(font);
	int textWidth = fm.horizontalAdvance(m_message);

	// 总宽度 = 左右 padding + 图标 + 间距 + 文字
	int totalWidth = padding + iconWidth + spacing + textWidth + padding;

	// 最小宽度保障
	totalWidth = qMax(totalWidth, 130);
	int height = 40;

	setFixedSize(totalWidth, height);

	// 添加阴影
	auto shadow = new QGraphicsDropShadowEffect(this);
	shadow->setBlurRadius(20);
	shadow->setOffset(0, 2);
	shadow->setColor(QColor(0, 0, 0, 80));
	setGraphicsEffect(shadow);

	// 判断使用哪个图
	switch (m_type)
	{
	case AntMessage::Info:
		m_svgPath = QString(":/Imgs/info.svg");
		break;
	case AntMessage::Success:
		m_svgPath = QString(":/Imgs/true.svg");
		break;
	case AntMessage::Error:
		m_svgPath = QString(":/Imgs/error.svg");
		break;
	case AntMessage::Warning:
		m_svgPath = QString(":/Imgs/warning.svg");
		break;
	default:
		m_svgPath = QString(":/Imgs/true.svg");
		break;
	}
}

void AntMessage::setupTimer()
{
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, this, &AntMessage::onTimeout);
}

void AntMessage::startDisplayTimer(int ms)
{
	timer->start(ms);
}

void AntMessage::onTimeout()
{
	m_timeoutOccurred = true;
	emit requestExit(this);
}

void AntMessage::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setOpacity(m_customOpacity);

	// 背景圆角绘制
	painter.setBrush(DesignSystem::instance()->currentTheme().msgBgColor);
	painter.setPen(Qt::NoPen);
	painter.drawRoundedRect(rect(), 8, 8);

	// 图标绘制
	int padding = 18;
	int iconWidth = 20;
	int spacing = 10;
	QSvgRenderer svgRenderer(m_svgPath);
	QRect iconRect(padding, (height() - iconWidth) / 2, iconWidth, iconWidth);
	svgRenderer.render(&painter, iconRect);

	// 文本绘制
	QRect textRect = rect().adjusted(padding + iconWidth + spacing, 0, -padding, 0);
	QFont font;
	font.setPointSizeF(10.5);
	painter.setFont(font);
	painter.setPen(DesignSystem::instance()->currentTheme().msgTextColor);
	painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_message);
}

void AntMessage::setCustomOpacity(qreal opacity)
{
	m_customOpacity = opacity;
	update();
}

void AntMessage::setIsExit(bool isExit)
{
	m_isExit = isExit;
}

bool AntMessage::isExit()
{
	return m_isExit;
}

bool AntMessage::hasTimeoutOccurred()
{
	return m_timeoutOccurred;
}

qreal AntMessage::getCustomOpacity()
{
	return m_customOpacity;
}