#include "AntMessageManager.h"

#include <QApplication>

#include "DesignSystem.h"

AntMessageManager* AntMessageManager::m_instance = nullptr;

AntMessageManager::AntMessageManager(QWidget* parent)
	: QWidget(parent)
{
}

AntMessageManager::~AntMessageManager()
{
	// 确保在析构时清理所有消息
	clearAllMessages();
}

QRect AntMessageManager::getScreenGeometry() const
{
	// Qt5: QDesktopWidget；Qt6: QScreen
	#ifdef QT_VERSION_MAJOR
	#if QT_VERSION_MAJOR >= 6
	QScreen* screen = QGuiApplication::primaryScreen();
	return screen->geometry();
	#else
	QDesktopWidget* desktop = QApplication::desktop();
	return desktop->screenGeometry(desktop->primaryScreen());
	#endif
	#else
	QDesktopWidget* desktop = QApplication::desktop();
	return desktop->screenGeometry(desktop->primaryScreen());
	#endif
}

QPoint AntMessageManager::getSingletonPosition(AntMessage* msg)
{
	QWidget* mainWindow = DesignSystem::instance()->getMainWindow();
	int x = (mainWindow->width() - msg->width()) / 2;
	int y = m_singletonOffsetY;
	return QPoint(x, y);
}

void AntMessageManager::clearAllMessages()
{
	// 先停止所有动画
	isAnimating = false;
	m_isBatchAnimating = false;

	// 清理所有消息
	for (AntMessage* msg : m_messages) {
		// 先断开所有连接，防止信号触发
		msg->disconnect();
		// 停止定时器
		if (msg->timer && msg->timer->isActive()) {
			msg->timer->stop();
		}
		// 立即删除而不是deleteLater，避免悬空指针
		delete msg;
	}
	m_messages.clear();
}

AntMessageManager* AntMessageManager::instance()
{
	if (!m_instance)
	{
		m_instance = new AntMessageManager();
	}
	return m_instance;
}

void AntMessageManager::showMessage(AntMessage::Type type, const QString& message, int msgDuration)
{
	showMessage(type, AntMessage::Queue, message, msgDuration);
}

void AntMessageManager::showMessage(AntMessage::Type type, AntMessage::Mode mode, const QString& message, int msgDuration)
{
	QWidget* mainWindow = DesignSystem::instance()->getMainWindow();

	// 单例模式：移除所有现有消息
	if (mode == AntMessage::Singleton)
		clearAllMessages();

	AntMessage* msg = new AntMessage(mainWindow, type, message);
	msgHeight = msg->height();

	// 连接信号
	connect(msg, &AntMessage::requestExit, this, &AntMessageManager::onMessageRequestExit);

	if (mode == AntMessage::Singleton)
	{
		// 单例模式：固定位置
		QPoint pos = getSingletonPosition(msg);
		msg->move(pos);

		// 动画 - 淡入效果
		QPropertyAnimation* opacityAnim = new QPropertyAnimation(msg, "customOpacity", msg);
		opacityAnim->setDuration(animDuration);
		opacityAnim->setEasingCurve(QEasingCurve::OutSine);
		opacityAnim->setStartValue(0.0);
		opacityAnim->setEndValue(1.0);

		m_messages.append(msg);
		msg->show();
		opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
		msg->startDisplayTimer(msgDuration);

	}
	else
	{
		// 队列模式
		int y = spacingY + m_singletonOffsetY;

		for (AntMessage* m : m_messages)
			y += msgHeight + spacingY;

		int x = (mainWindow->width() - msg->width()) / 2;

		QPropertyAnimation* opacityAnim = new QPropertyAnimation(msg, "customOpacity", msg);
		opacityAnim->setDuration(animDuration);
		opacityAnim->setEasingCurve(QEasingCurve::OutSine);
		opacityAnim->setStartValue(0.0);
		opacityAnim->setEndValue(1.0);

		if (m_isBatchAnimating)
			msg->move(x, y - spacingY - msg->height());
		else
			msg->move(QPoint(x, y));

		m_messages.append(msg);
		msg->show();
		opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
		msg->startDisplayTimer(msgDuration);
	}
}

void AntMessageManager::onMessageRequestExit(AntMessage* msg)
{
	if (isAnimating)
		return;

	if (!m_messages.isEmpty() && msg == m_messages.first())
	{
		startExitAnimation();
	}
}

void AntMessageManager::startExitAnimation()
{
	if (m_messages.isEmpty()) return;

	isAnimating = true;
	AntMessage* firstMsg = m_messages.first();
	firstMsg->setIsExit(true);

	// 创建退出动画组，将父对象设置为firstMsg确保自动清理
	QParallelAnimationGroup* slideOutGroup = new QParallelAnimationGroup(firstMsg);

	// 首条消息退出动画
	QPropertyAnimation* slideOutAnim = new QPropertyAnimation(firstMsg, "pos", slideOutGroup);
	QPropertyAnimation* opacityAnim = new QPropertyAnimation(firstMsg, "customOpacity", slideOutGroup);

	slideOutAnim->setDuration(animDuration - 150);
	slideOutAnim->setEasingCurve(QEasingCurve::InOutSine);
	slideOutAnim->setStartValue(firstMsg->pos());
	slideOutAnim->setEndValue(QPoint(firstMsg->x(), firstMsg->y() - 10));

	opacityAnim->setDuration(animDuration - 150);
	opacityAnim->setEasingCurve(QEasingCurve::InOutSine);
	opacityAnim->setStartValue(1.0);
	opacityAnim->setEndValue(0.0);

	slideOutGroup->addAnimation(slideOutAnim);
	slideOutGroup->addAnimation(opacityAnim);

	// 队列模式下，后续消息上移
	if (m_messages.count() > 1) {
		int baseY = spacingY + m_singletonOffsetY;
		int heightWithSpacing = msgHeight + spacingY;

		for (int i = 1; i < m_messages.count(); ++i) {
			AntMessage* msg = m_messages[i];
			int x = (DesignSystem::instance()->getMainWindow()->width() - msg->width()) / 2;
			QPropertyAnimation* anim = new QPropertyAnimation(msg, "pos", slideOutGroup);
			anim->setDuration(animDuration - 100);
			anim->setEasingCurve(QEasingCurve::InOutSine);
			anim->setStartValue(QPoint(x, msg->pos().y()));
			anim->setEndValue(QPoint(x, baseY + (i - 1) * heightWithSpacing));
			slideOutGroup->addAnimation(anim);
		}
	}

	// 动画结束后清理
	connect(slideOutGroup, &QParallelAnimationGroup::finished, this, [this, firstMsg]() {
		// 从列表中移除
		if (!m_messages.isEmpty() && m_messages.first() == firstMsg) {
			m_messages.removeFirst();
		}

		// 删除消息对象
		firstMsg->deleteLater();

		isAnimating = false;
		m_isBatchAnimating = false;

		// 检查下一个消息是否需要退出
		if (!m_messages.isEmpty()) {
			AntMessage* newFirst = m_messages.first();
			if (newFirst->hasTimeoutOccurred()) {
				startExitAnimation();
			}
		}
		});

	m_isBatchAnimating = true;
	slideOutGroup->start(QAbstractAnimation::DeleteWhenStopped);
}

void AntMessageManager::onMessageExitFinished(AntMessage* msg)
{
	// 这个信号可能不再需要，因为我们在动画组中处理了清理
}