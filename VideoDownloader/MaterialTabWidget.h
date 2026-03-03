#pragma once

#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>

class MaterialTabBar;
class SlideStackedWidget;

class MaterialTabWidget : public QWidget
{
	Q_OBJECT

public:
	explicit MaterialTabWidget(QWidget* parent = nullptr);
	~MaterialTabWidget();

	void setCurrentIndex(int index);
	int currentIndex() const noexcept;

	int count() const;
	void removeTab(int index);

	void addTab(QWidget* wid, QString tabName);
	QWidget* getWidget(int index);
	QVBoxLayout* getLayout();

signals:
	void itemIndexChanged(int index);

protected:
	void wheelEvent(QWheelEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
	void onTabClicked(int index);
	void handleWheelChange();

private:
	void handleWheelEvent(QWheelEvent* event);
	void jumpToTargetIndex();

private:
	MaterialTabBar* m_tabBar;
	SlideStackedWidget* m_stackedWidget;
	QVBoxLayout* layout;
	int m_currentIndex = 0;
	bool m_isAnimating = false;
	int m_animationDuration = 300;

	// 快速滚动的成员变量
	QTimer* m_wheelTimer;
	int m_targetIndex = -1;
	qint64 m_lastWheelTime = 0;
	int m_wheelAccumulator = 0;
	bool m_rapidScrolling = false;
};