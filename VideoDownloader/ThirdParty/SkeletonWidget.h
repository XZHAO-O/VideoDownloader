#ifndef SKELETONWIDGET_H
#define SKELETONWIDGET_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>

class SkeletonWidget : public QWidget
{
	Q_OBJECT
		Q_PROPERTY(int gradientX READ gradientX WRITE setGradientX)

public:
	explicit SkeletonWidget(int rectRadius, QWidget* parent = nullptr);
	int gradientX() const { return m_gradientX; }
	void setGradientX(int x);
public:
	// 加载完成后调用
	void stopSkeleton();
	// 启动骨架屏动画
	void startSkeleton();
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	int m_gradientX = 0;
	int m_rectRadius = 0; // 圆角半径
	QPropertyAnimation* m_animation = nullptr;
};

#endif // SKELETONWIDGET_H
