#pragma once

#include <QWidget>
#include <QList>
#include "NotificationWidget.h"

class NotificationManager : public QWidget
{
	Q_OBJECT

public:
	static NotificationManager* instance()
	{
		if (!m_instance)
		{
			m_instance = new NotificationManager();
		}
		return m_instance;
	}

	void getMainWindow(QWidget* parentWindow)
	{
		m_parentWindow = parentWindow;
	}

	// 显示通知
	void showNotification(const QString& title, QSize size = QSize(360, 220), int duration = 2500);
	// 重新布局通知
	void relayoutNotifications(int parentWidth, int parentHeight);
private:
	NotificationManager(QWidget* parent = nullptr);

	static NotificationManager* m_instance;
	QList<NotificationWidget*> m_notifications;
	QWidget* m_parentWindow = nullptr;
	int margin = 10;
	int spacing = 2;	// 高度间距
};
