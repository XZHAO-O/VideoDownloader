#pragma once

#include <QObject>

#include "DownloadCardState.h"

class DownloadCard;
class DownloadCardModel;

class DownloadCardPool : public QObject
{
	Q_OBJECT

public:
	explicit DownloadCardPool(QObject* parent = nullptr);
	~DownloadCardPool();

	// 获取卡片（从池中获取可用的或创建新的）
	DownloadCard* getCard(QSharedPointer<DownloadCardModel> model, QWidget* parent = nullptr);

	// 释放卡片到池中
	void releaseCard(DownloadCard* card);

	// 清理池
	void clearPool();

	// 获取池中卡片数量
	int getPoolSize(DownloadCardState state) const;

private:
	// 三个池分别对应三种状态
	QList<DownloadCard*> m_pendingPool;      // 待下载卡片池
	QList<DownloadCard*> m_downloadingPool;  // 下载中卡片池  
	QList<DownloadCard*> m_downloadedPool;   // 已下载卡片池

	const int MAX_POOL_SIZE = 10; // 每个池的最大容量
};