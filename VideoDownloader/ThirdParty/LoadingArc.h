#ifndef LOADINGARC_H
#define LOADINGARC_H

#include <QWidget>
#include <QPropertyAnimation>

class LoadingArc : public QWidget {
	Q_OBJECT
		Q_PROPERTY(qreal rotationAngle READ rotationAngle WRITE setRotationAngle)

public:
	explicit LoadingArc(QWidget* parent = nullptr);
	void start();
	void stop();

	qreal rotationAngle() const { return m_rotationAngle; }
	void setRotationAngle(qreal angle);

protected:
	void paintEvent(QPaintEvent* event) override;
private:
	qreal m_rotationAngle = 0;
	QPropertyAnimation* m_animation = nullptr;
	QColor arcColor;
};

#endif // LOADINGARC_H
