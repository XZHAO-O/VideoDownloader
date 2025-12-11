#include "AntButton.h"

#include <QPainterPath>

#include "DesignSystem.h"

AntButton::AntButton(QString btnText, qreal textSize, QWidget* parent)
	:QPushButton(parent),
	m_radius(6),
	m_margin(8),
	m_hovered(false),
	m_pressed(false),
	m_toolTipEnabled(true),  // 默认启用tooltip
	m_toolTipText(""),
	m_toolTipPosition(AntTooltipManager::Position::Top)  // 默认顶部显示
{
	setCursor(Qt::PointingHandCursor);
	QFont font;
	font.setPointSizeF(textSize);
	setFont(font);
	setText(btnText);

	// 连接主题变化信号，直接触发重绘
	connect(DesignSystem::instance(), &DesignSystem::themeChanged,
		this, QOverload<>::of(&AntButton::update));
}

AntButton::~AntButton()
{
	// 确保隐藏tooltip
	hideCustomTooltip();

	if (m_ripples.size() > 0)
	{
		for (auto ripple : m_ripples)
		{
			m_ripples.removeOne(ripple);
			if (ripple)
			{
				delete ripple;
				ripple = nullptr;
			}
		}
	}
	if (m_ripp)
	{
		delete m_ripp;
		m_ripp = nullptr;
	}
}

void AntButton::setIconKey(const QString& iconKey)
{
	if (m_iconKey != iconKey) {
		m_iconKey = iconKey;
		update();
	}
}

void AntButton::setButtonMode(ButtonMode mode)
{
	if (m_buttonMode != mode)
	{
		m_buttonMode = mode;
		update();
	}
}

void AntButton::setStrokeWidth(int width)
{
	if (width >= 0 && m_strokeWidth != width)
	{
		m_strokeWidth = width;
		update();
	}
}

void AntButton::setButtonColor(const QColor& color)
{
	if (m_buttonColor != color)
	{
		m_buttonColor = color;
		update();
	}
}

void AntButton::setTextColor(const QColor& color)
{
	if (m_textColor != color)
	{
		m_textColor = color;
		update();
	}
}

void AntButton::setIconScale(qreal scale)
{
	if (scale > 0 && scale <= 1.0)
	{
		m_scaleFactor = scale;
		update();
	}
}

void AntButton::setHoverIconEnabled(bool enabled)
{
	if (m_hoverIconEnabled != enabled)
	{
		m_hoverIconEnabled = enabled;
		update();
	}
}

void AntButton::setToolTip(const QString& text)
{
	// 保存tooltip文本到成员变量
	m_toolTipText = text;
}

void AntButton::setToolTipEnabled(bool enabled)
{
	if (m_toolTipEnabled != enabled)
	{
		m_toolTipEnabled = enabled;

		// 如果禁用了tooltip且当前有显示，则隐藏它
		if (!enabled && m_hovered)
		{
			hideCustomTooltip();
		}
	}
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
				height() - 2 * m_margin);
			Ripple* ripple = new Ripple(buttonRect, m_radius, this);
			m_ripples.append(ripple);

			// 创建并配置并行动画组
			QParallelAnimationGroup* rippleAnimationGroup = new QParallelAnimationGroup(this);

			// 半径动画：从 0 → 1.0，后面乘以最大半径值
			QPropertyAnimation* offsetAnimation = new QPropertyAnimation(ripple, "m_offset");
			offsetAnimation->setDuration(animTime);
			offsetAnimation->setEasingCurve(QEasingCurve::InOutSine);

			// 透明度动画：从 0.3 → 0.0
			QPropertyAnimation* opacityAnimation = new QPropertyAnimation(ripple, "m_opacity");
			opacityAnimation->setDuration(animTime + 300);
			opacityAnimation->setEasingCurve(QEasingCurve::InOutSine);

			rippleAnimationGroup->addAnimation(offsetAnimation);
			rippleAnimationGroup->addAnimation(opacityAnimation);

			// 仅当动画未运行时才重置
			if (rippleAnimationGroup->state() != QAbstractAnimation::Running)
			{
				ripple->setBeginValue(0, 0.3);
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

	// 如果启用了tooltip且有tooltip文本，显示tooltip
	if (m_toolTipEnabled && !m_toolTipText.isEmpty())
	{
		showCustomTooltip();
	}

	QPushButton::enterEvent(event);
}

