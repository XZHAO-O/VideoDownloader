#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include "AntButton.h"

class UserInfoBubble : public QWidget
{
	Q_OBJECT
public:
	UserInfoBubble(QWidget* parent);
	~UserInfoBubble();
	void showAnimated(QPoint globalPos);
	void hideAnimated();
protected:
	void paintEvent(QPaintEvent*) override;
	void leaveEvent(QEvent* e) override;
signals:
	void requestHide();
	void exitLogin(bool loginState);
private:
	int margin = 8;	// 气泡框(圆角矩形)的外边距
	int spacing = 14;
	// 美观参数配置（基于13px基准调整）
	const int arrowHeight = 14;		// 箭头高度（视觉舒适高度）
	const int arrowWidth = 7;		// 箭头宽度
	const int arrowOffsetY = 16;    // 从顶部向下偏移（避免贴边）
	// 视觉动画 (透明度+缩放)
	
	// 鼠标是否悬停在头像上
	bool isHoverAvatar = false;
	// ant按钮
	AntButton* m_antBtn = nullptr;
};
