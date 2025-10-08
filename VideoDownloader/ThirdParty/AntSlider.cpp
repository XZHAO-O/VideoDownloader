// AntSlider.cpp
#include "AntSlider.h"
#include <QPainter>
#include <QStyleOptionSlider>
#include <QMouseEvent>
#include <QPainterPath>
#include <QTimer>
#include "DesignSystem.h"

AntSlider::AntSlider(int min, int max, int value, QWidget* parent)
	: QSlider(Qt::Horizontal, parent)
{
	// 初始化范围和值
	setRange(min, max);
	setValue(value);
	setMouseTracking(true);
	setContentsMargins(m_margin, 0, m_margin, 0);

	// 初始化 handle 动画
	m_radiusAnimation = new QPropertyAnimation(this, "handleRadius", this);
	m_radiusAnimation->setDuration(180);
	m_radiusAnimation->setEasingCurve(QEasingCurve::InOutQuad);

	// 初始化 tooltip
	m_tooltip = new DownArrowTooltip(nullptr);
	m_tooltip->setText(QString::number(value));
	m_tooltip->hide();

	// 槽函数连接
	connect(this, &QSlider::sliderPressed, this, [this]() {
		m_tooltip->setText(QString::number(this->value()));
		});

	connect(this, &QSlider::valueChanged, this, [this](int val) {
		m_tooltip->setText(QString::number(val));
		});

	connect(this, &AntSlider::handleMoved, this, [=](const QPoint& pt) {
		QPoint global = mapToGlobal(pt);
		m_tooltip->move(global.x() - m_tooltip->width() / 2,
			global.y() - m_tooltip->height() - 15);
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			update();
		});
}

void AntSlider::setHandleRadius(qreal radius)
{
	if (!qFuzzyCompare(m_handleRadius, radius)) {
		m_handleRadius = radius;
		update();
	}
}

void AntSlider::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	// ---- 轨道 ---------------------------------------------------
	const QRect tr = trackRect();
	const int   handleX = tr.left()
		+ qRound(double(value() - minimum())
			/ (maximum() - minimum())
			* tr.width());

	p.setPen(Qt::NoPen);
	p.setBrush(DesignSystem::instance()->currentTheme().slideBgColor);                // 背景
	p.drawRoundedRect(tr, tr.height() / 2, tr.height() / 2);

	QRect fillRect(tr.left(), tr.top(),
		handleX - tr.left(), tr.height());
	p.setBrush(DesignSystem::instance()->currentTheme().slideColor);                 // 已完成部分
	p.drawRoundedRect(fillRect, tr.height() / 2, tr.height() / 2);

	// ---- 滑块 ---------------------------------------------------
	const QPointF c(handleX, height() / 2.0);
	const qreal outerR = m_handleRadius;
	const qreal ringW = static_cast<qreal>(m_ringWidth);
	const qreal innerR = outerR - ringW / 2.0;

	QColor ringColor = m_hovered ? DesignSystem::instance()->currentTheme().ringhoveredColor
		: DesignSystem::instance()->currentTheme().ringColor;

	p.setBrush(Qt::NoBrush);
	p.setPen(QPen(ringColor, ringW));
	p.drawEllipse(c, outerR, outerR);               // 外环

	p.setPen(Qt::NoPen);
	p.setBrush(Qt::white);
	p.drawEllipse(c, innerR, innerR);               // 白心

	// 发射滑块中心坐标
	emit handleMoved(QPoint(handleX, height() / 2));
}

void AntSlider::enterEvent(QEnterEvent* event)
{
	QSlider::enterEvent(event);
	m_hovered = true;
	m_radiusAnimation->stop();
	m_radiusAnimation->setStartValue(m_handleRadius);
	m_radiusAnimation->setEndValue(9);
	m_radiusAnimation->start();
	m_tooltip->fadeIn();
	update();
}