void AntButton::leaveEvent(QEvent* event)
{
	m_hovered = false;
	m_pressed = false;
	update();

	// 离开时隐藏tooltip
	hideCustomTooltip();

	QPushButton::leaveEvent(event);
}

void AntButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// 1. 定义按钮矩形区域
	QRectF buttonRect;

	// 2. 判断是否设置了图标
	bool hasIcon = !m_iconKey.isEmpty();

	// 3. 获取当前主题
	const Theme& theme = DesignSystem::instance()->currentTheme();

	// 4. 确定实际使用的颜色
	QColor currentButtonColor = m_buttonColor.isValid() ? m_buttonColor : theme.primaryColor;
	QColor currentTextColor = m_textColor.isValid() ? m_textColor :
		(m_buttonMode == Filled ? theme.textColor : theme.primaryTextColor);

	// 5. 确定边框颜色
	QColor borderColor;
	if (m_buttonMode == Filled) {
		// 填充模式：无边框
		borderColor = Qt::transparent;
	}
	else {
		// 描边模式：边框颜色为主题边框色
		borderColor = theme.borderColor;
		// 悬停时边框颜色为主题悬停边框色
		if (m_hovered)
			borderColor = theme.borderColorHover;
	}

	if (hasIcon)
	{
		// 5.1 圆形按钮
		int diameter = qMin(width(), height());
		buttonRect = QRectF(m_margin, m_margin, diameter - 2 * m_margin, diameter - 2 * m_margin);

		if (m_buttonMode == Filled)
		{
			// 填充模式
			QColor fillColor = currentButtonColor;
			if (m_pressed)
				fillColor = fillColor.darker(120);
			else if (m_hovered)
				fillColor = fillColor.lighter(110);

			painter.setBrush(fillColor);
			painter.setPen(Qt::NoPen);
			painter.drawEllipse(buttonRect);
		}
		else // Outlined 模式
		{
			// 描边模式：Ant Design 的图标按钮描边模式
			// 背景保持透明
			painter.setBrush(Qt::transparent);
			painter.setPen(QPen(borderColor, m_strokeWidth));
			painter.drawEllipse(buttonRect);
		}
	}
	else
	{
		// 5.2 圆角矩形按钮
		buttonRect = QRectF(m_margin, m_margin, width() - 2 * m_margin, height() - 2 * m_margin);

		if (m_buttonMode == Filled)
		{
			// 填充模式：Ant Design 的主要按钮
			QColor fillColor = currentButtonColor;
			if (m_pressed)
				fillColor = fillColor.darker(120);
			else if (m_hovered)
				fillColor = fillColor.lighter(110);

			painter.setBrush(fillColor);
			painter.setPen(Qt::NoPen);
			painter.drawRoundedRect(buttonRect, m_radius, m_radius);
		}
		else // Outlined 模式
		{
			// 描边模式：Ant Design 的默认按钮（取消按钮）
			// 背景保持透明
			painter.setBrush(Qt::transparent);
			painter.setPen(QPen(borderColor, m_strokeWidth));
			painter.drawRoundedRect(buttonRect, m_radius, m_radius);
		}
	}

	// 6. 绘制图标或文字
	if (hasIcon)
	{
		// 获取当前状态下的图标
		QPixmap iconPixmap = getCurrentIcon();
		if (!iconPixmap.isNull())
		{
			// 计算图标矩形
			QSizeF iconSize = buttonRect.size() * m_scaleFactor;
			QRectF iconRect = buttonRect;
			iconRect.setSize(iconSize);
			iconRect.moveCenter(buttonRect.center());

			// 绘制图标，保持宽高比
			QRectF targetRect = iconRect;
			QRectF sourceRect = iconPixmap.rect();

			qreal scale = qMin(targetRect.width() / sourceRect.width(),
				targetRect.height() / sourceRect.height());

			QRectF scaledRect;
			scaledRect.setWidth(sourceRect.width() * scale);
			scaledRect.setHeight(sourceRect.height() * scale);
			scaledRect.moveCenter(targetRect.center());

			painter.drawPixmap(scaledRect, iconPixmap, sourceRect);
		}
	}
	else
	{
		// 绘制文本
		QColor textColor = currentTextColor;

		// 描边模式下，正常状态文字颜色为主题主文字色
		// 悬停和按下时的文字颜色变化
		if (m_buttonMode == Outlined) {
			// 正常状态：使用主题主文字色
			textColor = currentTextColor;

			// 悬停时文字颜色变化
			if (m_hovered) {
				// 如果悬停，文字颜色使用主题色
				textColor = currentButtonColor;
				// 根据按下状态调整
				if (m_pressed)
					textColor = textColor.darker(120);
				else
					textColor = textColor.lighter(110);
			}
			// 按下状态（但鼠标未悬停，这种情况很少见）
			else if (m_pressed) {
				textColor = currentButtonColor.darker(120);
			}
		}
		else {
			// 填充模式下的文字颜色变化
			if (m_pressed)
				textColor = textColor.darker(120);
			else if (m_hovered)
				textColor = textColor.lighter(110);
		}

		painter.setPen(textColor);
		painter.setFont(font());
		painter.drawText(buttonRect, Qt::AlignCenter, text());
	}

	// 7. 如果正在播放波纹动画，绘制涟漪效果（改回最初的样子）
	if (!m_ripples.isEmpty())
	{
		// 改回最初的样子：统一使用按钮颜色作为涟漪颜色
		QColor rippleColor = currentButtonColor;
		painter.setPen(Qt::NoPen);

		for (Ripple* ripple : m_ripples)
		{
			// 改回最初的样子：涟漪颜色透明度由ripple->opacity()控制
			rippleColor.setAlphaF(ripple->opacity());
			painter.setBrush(rippleColor);

			// 7.1 绘制涟漪"环形"路径
			int rippleOffset = ripple->offset();
			QRectF outerRect = buttonRect.adjusted(
				-rippleOffset, -rippleOffset,
				+rippleOffset, +rippleOffset
			);

			QRectF innerRect = buttonRect;

			// 7.2 计算外环与内环路径
			QPainterPath outerPath;
			if (hasIcon)
				outerPath.addEllipse(outerRect);
			else
				outerPath.addRoundedRect(outerRect, m_radius, m_radius);

			QPainterPath innerPath;
			if (hasIcon)
				innerPath.addEllipse(innerRect);
			else
				innerPath.addRoundedRect(innerRect, m_radius, m_radius);

			QPainterPath ringPath = outerPath.subtracted(innerPath);

			// 7.3 填充涟漪环形路径
			painter.drawPath(ringPath);
		}
	}
}

