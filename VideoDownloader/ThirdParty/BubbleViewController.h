#pragma once

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "UserInfoBubble.h"

class BubbleViewController : public QGraphicsView
{
	Q_OBJECT

public:
	BubbleViewController(QWidget* parent);
	~BubbleViewController();
	void showAnimated(QPoint globalPos);
	void hideAnimated();
signals:
	void requestHide();
	void exitLogin(bool loginState);
private:
	QGraphicsScene* scene = nullptr;
	QGraphicsProxyWidget* proxy = nullptr;
	QPropertyAnimation* scaleAnim = nullptr;
	QPropertyAnimation* opacityAnim = nullptr;
	QParallelAnimationGroup* groupAnim = nullptr;

	UserInfoBubble* bubble = nullptr;
	QSize m_bubbleSize;
};
