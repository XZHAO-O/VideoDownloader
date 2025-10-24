#include "DownloadCardPool.h"

#include "DownloadCard.h"
#include "DownloadCardModel.h"

DownloadCardPool::DownloadCardPool(QObject* parent)
	: QObject(parent)
{
}

DownloadCardPool::~DownloadCardPool()
{
	clearPool();
}

DownloadCard* DownloadCardPool::getCard(QSharedPointer<DownloadCardModel> model, QWidget* parent)
{
	if (!model) return nullptr;

	DownloadCard* card = nullptr;
	QList<DownloadCard*>* targetPool = nullptr;

	// 根据模型状态选择对应的池
	switch (model->state()) {
	case DownloadCardState::Pending:
		targetPool = &m_pendingPool;
		break;
	case DownloadCardState::Downloading:
		targetPool = &m_downloadingPool;
		break;
	case DownloadCardState::Downloaded:
		targetPool = &m_downloadedPool;
		break;
	default:
		targetPool = &m_pendingPool;
		break;
	}

	// 如果池中有可用卡片，从池中取出
	if (!targetPool->isEmpty())
	{
		card = targetPool->takeFirst();
		// 设置模型和父控件
		card->setModel(model);
		card->setParent(parent);
	}
	else
	{
		// 池为空，创建新卡片
		card = new DownloadCard(model, parent);
	}
	card->show(); // 显示卡片

	return card;
}

void DownloadCardPool::releaseCard(DownloadCard* card)
{
	if (!card) return;

	// 断开所有连接
	card->disconnect();

	// 根据卡片状态放入对应的池
	if (card->model()) {
		QList<DownloadCard*>* targetPool = nullptr;

		switch (card->model()->state()) {
		case DownloadCardState::Pending:
			targetPool = &m_pendingPool;
			break;
		case DownloadCardState::Downloading:
			targetPool = &m_downloadingPool;
			break;
		case DownloadCardState::Downloaded:
			targetPool = &m_downloadedPool;
			break;
		default:
			targetPool = &m_pendingPool;
			break;
		}

		// 如果池未满，放入池中；否则删除卡片
		if (targetPool->size() < MAX_POOL_SIZE) {
			card->setParent(nullptr); // 从父控件中移除
			card->hide(); // 隐藏卡片
			targetPool->append(card);
		}
		else {
			delete card;
		}
	}
	else {
		delete card;
	}
}

void DownloadCardPool::clearPool()
{
	// 清理所有池中的卡片
	for (auto card : m_pendingPool) {
		delete card;
	}
	m_pendingPool.clear();

	for (auto card : m_downloadingPool) {
		delete card;
	}
	m_downloadingPool.clear();

	for (auto card : m_downloadedPool) {
		delete card;
	}
	m_downloadedPool.clear();
}

int DownloadCardPool::getPoolSize(DownloadCardState state) const
{
	switch (state) {
	case DownloadCardState::Pending:
		return m_pendingPool.size();
	case DownloadCardState::Downloading:
		return m_downloadingPool.size();
	case DownloadCardState::Downloaded:
		return m_downloadedPool.size();
	default:
		return 0;
	}
}