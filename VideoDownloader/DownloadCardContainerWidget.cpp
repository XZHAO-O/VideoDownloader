#include "DownloadCardContainerWidget.h"

#include <QNetworkReply>
#include <QDir>
#include <QDesktopServices>
#include <QApplication>

#include "MaterialSpinner.h"
#include "PaginationWidget.h"
#include "AntScrollArea.h"
#include "NoDataWidget.h"
#include "DesignSystem.h"
#include "AntButton.h"

#include "DownloadTaskInfo.h"
#include "DownloadCard.h"
#include "DownloadEngine.h"
#include "Instrumentor.h"

DownloadCardContainerWidget::DownloadCardContainerWidget(QSharedPointer<DownloadEngine> downloadEngine,
	ContainerState state,
	QWidget* parent)
	: QWidget(parent)
	, m_downloadEngine(downloadEngine)
	, m_containerState(state)
	, m_mainLayout(nullptr)
	, m_scrollArea(nullptr)
	, m_scrollWidget(nullptr)
	, m_scrollLayout(nullptr)
	, m_noDataWidget(nullptr)
	, m_paginationWidget(nullptr)
	, m_spinner(nullptr)
	, m_buttonBar(nullptr)
	, m_buttonLayout(nullptr)
	, m_batchDownloadBtn(nullptr)
	, m_batchDeleteBtn(nullptr)
	, m_batchPauseBtn(nullptr)
	, m_batchResumeBtn(nullptr)
{
	BENCHMARKING_FUNCTION();

	DownloadCardState cardState{};

	switch (m_containerState)
	{
	case ContainerState::DownloadReady:
		m_noDataText = tr("暂无待下载任务");
		cardState = DownloadCardState::Pending;
		break;
	case ContainerState::Downloading:
		m_noDataText = tr("暂无下载任务");
		cardState = DownloadCardState::Downloading;
		// 连接下载管理器信号
		connect(m_downloadEngine.get(), &DownloadEngine::downloadProgress, this, &DownloadCardContainerWidget::onDownloadProgress);
		connect(m_downloadEngine.get(), &DownloadEngine::downloadFinished, this, &DownloadCardContainerWidget::onDownloadCompleted);
		break;
	case ContainerState::Downloaded:
		m_noDataText = tr("暂无已下载任务");
		cardState = DownloadCardState::Downloaded;
		break;
	}

	// 预先创建固定数量的卡片并隐藏
	for (int i = 0; i < m_pageSize; ++i)
	{
		DownloadCard* card = new DownloadCard(cardState, this);
		card->setVisible(false);
		m_precreatedCards.append(card);
	}

	initUI();
}

DownloadCardContainerWidget::~DownloadCardContainerWidget()
{
	// 清理所有卡片
	clearAllCards();

	// 清理按钮
	delete m_batchDownloadBtn;
	delete m_batchDeleteBtn;
	delete m_batchPauseBtn;
	delete m_batchResumeBtn;
	delete m_buttonLayout;
	delete m_buttonBar;
}

void DownloadCardContainerWidget::initUI()
{
	BENCHMARKING_FUNCTION();
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 4, 0, 0);
	m_mainLayout->setSpacing(0);

	// 初始化按钮行
	initButtonBar();

	// 创建加载指示器
	m_spinner = new MaterialSpinner(QSize(40, 40), 4, DesignSystem::instance()->primaryColor(), this);
	m_spinner->setVisible(false);  // 初始隐藏
	m_spinner->setFixedSize(60, 60);  // 设置固定大小
	m_spinner->setStyleSheet("background-color: rgba(255, 255, 255, 200); border-radius: 10px;");  // 半透明背景

	// 创建滚动区域
	m_scrollArea = new AntScrollArea(AntScrollArea::ScrollVertical, this);
	m_scrollWidget = new QWidget(this);
	m_scrollLayout = new QVBoxLayout(m_scrollWidget);
	m_scrollLayout->setContentsMargins(16, 16, 16, 16);
	m_scrollLayout->setSpacing(12);
	m_scrollLayout->setAlignment(Qt::AlignTop);

	for (int i = 0; i < m_pageSize; ++i)
		m_scrollLayout->addWidget(m_precreatedCards[i]);

	// 添加一个弹簧，让内容从顶部开始
	m_scrollLayout->addStretch();

	m_scrollArea->addWidget(m_scrollWidget);
	m_mainLayout->addWidget(m_scrollArea);

	// 创建暂无数据组件
	m_noDataWidget = new NoDataWidget(this);
	m_noDataWidget->setText(m_noDataText);
	m_noDataWidget->hide();
	m_mainLayout->addWidget(m_noDataWidget);

	// 创建分页器
	m_paginationWidget = new PaginationWidget(QSize(35, 35), this);
	m_paginationWidget->setFixedHeight(50);
	m_paginationWidget->setTotalPages(1);
	m_paginationWidget->setCurrentPage(1);
	m_paginationWidget->setVisible(false);

	// 连接分页信号
	connect(m_paginationWidget, &PaginationWidget::currentPageChanged,
		this, &DownloadCardContainerWidget::onPageChanged, Qt::DirectConnection);

	m_mainLayout->addWidget(m_paginationWidget);

	updateVisibility();
}

