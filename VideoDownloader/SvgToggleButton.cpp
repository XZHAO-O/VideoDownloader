#include "SvgToggleButton.h"

#include "DesignSystem.h"

SvgToggleButton::SvgToggleButton(QWidget* parent)
	: SvgButton(parent),
	m_normalIconKey(""),
	m_activeIconKey(""),
	m_normalToolTip(""),
	m_activeToolTip(""),
	m_active(false)  // 默认处于正常状态（关闭状态）
{
	// 设置鼠标样式
	setCursor(Qt::PointingHandCursor);
}

SvgToggleButton::SvgToggleButton(const QString& normalIconKey,
	const QString& activeIconKey,
	QWidget* parent)
	: SvgButton(normalIconKey, parent),  // 初始使用正常图标
	m_normalIconKey(normalIconKey),
	m_activeIconKey(activeIconKey),
	m_normalToolTip(""),
	m_activeToolTip(""),
	m_active(false)  // 默认处于正常状态（关闭状态）
{
	// 设置鼠标样式
	setCursor(Qt::PointingHandCursor);

	// 如果激活图标键值不为空，连接主题变化信号
	if (!m_activeIconKey.isEmpty())
	{
		// 连接主题变化信号，直接触发重绘
		connect(DesignSystem::instance(), &DesignSystem::themeChanged,
			this, QOverload<>::of(&SvgToggleButton::update));
	}
}

SvgToggleButton::~SvgToggleButton()
{
}

void SvgToggleButton::setNormalIconKey(const QString& iconKey)
{
	if (m_normalIconKey != iconKey)
	{
		m_normalIconKey = iconKey;

		// 如果当前是普通状态，更新图标
		if (!m_active)
		{
			SvgButton::setIconKey(iconKey);
			update();
		}
	}
}

void SvgToggleButton::setActiveIconKey(const QString& iconKey)
{
	if (m_activeIconKey != iconKey)
	{
		m_activeIconKey = iconKey;

		// 如果当前是激活状态，更新图标
		if (m_active)
		{
			SvgButton::setIconKey(iconKey);
			update();
		}
	}
}

void SvgToggleButton::setActive(bool active)
{
	if (m_active != active)
	{
		m_active = active;
		updateIconKey();
		updateToolTip();
		update();

		// 发射状态变化信号
		emit stateChanged(m_active);
	}
}

void SvgToggleButton::toggle()
{
	setActive(!m_active);
}

void SvgToggleButton::setToolTip(const QString& text)
{
	// 设置统一的tooltip，同时应用于两个状态
	m_normalToolTip = text;
	m_activeToolTip = text;

	// 更新当前状态的tooltip
	updateToolTip();
}

void SvgToggleButton::setNormalToolTip(const QString& text)
{
	m_normalToolTip = text;

	// 如果当前是正常状态，更新tooltip
	if (!m_active)
	{
		updateToolTip();
	}
}

void SvgToggleButton::setActiveToolTip(const QString& text)
{
	m_activeToolTip = text;

	// 如果当前是激活状态，更新tooltip
	if (m_active)
	{
		updateToolTip();
	}
}

void SvgToggleButton::mousePressEvent(QMouseEvent* event)
{
	// 先调用基类处理
	SvgButton::mousePressEvent(event);
}

void SvgToggleButton::mouseReleaseEvent(QMouseEvent* event)
{
	// 先调用基类处理
	SvgButton::mouseReleaseEvent(event);

	// 只在左键释放时切换状态
	if (event->button() == Qt::LeftButton && rect().contains(event->pos()))
	{
		// 切换状态
		toggle();

		// 根据切换后的状态发送信号
		if (m_active)
		{
			// 现在处于激活状态
			emit actived();
		}
		else
		{
			// 现在处于正常状态
			emit normalized();
		}

		// 发送通用的 clicked 信号
		emit clicked();
	}
}

void SvgToggleButton::enterEvent(QEnterEvent* event)
{
	// 先调用基类的enterEvent
	SvgButton::enterEvent(event);

	// 在进入事件中强制更新tooltip
	updateToolTip();
}

void SvgToggleButton::updateIconKey()
{
	// 根据当前状态设置对应的图标键值
	if (m_active && !m_activeIconKey.isEmpty())
	{
		SvgButton::setIconKey(m_activeIconKey);
	}
	else
	{
		SvgButton::setIconKey(m_normalIconKey);
	}
}

void SvgToggleButton::updateToolTip()
{
	// 根据当前状态设置对应的tooltip
	QString currentToolTip;
	if (m_active)
	{
		currentToolTip = m_activeToolTip;
	}
	else
	{
		currentToolTip = m_normalToolTip;
	}

	// 调用基类的setToolTip方法
	SvgButton::setToolTip(currentToolTip);

	// 如果当前鼠标在按钮上，需要立即更新tooltip显示
	if (underMouse())
	{
		// 隐藏当前tooltip（如果正在显示）
		hideCustomTooltip();

		// 重新显示新的tooltip
		showCustomTooltip();
	}
}