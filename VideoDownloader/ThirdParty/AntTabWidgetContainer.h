#pragma once

#include <QWidget>
#include <QStackedWidget>
#include "AntTabBarWidget.h"

class AntTabWidgetContainer : public QWidget
{
	Q_OBJECT

public:
	explicit AntTabWidgetContainer(int tabwidth, int tabHeight, bool isEnableAddBtn = true, QWidget* parent = nullptr);

	void addTab(const QString& title, const QString& icon, QWidget* contentWidget);

	// 获取当前选中的 tab index
	int currentIndex() const;
	void setCurrentIndex(int index);

protected:
	void resizeEvent(QResizeEvent* event) override;
private slots:
	void onTabClicked(int index);

private:
	AntTabBarWidget* m_tabBar;         // TabBar 控件
	QStackedWidget* m_contentWidget;   // 用于存放内容区域的 QStackedWidget
	QVector<QWidget*> m_tabsContent;   // 存放内容部件的 QVector
};
