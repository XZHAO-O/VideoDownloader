#pragma once

#include <QObject>
#include <QMutex>
#include <QSet>
#include <atomic>

class CancelManager : public QObject
{
	Q_OBJECT

public:
	static CancelManager& instance();

	// 为特定操作创建取消令牌
	QString createCancelToken(const QString& operationType = "default");

	// 取消特定操作
	void cancelOperation(const QString& token);

	// 取消所有操作
	void cancelAll();

	// 检查是否已取消
	bool isCancelled(const QString& token) const;

	// 清理已完成的令牌
	void cleanupToken(const QString& token);

signals:
	void operationCancelled(const QString& token);

private:
	CancelManager() = default;

	mutable QMutex m_mutex;
	QHash<QString, std::shared_ptr<std::atomic<bool>>> m_cancelFlags;
	QSet<QString> m_activeTokens;
	static std::atomic<int> m_tokenCounter;
};