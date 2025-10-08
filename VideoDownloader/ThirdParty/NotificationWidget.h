#pragma once

#include <QWidget>
#include <QEvent>
#include <QLabel>
#include <QToolButton>

class NotificationWidget : public QWidget
{
	Q_OBJECT
public:
	NotificationWidget(const QString& title, QSize size, QWidget* parentWindow = nullptr);
	~NotificationWidget();

	QSize getSize() { return m_size; };

	void animateIn(const QPoint& startPos, const QPoint& endPos);
	void animateOut(const QPoint& endPos, std::function<void()> onFinished);
	void addTaskCount();
signals:
	void exitAnim();
private:
	static int taskCount;
	qreal customOpacity = 1.0;
	QSize m_size;
	QString m_text;
	QLabel* m_descLab;
	QWidget* bg;
	QToolButton* closeBtn;
};
