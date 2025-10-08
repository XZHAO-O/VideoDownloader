#include "NotificationManager.h"
#include <QTimer>
#include <QPropertyAnimation>

NotificationManager* NotificationManager::m_instance = nullptr;

NotificationManager::NotificationManager(QWidget* parent)
	: QWidget(parent)
{
}

void NotificationManager::showNotification(const QString& title, QSize size, int duration)
{
	if (!m_notifications.isEmpty())
	{
		m_notifications.first()->addTaskCount();
		return;
	}

	auto* notif = new NotificationWidget(title, size, m_parentWindow);
	QSize notifSize = notif->getSize();
	QSize parentSize = m_parentWindow->size();

	// 右下角基准点（局部坐标）
	QPoint basePos(parentSize.width() - notifSize.width() - margin, parentSize.height() - notifSize.height() - margin);

	// 新通知初始位置（在屏幕右边外）
	QPoint startPos = basePos + QPoint(notifSize.width() + margin, 0);
	QPoint endPos = basePos - QPoint(0, 0);

	// 插入列表 & 设置初始位置
	notif->move(startPos);
	notif->show();

	// 先播放新通知的进入动画
	notif->addTaskCount();
	notif->animateIn(startPos, endPos);

	m_notifications.append(notif);
	NotificationWidget* w = m_notifications.first();
	connect(notif, &NotificationWidget::exitAnim, this, [=]()
		{
			QPoint exitPos = notif->pos() + QPoint(notifSize.width() + margin, 0); // 向右滑出
			notif->animateOut(exitPos, [=]()
				{
					// 动画结束后删除并移除
					m_notifications.removeOne(notif);
					notif->deleteLater();
				});
		});
}

void NotificationManager::relayoutNotifications(int parentWidth, int parentHeight)
{
	if (m_notifications.isEmpty()) return;

	QSize notifSize = m_notifications.first()->getSize();

	QPoint basePos(parentWidth - notifSize.width() - margin, parentHeight - notifSize.height() - margin);

	for (int i = 0; i < m_notifications.size(); ++i)
	{
		NotificationWidget* notif = m_notifications[i];
		QPoint pos = basePos - QPoint(0, i * (notifSize.height() + spacing));
		notif->move(pos);  // 直接移动，无动画
	}
}