void DownloadCardContainerWidget::initButtonBar()
{
	// 创建按钮行容器
	m_buttonBar = new QWidget(this);
	m_buttonBar->setFixedHeight(50);
	m_buttonLayout = new QHBoxLayout(m_buttonBar);
	m_buttonLayout->setContentsMargins(16, 8, 16, 8);
	m_buttonLayout->setSpacing(12);

	m_batchDeleteBtn = new AntButton(tr("全部删除"), 10, this);
	m_batchDeleteBtn->setButtonColor(DesignSystem::instance()->dangerColor());
	m_batchDeleteBtn->setFixedSize(120, 45);
	m_batchDeleteBtn->setIconKey("trash");
	connect(m_batchDeleteBtn, &AntButton::clicked, this, &DownloadCardContainerWidget::onBatchDeleteClicked, Qt::DirectConnection);

	// 根据容器状态创建不同的按钮
	switch (m_containerState)
	{
	case ContainerState::DownloadReady:
		// 批量下载按钮
		m_batchDownloadBtn = new AntButton(tr("批量下载"), 10, this);
		m_batchDownloadBtn->setFixedSize(120, 45);
		m_batchDownloadBtn->setIconKey("download");
		m_batchDownloadBtn->setToolTip(tr("自定义批量下载任务"));
		connect(m_batchDownloadBtn, &AntButton::clicked, this, &DownloadCardContainerWidget::onBatchDownloadClicked, Qt::DirectConnection);

		// 全部删除按钮
		m_batchDeleteBtn->setToolTip(tr("删除当前待下载任务"));

		m_buttonLayout->addWidget(m_batchDownloadBtn);
		m_buttonLayout->addWidget(m_batchDeleteBtn);
		break;

	case ContainerState::Downloading:
		// 全部开始按钮
		m_batchResumeBtn = new AntButton(tr("全部开始"), 10, this);
		m_batchResumeBtn->setFixedSize(120, 45);
		m_batchResumeBtn->setIconKey("play-fill");
		m_batchResumeBtn->setToolTip(tr("开始当前下载任务"));
		connect(m_batchResumeBtn, &AntButton::clicked, this, &DownloadCardContainerWidget::onBatchResumeClicked, Qt::DirectConnection);

		// 全部暂停按钮
		m_batchPauseBtn = new AntButton(tr("全部暂停"), 10, this);
		m_batchPauseBtn->setFixedSize(120, 45);
		m_batchPauseBtn->setIconKey("pause-fill");
		m_batchPauseBtn->setToolTip(tr("暂停当前下载任务"));
		connect(m_batchPauseBtn, &AntButton::clicked, this, &DownloadCardContainerWidget::onBatchPauseClicked, Qt::DirectConnection);

		// 全部删除按钮
		m_batchDeleteBtn->setToolTip(tr("删除当前下载任务"));

		m_buttonLayout->addWidget(m_batchResumeBtn);
		m_buttonLayout->addWidget(m_batchPauseBtn);
		m_buttonLayout->addWidget(m_batchDeleteBtn);
		break;

	case ContainerState::Downloaded:
		// 全部删除按钮
		m_batchDeleteBtn->setToolTip(tr("删除当前下载记录"));

		m_buttonLayout->addWidget(m_batchDeleteBtn);
		break;
	}

	// 添加弹簧，让按钮靠左对齐
	m_buttonLayout->addStretch();

	// 将按钮行添加到主布局的最上方
	m_mainLayout->insertWidget(0, m_buttonBar);
	m_buttonBar->setVisible(false);  // 初始隐藏
}

