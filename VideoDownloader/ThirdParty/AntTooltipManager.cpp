#include "AntTooltipManager.h"
#include <QGuiApplication>
#include <QScreen>
#include "DesignSystem.h"

AntTooltipManager* AntTooltipManager::m_instance = nullptr;

AntTooltipManager::AntTooltipManager(QWidget* parent)
	: QWidget(parent)
{
}

AntTooltipManager::~AntTooltipManager()
{
}

void AntTooltipManager::showTooltip(QWidget* targetWidget, const QString& text, Position position)
{
	if (!targetWidget)
		return;

	AntTooltip::ArrowDir dir = AntTooltip::None;
	switch (position)
	{
	case AntTooltipManager::Position::Top:    dir = AntTooltip::ArrowBottom; break;
	case AntTooltipManager::Position::Bottom: dir = AntTooltip::ArrowTop; break;
	case AntTooltipManager::Position::Left:   dir = AntTooltip::ArrowRight; break;
	case AntTooltipManager::Position::Right:  dir = AntTooltip::ArrowLeft; break;
	default: break;
	}

	// 创建提示控件
	m_tooltipView = new AntTooltipViewController(text, dir, DesignSystem::instance()->getMainWindow());

	// 隐藏
	connect(this, &AntTooltipManager::hideTip, m_tooltipView, &AntTooltipViewController::hideAnimated);

	const QSize tooltipSize = m_tooltipView->tooltip->size();				// 获取真实尺寸
	QRect targetRect = targetWidget->rect();
	QPoint globalCenter = targetWidget->mapToGlobal(targetRect.center());	// 目标控件中心全局坐标

	QPoint tipOffset = m_tooltipView->tooltip->arrowTipOffset();			// 箭头尖端相对气泡左上角的偏移

	QPoint topLeft;

	int spacing = 8; // 空隙距离

	// +2 是因为qt官方bug 他的像素问题 (有时候即便你算对了位置也会差1到2个像素点)
	switch (position) {
	case Position::Top:
		// 气泡在目标上方，箭头向下，尖端应对准目标中心
		// tipOffset.y() 是箭头尖端相对气泡上边缘的偏移，tipOffset.x()是水平偏移
		topLeft = QPoint(globalCenter.x() - tipOffset.x() + 2,
			targetWidget->mapToGlobal(targetRect.topLeft()).y() - tooltipSize.height() - spacing);
		break;

	case Position::Bottom:
		// 气泡在目标下方，箭头向上
		topLeft = QPoint(globalCenter.x() - tipOffset.x() + 2,
			targetWidget->mapToGlobal(targetRect.bottomLeft()).y() + spacing);
		break;

	case Position::Left:
		// 气泡在目标左侧，箭头向右
		topLeft = QPoint(targetWidget->mapToGlobal(targetRect.topLeft()).x() - tooltipSize.width() - spacing,
			globalCenter.y() - tipOffset.y() + 2);
		break;

	case Position::Right:
		// 气泡在目标右侧，箭头向左
		topLeft = QPoint(targetWidget->mapToGlobal(targetRect.topRight()).x() + spacing,
			globalCenter.y() - tipOffset.y() + 2);
		break;
	}

	QRect tooltipRect(topLeft, tooltipSize);

	// 限制提示框不要超出屏幕范围
	QRect screenRect = QGuiApplication::screenAt(globalCenter)->geometry();
	if (!screenRect.contains(tooltipRect)) {
		tooltipRect.moveTopLeft(screenRect.adjusted(4, 4, -4, -4).intersected(tooltipRect).topLeft());
	}

	m_tooltipView->showAnimated(tooltipRect.topLeft());
}

void AntTooltipManager::hideTooltip()
{
	emit hideTip();
}