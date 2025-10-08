#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVector>
#include "AntTabWidget.h"

class AntTabBarWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AntTabBarWidget(int tabwidth, int tabHeight, bool isEnableAddBtn, QWidget* parent = nullptr);

	void addTab(const QString& title, const QString& icon);

	int currentIndex() const;
	void setCurrentIndex(int index);
signals:
	void tabClicked(int index);
	void currentIndexChanged(int index);  // 当前 tab 改变时发射此信号
	void removeContent(int index, int currentIndex);
	void addTabAndContent(QString title);
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
private:
	QVector<AntTabWidget*> m_tabs;
	AntTabWidget* m_draggingTab = nullptr;
	int m_currentIndex = -1;   // 当前选中的 tab index
	int m_draggingIndex = -1;
	int m_tabWidth;
	int m_tabHeight;
	const int m_spacing = 2;
	const int m_startX = 8;	// 标签的起始x
	const int m_startY = 4;	// 按钮的起始y
	QPoint m_dragOffset;
	QPoint m_dragStartPos;
	QPushButton* m_addButton = nullptr;
	bool m_isEnableAddBtn;

	void updateTabPositions(AntTabWidget* excludeDragTab = nullptr);
	void animateTabToPosition(AntTabWidget* tab, int index);
	void raiseTab(AntTabWidget* tab);
	void updateAddButtonPositionAnimated();
};
