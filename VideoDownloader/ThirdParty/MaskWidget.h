#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>

class MaskWidget : public QWidget
{
	Q_OBJECT
		Q_PROPERTY(qreal m_alpha READ alpha WRITE setAlpha)
public:
	MaskWidget(int w, int h, QWidget* parent);
	~MaskWidget();

	qreal alpha() const { return m_alpha; }
	void setAlpha(qreal alpha) {
		m_alpha = alpha;
		update(); // 触发重绘
	}
	// 播放透明度动画
	void showAnim();
	void hideAnim();
protected:
	void mousePressEvent(QMouseEvent* e) override;
	void paintEvent(QPaintEvent*) override;
	void mouseReleaseEvent(QMouseEvent* e)
	{
		e->accept();
	}
	void mouseMoveEvent(QMouseEvent* e)
	{
		e->accept();
	}
	void keyPressEvent(QKeyEvent* e)
	{
		e->accept();
	}
	void keyReleaseEvent(QKeyEvent* e)
	{
		e->accept();
	}
signals:
	void clicked();
private:
	bool isHide = false;
	qreal m_alpha = 0.0;
	QPropertyAnimation* alphaAnim = nullptr;
};