void DownloadCardContainerWidget::onPageChanged(int page)
{
	m_currentPage = page;
	updateCurrentPageCards();
}

void DownloadCardContainerWidget::updateCurrentPageCards()
{
	BENCHMARKING_FUNCTION();
	// 计算当前页的任务范围
	int startIndex = (m_currentPage - 1) * m_pageSize;
	int endIndex = qMin(startIndex + m_pageSize, static_cast<int>(m_downloadTasks.size()));
	int currentPageTaskCount = endIndex - startIndex;

	// 清空当前显示的卡片映射
	clearCurrentCards();

	// 更新预先创建卡片的显示和数据
	int cardIndex = 0;
	auto it = m_downloadTasks.begin();
	std::advance(it, startIndex); // 移动到当前页起始位置

	for (int i = 0; i < m_precreatedCards.size(); ++i)
	{
		DownloadCard* card = m_precreatedCards[i];

		if (i < currentPageTaskCount && it != m_downloadTasks.end())
		{
			// 显示卡片并设置数据
			auto taskInfo = *it;
			card->updateFromTaskInfo(taskInfo);
			card->setVisible(true);

			// 重新设置连接
			setupCardConnections(card, taskInfo);

			// 添加到当前显示的映射
			m_downloadCards.insert(taskInfo->taskId, card);

			++it;
			++cardIndex;
		}
		else
		{
			card->disconnect(); // 断开连接
			// 隐藏多余的卡片
			card->setVisible(false);
		}
	}

	updateVisibility();
	update();
}

void DownloadCardContainerWidget::clearCurrentCards()
{
	BENCHMARKING_FUNCTION();
	// 断开所有连接并隐藏卡片
	for (auto card : m_precreatedCards)
	{
		card->disconnect();
		card->setVisible(false);
	}
	m_downloadCards.clear();
}

void DownloadCardContainerWidget::clearAllCards()
{
	BENCHMARKING_FUNCTION();
	// 清理所有卡片
	for (auto card : m_precreatedCards) {
		card->deleteLater();
	}
	m_precreatedCards.clear();
	m_downloadCards.clear();
}

