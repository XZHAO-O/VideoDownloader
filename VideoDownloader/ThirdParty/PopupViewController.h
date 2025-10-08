#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsScale>
#include "PopupWidget.h"

class PopupViewController : public QGraphicsView
{
	Q_OBJECT

public:
	enum  AnchorPoint
	{
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight
	};

	PopupViewController(int height, bool enableMultiLevel, QWidget* parent);
	~PopupViewController();
	void showAnimated(const QPoint& pos, int width);
	void hideAnimated();
	bool getVisible() { return m_isVisible; }
	void updateSize(int width, int height);
	void follow(QWidget* anchorWidget, AnchorPoint anchor = AnchorPoint::BottomLeft);
signals:
	void itemSelected(const QModelIndex& index);
public:
	PopupWidget* popup = nullptr;
private:
	bool m_isVisible;
	int m_height;
	QGraphicsScene* scene = nullptr;
	QGraphicsProxyWidget* proxy = nullptr;
	QPropertyAnimation* scaleAnim = nullptr;
	QPropertyAnimation* opacityAnim = nullptr;
	QParallelAnimationGroup* groupAnim = nullptr;
	QGraphicsScale* scaleTransform = nullptr;
};
