#include "AntTooltipManager.h"
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>
#include "DesignSystem.h"

AntTooltipManager* AntTooltipManager::m_instance = nullptr;

AntTooltipManager::AntTooltipManager(QWidget* parent)
	: QWidget(parent)
{
	// 创建隐藏定时器（用于延迟隐藏）
	m_hideTimer = new QTimer(this);
	m_hideTimer->setSingleShot(true);
	connect(m_hideTimer, &QTimer::timeout, this, &AntTooltipManager::hideTooltipImmediately);

	// 应用程序退出时清理
	connect(qApp, &QCoreApplication::aboutToQuit, this, &AntTooltipManager::cleanup);
}

AntTooltipManager::~AntTooltipManager()
{
	cleanup();
}

void AntTooltipManager::showTooltip(QWidget* targetWidget, const QString& text, Position position)
{
	if (!targetWidget || text.isEmpty()) {
		return;
	}

	// 如果当前显示的工具提示是给同一个控件的，并且文本和位置都相同，则直接返回
	if (m_tooltipView && m_tooltipView->isVisible() &&
		m_currentTargetWidget == targetWidget &&
		m_currentText == text &&
		m_currentPosition == position) {
		return;
	}

	// 停止隐藏定时器
	if (m_hideTimer && m_hideTimer->isActive()) {
		m_hideTimer->stop();
	}

	// 确定箭头方向
	AntTooltip::ArrowDir dir = AntTooltip::None;
	switch (position)
	{
	case AntTooltipManager::Position::Top:    dir = AntTooltip::ArrowBottom; break;
	case AntTooltipManager::Position::Bottom: dir = AntTooltip::ArrowTop; break;
	case AntTooltipManager::Position::Left:   dir = AntTooltip::ArrowRight; break;
	case AntTooltipManager::Position::Right:  dir = AntTooltip::ArrowLeft; break;
	default: break;
	}

	// 如果工具提示不存在，创建它
	if (!m_tooltipView) {
		m_tooltipView = new AntTooltipViewController(text, dir, DesignSystem::instance()->getMainWindow());

		// 连接隐藏信号
		connect(this, &AntTooltipManager::hideTip, m_tooltipView, &AntTooltipViewController::hideAnimated);

		// 当工具提示被隐藏时，清除当前目标控件
		connect(m_tooltipView, &AntTooltipViewController::hidden, this, [this]() {
			m_currentTargetWidget = nullptr;
			m_currentText.clear();
			});
	}
	else {
		// 更新现有工具提示的内容
		updateTooltipContent(text, dir);
	}

	// 保存当前状态
	m_currentTargetWidget = targetWidget;
	m_currentText = text;
	m_currentPosition = position;

	// 计算位置
	const QSize tooltipSize = m_tooltipView->tooltip->size();
	QPoint topLeft = calculateTooltipPosition(targetWidget, tooltipSize, position);

	// 显示工具提示
	m_tooltipView->showAnimated(topLeft);
}

void AntTooltipManager::hideTooltip()
{
	if (m_tooltipView && m_tooltipView->isVisible()) {
		emit hideTip();
	}

	// 延迟清除当前目标控件（等待动画完成）
	if (m_hideTimer) {
		m_hideTimer->start(300);  // 300ms后清除
	}
}

void AntTooltipManager::hideTooltipImmediately()
{
	if (m_tooltipView) {
		m_tooltipView->hide();
		m_currentTargetWidget = nullptr;
		m_currentText.clear();
	}
}

void AntTooltipManager::cleanup()
{
	if (m_hideTimer) {
		m_hideTimer->stop();
	}

	if (m_tooltipView) {
		m_tooltipView->hide();
		m_tooltipView->deleteLater();
		m_tooltipView = nullptr;
	}

	m_currentTargetWidget = nullptr;
	m_currentText.clear();
}

void AntTooltipManager::updateTooltipContent(const QString& text, AntTooltip::ArrowDir dir)
{
	if (!m_tooltipView || !m_tooltipView->tooltip) {
		return;
	}

	// 更新文本
	m_tooltipView->tooltip->setText(text);

	// 注意：箭头方向在AntTooltip构造时确定，无法修改
	// 如果需要箭头方向不同，需要重新创建AntTooltip
	// 这里假设大多数情况下箭头方向相同
	// 如果箭头方向确实需要改变，我们可以重新创建整个视图
	// 但考虑到性能，我们可以假设同一控件的tooltip位置是固定的
}

QPoint AntTooltipManager::calculateTooltipPosition(QWidget* targetWidget, const QSize& tooltipSize, Position position)
{
	QRect targetRect = targetWidget->rect();
	QPoint globalCenter = targetWidget->mapToGlobal(targetRect.center());
	QPoint tipOffset = m_tooltipView->tooltip->arrowTipOffset();

	QPoint topLeft;
	int spacing = 8; // 空隙距离

	switch (position) {
	case Position::Top:
		topLeft = QPoint(globalCenter.x() - tipOffset.x() + 2,
			targetWidget->mapToGlobal(targetRect.topLeft()).y() - tooltipSize.height() - spacing);
		break;

	case Position::Bottom:
		topLeft = QPoint(globalCenter.x() - tipOffset.x() + 2,
			targetWidget->mapToGlobal(targetRect.bottomLeft()).y() + spacing);
		break;

	case Position::Left:
		topLeft = QPoint(targetWidget->mapToGlobal(targetRect.topLeft()).x() - tooltipSize.width() - spacing,
			globalCenter.y() - tipOffset.y() + 2);
		break;

	case Position::Right:
		topLeft = QPoint(targetWidget->mapToGlobal(targetRect.topRight()).x() + spacing,
			globalCenter.y() - tipOffset.y() + 2);
		break;
	}

	// 限制提示框不要超出屏幕范围
	QRect screenRect = QGuiApplication::screenAt(globalCenter)->geometry();
	QRect tooltipRect(topLeft, tooltipSize);

	if (!screenRect.contains(tooltipRect)) {
		topLeft = screenRect.adjusted(4, 4, -4, -4).intersected(tooltipRect).topLeft();
	}

	return topLeft;
}