void DownloadCardContainerWidget::setupCardConnections(DownloadCard* card, QSharedPointer<DownloadTaskInfo> taskInfo)
{
	BENCHMARKING_FUNCTION();

	card->disconnect();

	switch (m_containerState)
	{
	case ContainerState::DownloadReady:
		// 视频质量改变
		connect(card, &DownloadCard::videoQualityChanged,
			this, [this, taskInfo, card](const QString& quality) {
				taskInfo->selectedVideoQuality = quality;
				card->setCurrentVideoQuality(quality);
				card->updateFileSizes(taskInfo->videoStreamInfo[quality].fileSize, taskInfo->audioStreamInfo[taskInfo->selectedAudioQuality].fileSize);
			}, Qt::DirectConnection);

		// 音频质量改变
		connect(card, &DownloadCard::audioQualityChanged,
			this, [this, taskInfo, card](const QString& quality) {
				taskInfo->selectedAudioQuality = quality;
				card->setCurrentAudioQuality(quality);
				card->updateFileSizes(taskInfo->videoStreamInfo[taskInfo->selectedVideoQuality].fileSize, taskInfo->audioStreamInfo[quality].fileSize);
			}, Qt::DirectConnection);

		// 下载按钮点击
		connect(card, &DownloadCard::downloadClicked,
			this, [this, taskInfo]() {
				taskInfo->downloadFormat = DownloadFormat::Separated;
				emit taskStateChanged(taskInfo->taskId, ContainerState::Downloading);
			}, Qt::DirectConnection);

		// 仅下载视频
		connect(card, &DownloadCard::videoDownloadClicked,
			this, [this, taskInfo]() {
				taskInfo->downloadFormat = DownloadFormat::VideoOnly;
				emit taskStateChanged(taskInfo->taskId, ContainerState::Downloading);
			}, Qt::DirectConnection);

		// 仅下载音频
		connect(card, &DownloadCard::audioDownloadClicked,
			this, [this, taskInfo]() {
				taskInfo->downloadFormat = DownloadFormat::AudioOnly;
				emit taskStateChanged(taskInfo->taskId, ContainerState::Downloading);
			}, Qt::DirectConnection);

		// 删除按钮点击
		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {
			card->disconnect();

			m_downloadTasks.remove(taskInfo->taskId);
			m_downloadCards.remove(taskInfo->taskId);

			// 隐藏卡片
			card->setVisible(false);

			// 更新分页器总页数
			int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);

			// 如果删除后当前页没有内容且不是第一页，则回到前一页
			if (m_downloadCards.isEmpty() && m_currentPage > 1) {
				m_currentPage--;
				m_paginationWidget->setCurrentPage(m_currentPage);
				updateCurrentPageCards(); // 需要重新加载整个页面
			}
			else {
				// 更新当前页显示
				updateCurrentPageCards();
			}
			}, Qt::DirectConnection);

		// 打开链接
		connect(card, &DownloadCard::openUrlClicked, this, [this, taskInfo]() {
			QDesktopServices::openUrl(taskInfo->videoInfo.url);
			}, Qt::DirectConnection);

		// 预览点击
		connect(card, &DownloadCard::previewClicked, this, [this, taskInfo]() {
			// 预览逻辑（如果需要容器处理）
			Q_UNUSED(taskInfo);
			}, Qt::DirectConnection);
		break;

	case ContainerState::Downloading:
		// 暂停按钮点击
		connect(card, &DownloadCard::pauseClicked, this, [this, taskInfo]() {
			auto downloadEngine = m_downloadEngine.get();
			QMetaObject::invokeMethod(downloadEngine, [this, downloadEngine, taskInfo]() {
				downloadEngine->pauseDownload(taskInfo->taskId);
				}, Qt::QueuedConnection);
			}, Qt::DirectConnection);

		// 继续按钮点击
		connect(card, &DownloadCard::resumeClicked, this, [this, taskInfo]() {
			// 恢复下载逻辑（如果需要容器处理）
			Q_UNUSED(taskInfo);
			}, Qt::DirectConnection);

		// 删除按钮点击
		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {

			card->disconnect();

			auto downloadEngine = m_downloadEngine.get();
			QMetaObject::invokeMethod(downloadEngine, [this, downloadEngine, taskInfo]() {
				downloadEngine->cancelDownload(taskInfo->taskId);
				}, Qt::QueuedConnection);

			m_downloadTasks.remove(taskInfo->taskId);
			m_downloadCards.remove(taskInfo->taskId);

			// 隐藏卡片
			card->setVisible(false);

			// 更新分页和显示
			updateCurrentPageCards();
			// 更新分页器总页数
			int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);
			}, Qt::DirectConnection);

		// 打开文件夹
		connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
			QFileInfo fileInfo(taskInfo->downloadFilePath);
			QDir dir = fileInfo.absoluteDir();
			if (!dir.exists()) {
				dir.mkpath(".");
			}
			QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
			}, Qt::DirectConnection);

		// 打开链接
		connect(card, &DownloadCard::openUrlClicked, this, [this, taskInfo]() {
			QDesktopServices::openUrl(taskInfo->videoInfo.url);
			}, Qt::DirectConnection);
		break;

	case ContainerState::Downloaded:
		// 打开文件夹
		connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
			QFileInfo fileInfo(taskInfo->downloadFilePath);
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
			}, Qt::DirectConnection);

		// 打开链接
		connect(card, &DownloadCard::openUrlClicked, this, [this, taskInfo]() {
			QDesktopServices::openUrl(taskInfo->videoInfo.url);
			}, Qt::DirectConnection);

		// 删除按钮点击
		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {
			card->disconnect();

			m_downloadTasks.remove(taskInfo->taskId);
			m_downloadCards.remove(taskInfo->taskId);

			// 隐藏卡片
			card->setVisible(false);

			// 更新分页和显示
			updateCurrentPageCards();
			// 更新分页器总页数
			int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);

			// 可选：删除本地文件
			if (QFile::exists(taskInfo->downloadFilePath))
			{
				QFile::remove(taskInfo->downloadFilePath);
			}
			}, Qt::DirectConnection);

		// 预览点击
		connect(card, &DownloadCard::previewClicked, this, [this, taskInfo]() {
			// 预览逻辑（如果需要容器处理）
			Q_UNUSED(taskInfo);
			}, Qt::DirectConnection);
		break;
	}
}

