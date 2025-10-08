#include "AntButton.h"
#include <QPainter>
#include <QPainterPath>
#include <QOverload>
#include "DesignSystem.h"

AntButton::AntButton(QString btnText, qreal textSize, QWidget* parent)
	:QPushButton(parent),
	m_radius(6),
	m_margin(8),
	m_hovered(false),
	m_pressed(false),
	baseColor(DesignSystem::instance()->primaryColor()) // Ant Design 默认主蓝色
{
	setCursor(Qt::PointingHandCursor);
	QFont font;
	font.setPointSizeF(textSize);
	setFont(font);
	setText(btnText);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			baseColor = DesignSystem::instance()->primaryColor();
			update();
		});
}

AntButton::~AntButton()
{
	if (m_svgRenderer)
	{
		delete m_svgRenderer; // 清理SVG渲染器
	}
}

void AntButton::setSvgIcon(const QString& iconPath)
{
	m_svgRenderer = new QSvgRenderer(iconPath, this);
	update(); // 更新按钮显示
}

void AntButton::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		if (!m_clickTimer.isValid() || m_clickTimer.elapsed() >= m_clickIntervalMs)
		{
			m_clickTimer.restart();

			// 点击时创建一个新的波纹实例
			QRectF buttonRect = QRectF(m_margin, m_margin,
				width() - 2 * m_margin,
				height() - 2 * m_margin);	// 波纹起始矩形区域
			Ripple* ripple = new Ripple(buttonRect, m_radius, this);
			m_ripples.append(ripple);

			// 创建并配置并行动画组
			QParallelAnimationGroup* rippleAnimationGroup = new QParallelAnimationGroup(this);

			// 半径动画：从 0 → 1.0，后面乘以最大半径值
			QPropertyAnimation* offsetAnimation = new QPropertyAnimation(ripple, "m_offset");
			offsetAnimation->setDuration(animTime);
			offsetAnimation->setEasingCurve(QEasingCurve::InOutSine);

			// 透明度动画：从 0.8 → 0.0
			QPropertyAnimation* opacityAnimation = new QPropertyAnimation(ripple, "m_opacity");
			opacityAnimation->setDuration(animTime + 300);
			opacityAnimation->setEasingCurve(QEasingCurve::InOutSine);

			rippleAnimationGroup->addAnimation(offsetAnimation);
			rippleAnimationGroup->addAnimation(opacityAnimation);

			// 仅当动画未运行时才重置
			if (rippleAnimationGroup->state() != QAbstractAnimation::Running)
			{
				ripple->setBeginValue(0, 0.5);
			}

			// 配置动画参数
			offsetAnimation->setStartValue(ripple->offset());
			offsetAnimation->setEndValue(m_margin - 2);

			opacityAnimation->setStartValue(ripple->opacity());
			opacityAnimation->setEndValue(0.0);

			// 属性变化时更新界面
			connect(ripple, &Ripple::offsetChanged, this, QOverload<>::of(&AntButton::update));
			connect(ripple, &Ripple::opacityChanged, this, QOverload<>::of(&AntButton::update));

			// 启动动画
			rippleAnimationGroup->start(QAbstractAnimation::DeleteWhenStopped);

			// 动画结束后移除波纹实例
			connect(rippleAnimationGroup, &QParallelAnimationGroup::finished, this, [this, ripple]()
				{
					m_ripples.removeOne(ripple);
					ripple->deleteLater();
				});

			m_pressed = true;
			update();
		}
	}
	QPushButton::mousePressEvent(event);
}

void AntButton::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_pressed = false;
		update();
	}
	QPushButton::mouseReleaseEvent(event);
}

void AntButton::enterEvent(QEnterEvent* event)
{
	m_hovered = true;
	update();
	QPushButton::enterEvent(event);
}

void AntButton::leaveEvent(QEvent* event)
{
	m_hovered = false;
	update();
	QPushButton::leaveEvent(event);
}

void AntButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 1. 定义按钮矩形区域
	QRectF buttonRect;

	// 2. 判断是否设置了图标
	bool hasIcon = (m_svgRenderer != nullptr); // 如果有图标，背景是圆形

	if (hasIcon)
	{
		// 2.1 绘制按钮背景（圆形）
		int diameter = qMin(width(), height()); // 使用宽度或高度中的较小值来确保按钮是圆形的
		buttonRect = QRectF(m_margin, m_margin, diameter - 2 * m_margin, diameter - 2 * m_margin);
		QColor fillColor = baseColor;
		if (m_pressed)
			fillColor = baseColor.darker(120); // 按下时变暗
		else if (m_hovered)
			fillColor = baseColor.lighter(110); // 悬停时变亮
		painter.setBrush(fillColor);
		painter.setPen(Qt::NoPen);
		painter.drawEllipse(buttonRect); // 绘制圆形背景
	}
	else
	{
		// 2.2 绘制按钮背景（圆角矩形）
		buttonRect = QRectF(m_margin, m_margin, width() - 2 * m_margin, height() - 2 * m_margin);
		QColor fillColor = baseColor;
		if (m_pressed)
			fillColor = baseColor.darker(120); // 按下时变暗
		else if (m_hovered)
			fillColor = baseColor.lighter(110); // 悬停时变亮
		painter.setBrush(fillColor);
		painter.setPen(Qt::NoPen);
		painter.drawRoundedRect(buttonRect, m_radius, m_radius); // 绘制圆角矩形背景
	}

	// 3. 绘制图标或文字
	if (hasIcon)
	{
		// 如果有图标，绘制SVG图标
		QSizeF iconSize = buttonRect.size() * m_scaleFactor;
		QRectF iconRect = buttonRect;
		iconRect.setSize(iconSize);
		iconRect.moveCenter(buttonRect.center());
		m_svgRenderer->render(&painter, iconRect.toRect());
	}
	else
	{
		// 如果没有图标，绘制文本
		painter.setPen(DesignSystem::instance()->currentTheme().textColor);
		painter.setFont(font());
		painter.drawText(buttonRect, Qt::AlignCenter, text()); // 绘制文字
	}

	// 4. 如果正在播放波纹动画，绘制涟漪效果
	QColor rippleColor = baseColor;
	painter.setPen(Qt::NoPen);
	for (Ripple* ripple : m_ripples)
	{
		rippleColor.setAlphaF(ripple->opacity());
		painter.setBrush(rippleColor);

		// 4.1 绘制涟漪“环形”路径
		int rippleOffset = ripple->offset();
		QRectF outerRect = buttonRect.adjusted(
			-rippleOffset, -rippleOffset,
			+rippleOffset, +rippleOffset
		);

		QRectF innerRect = buttonRect;

		// 4.2 计算外环与内环路径
		QPainterPath outerPath;
		if (hasIcon)
			outerPath.addEllipse(outerRect); // 如果是圆形背景，使用圆形路径
		else
			outerPath.addRoundedRect(outerRect, m_radius, m_radius); // 否则使用圆角矩形

		QPainterPath innerPath;
		if (hasIcon)
			innerPath.addEllipse(innerRect); // 内环路径为圆形
		else
			innerPath.addRoundedRect(innerRect, m_radius, m_radius); // 内环路径为圆角矩形

		QPainterPath ringPath = outerPath.subtracted(innerPath);

		// 4.3 填充涟漪环形路径
		painter.drawPath(ringPath);
	}
}