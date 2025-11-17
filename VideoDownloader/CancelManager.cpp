#include "CancelManager.h"

#include <QDateTime>

std::atomic<int> CancelManager::m_tokenCounter(0);

CancelManager& CancelManager::instance()
{
	static CancelManager instance;
	return instance;
}

QString CancelManager::createCancelToken(const QString& operationType)
{
	QMutexLocker locker(&m_mutex);
	QString token = QString("%1_%2_%3")
		.arg(operationType)
		.arg(QDateTime::currentMSecsSinceEpoch())
		.arg(m_tokenCounter.fetch_add(1));

	m_cancelFlags[token] = std::make_shared<std::atomic<bool>>(false);
	m_activeTokens.insert(token);

	return token;
}

void CancelManager::cancelOperation(const QString& token)
{
	// 先检查令牌是否存在并设置取消状态
	bool shouldEmit = false;
	{
		QMutexLocker locker(&m_mutex);
		auto it = m_cancelFlags.find(token);
		if (it != m_cancelFlags.end())
		{
			it.value()->store(true);
			shouldEmit = true;
		}
	}

	// 在锁外发射信号，避免死锁和阻塞
	if (shouldEmit)
	{
		emit operationCancelled(token);
	}
}

void CancelManager::cancelAll()
{
	// 先收集所有需要取消的令牌
	QList<QString> tokensToCancel;
	{
		QMutexLocker locker(&m_mutex);
		for (auto it = m_cancelFlags.begin(); it != m_cancelFlags.end(); ++it)
		{
			it.value()->store(true);
			tokensToCancel.append(it.key());
		}
	}

	// 在锁外发射所有取消信号
	for (const QString& token : tokensToCancel)
	{
		emit operationCancelled(token);
	}
}

bool CancelManager::isCancelled(const QString& token) const
{
	QMutexLocker locker(&m_mutex);
	auto it = m_cancelFlags.find(token);
	if (it != m_cancelFlags.end())
	{
		return it.value()->load();
	}
	return false;
}

void CancelManager::cleanupToken(const QString& token)
{
	QMutexLocker locker(&m_mutex);
	m_cancelFlags.remove(token);
	m_activeTokens.remove(token);
}