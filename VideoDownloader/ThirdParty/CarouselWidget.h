#pragma once

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "CarouselCard.h"

class CarouselWidget : public QWidget
{
	Q_OBJECT

public:
	// 最少 4 张，最多 7 张 不然无法正常运行
	CarouselWidget(QSize size, int showCardNums, QStringList imagePaths, QWidget* parent = nullptr);
	void showNext();
	QList<CarouselCard*> getCards() { return cards; };
private:
	void setupSizes();
	void setupInitialLayout();
	void carouselAnimation(int startIndex);
	void addCardAnimation(QParallelAnimationGroup* group, CarouselCard* card, const QRect& startRect, const QRect& endRect);
private:
	struct CardInfo
	{
		QPoint pos;
		QSize size;
	};
	QTimer* m_timer;
	QTimer* resizeTimer;
	QList<CardInfo> layoutTemplate;
	QList<CarouselCard*> cards;
	int showCards;				// 至少显示4个
	int currentIndex = 0;		// 当前卡片索引
	int expandedWidth = 0;		// 展开卡片宽度
	int shrinkWidth = 0;		// 收缩卡片宽度
	int cardHeight = 0;			// 卡片高度
	int spacing = 30;			// 卡片间距
	int currentStartIndex = 0;
	bool animRunning = false;
};