void DownloadCardContainerWidget::addDownloadCard(QSharedPointer<DownloadTaskInfo> downloadTaskInfo)
{
	BENCHMARKING_FUNCTION();

	m_downloadTasks.insert(downloadTaskInfo->taskId, downloadTaskInfo);

	int beforeTotalPages = m_paginationWidget->totalPages();
	// 更新分页器总页数
	int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);

	// 只有当总页数确实发生变化时才更新分页器
	if (totalPages != beforeTotalPages)
	{
		m_paginationWidget->setTotalPages(totalPages);
	}

	// 如果当前页有空间，更新当前页显示
	int currentPageStart = (m_currentPage - 1) * m_pageSize;
	int currentPageEnd = currentPageStart + m_pageSize;
	int newTaskIndex = static_cast<int>(m_downloadTasks.size()) - 1;

	if (newTaskIndex >= currentPageStart && newTaskIndex < currentPageEnd)
	{
		// 新任务在当前显示页，更新卡片显示
		updateCurrentPageCards();
	}

	updateVisibility();
}

void DownloadCardContainerWidget::showLoading()
{
	BENCHMARKING_FUNCTION();
	// 显示加载指示器
	m_spinner->setVisible(true);
	m_spinner->raise();  // 确保在最上层
	// 居中显示
	m_spinner->move((width() - m_spinner->width()) / 2,
		(height() - m_spinner->height()) / 2);
}

void DownloadCardContainerWidget::hideLoading()
{
	BENCHMARKING_FUNCTION();
	m_spinner->setVisible(false);
}

// 添加一个批量添加任务的方法，用于优化大量任务添加时的性能
void DownloadCardContainerWidget::addDownloadCards(QList<QSharedPointer<DownloadTaskInfo>> tasks)
{
	BENCHMARKING_FUNCTION();
	if (tasks.isEmpty()) return;

	int beforeTotalPages = m_paginationWidget->totalPages();

	// 批量添加任务
	for (auto& task : tasks)
	{
		m_downloadTasks.insert(task->taskId, task);
	}

	// 计算新的总页数
	int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);

	if (totalPages != beforeTotalPages)
	{
		m_paginationWidget->setTotalPages(totalPages);
	}

	// 更新当前页的卡片
	updateCurrentPageCards();

	// 隐藏加载指示器
	m_spinner->setVisible(false);
}

void DownloadCardContainerWidget::updateVisibility()
{
	BENCHMARKING_FUNCTION();
	if (m_downloadCards.isEmpty())
	{
		m_buttonBar->setVisible(false);
		m_scrollArea->setVisible(false);
		m_noDataWidget->setVisible(true);
	}
	else
	{
		m_buttonBar->setVisible(true);
		m_scrollArea->setVisible(true);
		m_noDataWidget->setVisible(false);
	}

	bool visible = m_paginationWidget->isVisible();
	if (m_paginationWidget->totalPages() > 1)
	{
		if (!visible)
			m_paginationWidget->setVisible(true);
	}
	else if (visible)
		m_paginationWidget->setVisible(false);
}

void DownloadCardContainerWidget::onDownloadAdded(const QString& taskId)
{
	Q_UNUSED(taskId);
	// 处理新下载任务添加
}

void DownloadCardContainerWidget::onDownloadRemoved(const QString& taskId)
{
	//removeTask(taskId);
}

void DownloadCardContainerWidget::onDownloadStatusChanged(const QString& taskId)
{
	// 处理下载状态变化
	//auto it = m_downloadCards.find(taskId);
	//if (it != m_downloadCards.end())
	//{
	//	auto task = m_downloadTasks.find(taskId);
	//	if (task != m_downloadTasks.end())
	//	{
	//		it.value()->updateFromTaskInfo(task.value());
	//	}
	//}
}

