#ifndef LOADINGARC_H
#define LOADINGARC_H

#include <QWidget>
#include <QTimer>
#include <QPen>

class LoadingArc : public QWidget
{
	Q_OBJECT

public:
	explicit LoadingArc(QWidget* parent = nullptr);
	~LoadingArc();

	void start();
	void stop();
	void setUpdateInterval(int ms);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void updateArc();
	void updateCachedObjects();

	QTimer* m_timer = nullptr;
	QColor m_arcColor;

	// 帧动画相关
	int m_currentFrame = 0;
	static const int m_totalFrames = 30;  // 总帧数
	static const int m_angleStep = 12;   // 每帧旋转角度

	// 绘制参数
	QRectF m_cachedRect;
	int m_cachedThickness = 0;
	QPen m_cachedPen;
	bool m_needUpdatePen = true;
};

#endif // LOADINGARC_H