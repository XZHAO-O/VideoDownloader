#include "SvgButton.h"

#include "DesignSystem.h"

SvgButton::SvgButton(QWidget* parent)
	: QPushButton(parent),
	m_iconKey(""),
	m_hovered(false),
	m_pressed(false),
	m_hoverIconEnabled(true),
	m_toolTipEnabled(true),  // 默认启用tooltip
	m_iconMargin(4),
	m_iconScale(0.8),
	m_iconSize(Medium, Medium),
	m_toolTipText(""),
	m_toolTipPosition(AntTooltipManager::Position::Top)  // 默认显示
{
	setCursor(Qt::PointingHandCursor);

	// 设置默认大小
	setFixedSize(m_iconSize);

	// 连接主题变化信号，直接触发重绘
	connect(DesignSystem::instance(), &DesignSystem::themeChanged,
		this, QOverload<>::of(&SvgButton::update));
}

SvgButton::SvgButton(const QString& iconKey, QWidget* parent)
	: QPushButton(parent),
	m_iconKey(iconKey),
	m_hovered(false),
	m_pressed(false),
	m_hoverIconEnabled(true),
	m_toolTipEnabled(true),  // 默认启用tooltip
	m_iconMargin(4),
	m_iconScale(0.8),
	m_iconSize(Medium, Medium),
	m_toolTipText(""),
	m_toolTipPosition(AntTooltipManager::Position::Top)  // 默认显示
{
	setCursor(Qt::PointingHandCursor);

	// 设置默认大小
	setFixedSize(m_iconSize);

	// 连接主题变化信号，直接触发重绘
	connect(DesignSystem::instance(), &DesignSystem::themeChanged,
		this, QOverload<>::of(&SvgButton::update));
}

SvgButton::~SvgButton()
{
	// 确保隐藏tooltip
	hideCustomTooltip();
}

void SvgButton::setIconKey(const QString& iconKey)
{
	if (m_iconKey != iconKey) {
		m_iconKey = iconKey;
		update();
	}
}

void SvgButton::setIconSize(IconSize size)
{
	int iconSize = static_cast<int>(size);
	setIconSize(iconSize, iconSize);
}

void SvgButton::setIconSize(int width, int height)
{
	if (width > 0 && height > 0 && m_iconSize != QSize(width, height)) {
		m_iconSize = QSize(width, height);
		updateButtonSize();
		updateIconRect();
		update();
	}
}

void SvgButton::setIconScale(qreal scale)
{
	if (scale > 0 && scale <= 1.0 && qAbs(m_iconScale - scale) > 0.01) {
		m_iconScale = scale;
		updateIconRect();
		update();
	}
}

void SvgButton::setIconMargin(int margin)
{
	if (margin >= 0 && m_iconMargin != margin) {
		m_iconMargin = margin;
		updateIconRect();
		update();
	}
}

void SvgButton::setHoverIconEnabled(bool enabled)
{
	if (m_hoverIconEnabled != enabled) {
		m_hoverIconEnabled = enabled;
		update();
	}
}

void SvgButton::setToolTip(const QString& text)
{
	// 保存tooltip文本到成员变量
	m_toolTipText = text;
}

void SvgButton::setToolTipEnabled(bool enabled)
{
	if (m_toolTipEnabled != enabled) {
		m_toolTipEnabled = enabled;

		// 如果禁用了tooltip且当前有显示，则隐藏它
		if (!enabled && m_hovered) {
			hideCustomTooltip();
		}
	}
}

void SvgButton::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	// 绘制背景（透明）
	painter.fillRect(rect(), Qt::transparent);

	// 获取当前pixmap
	QPixmap pixmap = getCurrentPixmap();
	if (pixmap.isNull()) {
		// 如果没有有效的pixmap，绘制一个占位符
		painter.setPen(Qt::gray);
		painter.setBrush(Qt::lightGray);
		painter.drawRect(m_iconRect);
		return;
	}

	// 绘制图标，保持宽高比
	QRectF targetRect = m_iconRect;
	QRectF sourceRect = pixmap.rect();

	// 计算保持宽高比的缩放
	qreal scale = qMin(targetRect.width() / sourceRect.width(),
		targetRect.height() / sourceRect.height());

	QRectF scaledRect;
	scaledRect.setWidth(sourceRect.width() * scale);
	scaledRect.setHeight(sourceRect.height() * scale);
	scaledRect.moveCenter(targetRect.center());

	painter.drawPixmap(scaledRect, pixmap, sourceRect);
}

void SvgButton::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_pressed = true;
		update();
	}
	QPushButton::mousePressEvent(event);
}

void SvgButton::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_pressed = false;
		update();
	}
	QPushButton::mouseReleaseEvent(event);
}

void SvgButton::enterEvent(QEnterEvent* event)
{
	m_hovered = true;

	// 如果启用了tooltip且有tooltip文本，显示tooltip
	if (m_toolTipEnabled && !m_toolTipText.isEmpty())
	{
		showCustomTooltip();
	}

	update();
	QPushButton::enterEvent(event);
}

void SvgButton::leaveEvent(QEvent* event)
{
	m_hovered = false;
	m_pressed = false;
	update();

	// 离开时隐藏tooltip
	hideCustomTooltip();

	QPushButton::leaveEvent(event);
}

void SvgButton::resizeEvent(QResizeEvent* event)
{
	updateIconRect();
	QPushButton::resizeEvent(event);
}

QPixmap SvgButton::getCurrentPixmap() const
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

void SvgButton::updateIconRect()
{
	int buttonWidth = width();
	int buttonHeight = height();

	// 计算图标大小
	int iconWidth = static_cast<int>(buttonWidth * m_iconScale);
	int iconHeight = static_cast<int>(buttonHeight * m_iconScale);

	// 考虑边距
	iconWidth -= 2 * m_iconMargin;
	iconHeight -= 2 * m_iconMargin;

	// 确保图标大小有效
	iconWidth = qMax(1, iconWidth);
	iconHeight = qMax(1, iconHeight);

	// 居中显示图标
	int x = (buttonWidth - iconWidth) / 2;
	int y = (buttonHeight - iconHeight) / 2;

	m_iconRect = QRectF(x, y, iconWidth, iconHeight);
}

void SvgButton::updateButtonSize()
{
	// 设置按钮固定大小，考虑边距
	int totalWidth = m_iconSize.width() + 2 * m_iconMargin;
	int totalHeight = m_iconSize.height() + 2 * m_iconMargin;

	setFixedSize(totalWidth, totalHeight);
}

void SvgButton::showCustomTooltip()
{
	// 只有鼠标仍在按钮上时才显示tooltip
	if (m_hovered && m_toolTipEnabled && !m_toolTipText.isEmpty()) {
		AntTooltipManager::instance()->showTooltip(this, m_toolTipText, m_toolTipPosition);
	}
}

void SvgButton::hideCustomTooltip()
{
	AntTooltipManager::instance()->hideTooltip();
}