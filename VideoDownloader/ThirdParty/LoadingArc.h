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
	void setRotationSpeed(int degreesPerSecond);

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;

private slots:
	void updateArc();

private:
	void updateCachedObjects();
	void updateBuffer(); // 新增：更新缓冲区

	QTimer* m_timer = nullptr;
	QColor m_arcColor;

	// 旋转参数
	int m_rotationSpeed = 480;
	qreal m_currentAngle = 0.0;

	// 绘制参数
	QRectF m_cachedRect;
	int m_cachedThickness = 0;
	QPen m_cachedPen;
	bool m_needUpdatePen = true;

	// 缓存
	QPixmap m_buffer; // 缓存圆弧的Pixmap
	bool m_bufferDirty = true; // 标记缓冲区是否需要更新
};

#endif // LOADINGARC_H