QPixmap AntButton::getCurrentIcon() const
{
	if (m_iconKey.isEmpty()) {
		return QPixmap();
	}

	// 获取当前主题模式
	DesignSystem::ThemeMode theme = DesignSystem::instance()->themeMode();

	// 根据当前主题和悬停状态确定索引
	int index = 0;

	if (theme == DesignSystem::Dark) {
		index = (m_hovered && m_hoverIconEnabled) ? 3 : 2;
	}
	else {
		index = (m_hovered && m_hoverIconEnabled) ? 1 : 0;
	}

	// 通过DesignSystem获取pixmap
	return DesignSystem::instance()->getSvgIcon(m_iconKey, index);
}

void AntButton::updateButtonSize()
{
	// 注意：AntButton的大小通常由布局控制，这里不设置固定大小
	// 如果需要固定大小，可以在外部调用setFixedSize
}

void AntButton::showCustomTooltip()
{
	// 只有鼠标仍在按钮上时才显示tooltip
	if (m_hovered && m_toolTipEnabled && !m_toolTipText.isEmpty()) {
		AntTooltipManager::instance()->showTooltip(this, m_toolTipText, m_toolTipPosition);
	}
}

void AntButton::hideCustomTooltip()
{
	AntTooltipManager::instance()->hideTooltip();
}