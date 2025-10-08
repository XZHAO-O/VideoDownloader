#pragma once

#include <QWidget>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QGraphicsView>
#include <QGraphicsVideoItem>
#include <QMediaPlayer>
#include <QPushButton>
#include <QGraphicsScene>
#include <QOpenGLWidget>
#include <QTimer>
#include "GlassWidget.h"

class FramelessVideoWindow : public QWidget
{
	Q_OBJECT

public:
	FramelessVideoWindow(const QString& videoPath, QWidget* parent = nullptr);
	~FramelessVideoWindow();
protected:
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
private:
	bool isVirtualGPU();
private:
	QRect m_rect;
	QGraphicsScene* m_scene;
	QGraphicsView* m_view;
	QGraphicsVideoItem* m_videoItem;
	QMediaPlayer* m_player;
	QPushButton* m_button;
	QTimer* m_blurTimer; // 用于防抖定时器
	GlassWidget* glassWidget1;
	GlassWidget* glassWidget2;
};
