#include "AntScrollArea.h"
#include "StyleSheet.h"
#include "DesignSystem.h"
#include <QScrollBar>

AntScrollArea::AntScrollArea(ScrollDirection direction, QWidget* parent)
	: QScrollArea(parent)
{
	setObjectName("AntScrollArea");

	// QScrollArea 会自动根据滚动区域尺寸 调整内容控件的尺寸
	setWidgetResizable(true);

	// 设置滚动策略
	setScrollPolicy(direction);

	// 样式表
	auto theme = DesignSystem::instance()->currentTheme();
	viewport()->setStyleSheet("background-color:transparent;");
	setStyleSheet(StyleSheet::antScrollAreaQss(theme.backgroundColor, theme.scrollAreaHandleColor, theme.scrollAreaHoverColor));

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			auto theme = DesignSystem::instance()->currentTheme();
			setStyleSheet(StyleSheet::antScrollAreaQss(theme.backgroundColor, theme.scrollAreaHandleColor, theme.scrollAreaHoverColor));
		});
}

AntScrollArea::~AntScrollArea()
{
}

void AntScrollArea::addWidget(QWidget* w)
{
	setWidget(w);
}

void AntScrollArea::setScrollPolicy(int direction)
{
	setVerticalScrollBarPolicy(
		(direction & ScrollVertical) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(
		(direction & ScrollHorizontal) ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
}

void AntScrollArea::ensureWidgetVisibleInScrollArea(QWidget* targetWidget, int xMargin, int yMargin)
{
	if (!targetWidget)
		return;

	QWidget* contentWidget = this->widget();
	if (!contentWidget)
		return;

	// 计算目标控件左上角在内容widget坐标系的位置
	QPoint posInContent = targetWidget->mapTo(contentWidget, QPoint(0, 0));

	// 调用 ensureVisible 让目标控件可见
	this->ensureVisible(posInContent.x(), posInContent.y(), xMargin, yMargin);
}