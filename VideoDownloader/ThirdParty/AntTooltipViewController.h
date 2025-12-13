#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "AntTooltip.h"

class AntTooltipViewController : public QGraphicsView
{
	Q_OBJECT

public:
	AntTooltipViewController(QString text, AntTooltip::ArrowDir dir, QWidget* parent);
	~AntTooltipViewController();

	void showAnimated(QPoint globalPos);
	void hideAnimated();

signals:
	// 隐藏完成信号
	void hidden();

private:
	void updateTransformOrigin();

public:
	AntTooltip* tooltip = nullptr;

private:
	QGraphicsScene* scene = nullptr;
	QGraphicsProxyWidget* proxy = nullptr;
	QPropertyAnimation* scaleAnim = nullptr;
	QPropertyAnimation* opacityAnim = nullptr;
	QParallelAnimationGroup* groupAnim = nullptr;
	bool isHide = false;
};