#pragma once

#include <QTabBar>
#include <QPropertyAnimation>

class MaterialTabBar : public QTabBar
{
	Q_OBJECT
		Q_PROPERTY(int indicatorPos READ indicatorPos WRITE setIndicatorPos)

public:
	explicit MaterialTabBar(QWidget* parent = nullptr);
	~MaterialTabBar();

	int indicatorPos() const { return m_indicatorPosX; }
	void setIndicatorPos(int pos);

	QSize tabSizeHint(int index) const override;

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void updateIndicatorPosition(int index);

private:
	int m_indicatorPosX = 0;
	int m_indicatorWidth = 0;
	QPropertyAnimation* m_animation = nullptr;
};