void AntSlider::leaveEvent(QEvent* event)
{
	QSlider::leaveEvent(event);
	m_hovered = false;
	m_radiusAnimation->stop();
	m_radiusAnimation->setStartValue(m_handleRadius);
	m_radiusAnimation->setEndValue(7);
	m_radiusAnimation->start();
	m_tooltip->fadeOut();
	update();
}

void AntSlider::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
	{
		const QRect tr = trackRect();
		int pos = std::clamp(e->pos().x(), tr.left(), tr.right());
		// 用于将点击位置 pos 映射为对应的滑块值
		int v = QStyle::sliderValueFromPosition(minimum(), maximum(),
			pos - tr.left(),
			tr.width());
		setValue(v);
		e->accept();
	}
	QSlider::mousePressEvent(e);   // 继续支持拖拽
}

QSize AntSlider::sizeHint() const
{
	QSize s = QSlider::sizeHint();
	s.setHeight(m_maxRadius * 2 + m_ringWidth + 4);
	return s;
}

DownArrowTooltip::DownArrowTooltip(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);

	m_font.setPointSize(12);
	setFont(m_font);

	// ---------- 添加透明度效果 ----------
	auto* opacityEffect = new QGraphicsOpacityEffect(this);
	setGraphicsEffect(opacityEffect);

	m_opacityAnimation = new QPropertyAnimation(opacityEffect, "opacity", this);
	m_opacityAnimation->setDuration(300);
	m_opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);

	connect(m_opacityAnimation, &QPropertyAnimation::finished, this, [this]() {
		if (isHide)
			this->hide();
		});
}

void DownArrowTooltip::setText(const QString& text)
{
	m_text = text;

	QFontMetrics fm(m_font);
	int textWidth = fm.horizontalAdvance(m_text);
	int totalWidth = textWidth + padding * 2;

	resize(totalWidth, fixedHeight + arrowHeight);
	update();
}

void DownArrowTooltip::showAt(const QPoint& globalPos)
{
	move(globalPos);
	show();
}

// DownArrowTooltip

void DownArrowTooltip::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	const int radius = 6;

	QRect bubbleRect(0, 0, width(), height() - arrowHeight);
	QPainterPath path;

	// 圆角矩形部分
	path.moveTo(radius, 0);
	path.lineTo(width() - radius, 0);
	path.quadTo(width(), 0, width(), radius);
	path.lineTo(width(), bubbleRect.bottom() - radius);
	path.quadTo(width(), bubbleRect.bottom(), width() - radius, bubbleRect.bottom());
	path.lineTo(radius, bubbleRect.bottom());
	path.quadTo(0, bubbleRect.bottom(), 0, bubbleRect.bottom() - radius);
	path.lineTo(0, radius);
	path.quadTo(0, 0, radius, 0);

	// 向下箭头
	int ax = width() / 2;
	int ay = bubbleRect.bottom();
	path.moveTo(ax - arrowWidth / 2, ay);
	path.lineTo(ax, ay + arrowHeight);
	path.lineTo(ax + arrowWidth / 2, ay);
	path.closeSubpath();

	// 填充背景：黑色
	p.setBrush(DesignSystem::instance()->currentTheme().toolTipBgColor);  // 深灰接近黑
	p.setPen(Qt::NoPen);
	p.drawPath(path);

	// 文字：白色
	p.setFont(m_font);
	p.setPen(DesignSystem::instance()->currentTheme().toolTipTextColor);
	p.drawText(bubbleRect, Qt::AlignCenter, m_text);
}

void DownArrowTooltip::fadeIn()
{
	isHide = false;
	show();
	m_opacityAnimation->stop();
	m_opacityAnimation->setStartValue(0.0);
	m_opacityAnimation->setEndValue(1.0);
	m_opacityAnimation->start();
}

void DownArrowTooltip::fadeOut()
{
	isHide = true;
	m_opacityAnimation->stop();
	m_opacityAnimation->setStartValue(1.0);
	m_opacityAnimation->setEndValue(0.0);
	m_opacityAnimation->start();
}