void DownloadCardContainerWidget::onDownloadCompleted(const QString& taskId)
{
	BENCHMARKING_FUNCTION();

	auto card = m_downloadCards.find(taskId);
	if (card != m_downloadCards.end())
	{
		/*auto task = m_downloadTasks.find(taskId);
		if (task != m_downloadTasks.end())
		{
			card.value()->updateFromTaskInfo(task.value());
		}*/
	}

	// 发出任务完成信号
	emit taskStateChanged(taskId, ContainerState::Downloaded);
}

void DownloadCardContainerWidget::onDownloadFailed(const QString& taskId, const QString& error)
{
	Q_UNUSED(error);

	/*auto card = m_downloadCards.find(taskId);
	if (card != m_downloadCards.end())
	{
		auto task = m_downloadTasks.find(taskId);
		if (task != m_downloadTasks.end())
		{
			card.value()->updateFromTaskInfo(task.value());
		}
	}*/
}

void DownloadCardContainerWidget::onDownloadProgress(const QString& taskId)
{
	// 处理下载进度更新
	auto it = m_downloadCards.find(taskId);
	if (it != m_downloadCards.end())
	{
		auto task = m_downloadTasks.find(taskId);
		if (task != m_downloadTasks.end())
		{
			const auto& progressInfo = task.value()->progressInfo;
			it.value()->updateProgress(progressInfo);
		}
	}
}

void DownloadCardContainerWidget::transferTaskToThis(QSharedPointer<DownloadTaskInfo> taskInfo)
{
	BENCHMARKING_FUNCTION();
	// 更新任务状态以匹配容器状态
	QSharedPointer<DownloadTaskInfo> updatedTaskInfo = taskInfo;

	switch (m_containerState) {
	case ContainerState::DownloadReady:
		updatedTaskInfo->status = DownloadStatus::Queued;
		break;
	case ContainerState::Downloading:
		updatedTaskInfo->status = DownloadStatus::Downloading;
		break;
	case ContainerState::Downloaded:
		updatedTaskInfo->status = DownloadStatus::Completed;
		break;
	}

	addDownloadCard(updatedTaskInfo);
}

void DownloadCardContainerWidget::removeTask(const QString& taskId)
{
	BENCHMARKING_FUNCTION();
	// 查找任务
	auto it = m_downloadTasks.find(taskId);

	if (it != m_downloadTasks.end())
	{
		m_downloadTasks.erase(it);

		// 更新分页器总页数
		int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);
		m_paginationWidget->setTotalPages(totalPages);

		// 更新显示
		updateCurrentPageCards();
	}
}

QSharedPointer<DownloadTaskInfo> DownloadCardContainerWidget::getTaskInfo(const QString& taskId) const
{
	BENCHMARKING_FUNCTION();

	auto it = m_downloadTasks.find(taskId);

	if (it != m_downloadTasks.end())
	{
		return it.value();
	}

	return QSharedPointer<DownloadTaskInfo>::create();
}

void DownloadCardContainerWidget::onBatchDownloadClicked()
{
	if (m_downloadTasks.isEmpty()) return;

	QStringList taskIds = m_downloadTasks.keys();



	m_downloadTasks.clear();

}

void DownloadCardContainerWidget::onBatchDeleteClicked()
{
	if (m_downloadTasks.isEmpty()) return;

	DownloadTaskInfo taskInfo;

	QString message;

	switch (m_containerState)
	{
	case ContainerState::DownloadReady:
		taskInfo.status = DownloadStatus::Queued;
		message = tr("确定要删除所有待下载任务吗？");
		break;
	case ContainerState::Downloading:
		taskInfo.status = DownloadStatus::Downloading;
		message = tr("确定要删除所有下载中的任务吗？");
		break;
	case ContainerState::Downloaded:
		taskInfo.status = DownloadStatus::Completed;
		message = tr("确定要删除所有已下载任务吗？这将会同时删除本地文件。");
		break;
	}

	m_downloadTasks.clear();

	// 更新分页器总页数
	m_paginationWidget->setTotalPages(1);

	if (m_currentPage > 1)
	{
		m_currentPage = 1;
		m_paginationWidget->setCurrentPage(m_currentPage);
	}
	updateCurrentPageCards();
}

void DownloadCardContainerWidget::onBatchPauseClicked()
{
	if (m_downloadTasks.isEmpty()) return;

}

void DownloadCardContainerWidget::onBatchResumeClicked()
{
	if (m_downloadTasks.isEmpty()) return;

}