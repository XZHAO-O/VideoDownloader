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
#include "DownloadCard.h"
#include "DownloadManager.h"
#include "DownloadCardPool.h"

DownloadCardContainerWidget::DownloadCardContainerWidget(QSharedPointer<DownloadManager> downloadManager,
	ContainerState state,
	QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
	, m_containerState(state)
	, m_mainLayout(nullptr)
	, m_scrollArea(nullptr)
	, m_scrollWidget(nullptr)
	, m_scrollLayout(nullptr)
	, m_noDataWidget(nullptr)
	, m_paginationWidget(nullptr)
	, m_cardPool(new DownloadCardPool(this))
	, m_spinner(nullptr)
	, m_currentSpeed(0)
{
	// 根据状态设置无数据文本
	switch (m_containerState) {
	case ContainerState::DownloadReady:
		m_noDataText = "暂无待下载任务";
		break;
	case ContainerState::Downloading:
		m_noDataText = "暂无下载任务";
		// 连接下载管理器信号
		connect(m_downloadManager.get(), &DownloadManager::downloadProgress, this, &DownloadCardContainerWidget::onDownloadProgress);
		connect(m_downloadManager.get(), &DownloadManager::downloadCompleted, this, &DownloadCardContainerWidget::onDownloadCompleted);
		connect(m_downloadManager.get(), &DownloadManager::downloadFailed, this, &DownloadCardContainerWidget::onDownloadFailed);
		connect(m_downloadManager.get(), &DownloadManager::downloadPaused, this, &DownloadCardContainerWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadResumed, this, &DownloadCardContainerWidget::onDownloadStatusChanged);
		connect(m_downloadManager.get(), &DownloadManager::downloadStarted, this, &DownloadCardContainerWidget::onDownloadStarted);
		break;
	case ContainerState::Downloaded:
		m_noDataText = "暂无已下载任务";
		break;
	}

	initUI();
}

DownloadCardContainerWidget::~DownloadCardContainerWidget()
{
	// 清理当前显示的卡片
	clearCurrentCards();
}

void DownloadCardContainerWidget::initUI()
{
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);

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
		this, &DownloadCardContainerWidget::onPageChanged);

	m_mainLayout->addWidget(m_paginationWidget);

	updateVisibility();
}

void DownloadCardContainerWidget::onPageChanged(int page)
{
	m_currentPage = page;
	updateCurrentPageCards();
}

void DownloadCardContainerWidget::updateCurrentPageCards()
{
	// 清理当前显示的卡片
	clearCurrentCards();

	// 计算当前页的任务范围
	int startIndex = (m_currentPage - 1) * m_pageSize;
	int endIndex = qMin(startIndex + m_pageSize, m_downloadTasks.size());

	// 添加当前页的卡片
	for (int i = startIndex; i < endIndex; ++i) {
		const DownloadTaskInfo& taskInfo = m_downloadTasks[i];
		auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);

		// 从卡片池获取卡片
		DownloadCard* downloadCard = m_cardPool->getCard(model, this);

		m_downloadCards.insert(taskInfo.taskId, downloadCard);
		// 在添加弹簧之前插入卡片
		m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, downloadCard);

		// 设置卡片连接
		setupCardConnections(downloadCard, taskInfo);
	}

	updateVisibility();
}

void DownloadCardContainerWidget::clearCurrentCards()
{
	// 断开所有连接并从布局中移除
	for (auto it = m_downloadCards.begin(); it != m_downloadCards.end(); ++it)
	{
		DownloadCard* card = it.value();
		card->disconnect();
		m_scrollLayout->removeWidget(card);
		// 释放卡片到池中
		m_cardPool->releaseCard(card);
	}
	m_downloadCards.clear();
}

void DownloadCardContainerWidget::setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo)
{
	switch (m_containerState) {
	case ContainerState::DownloadReady:
		connect(card, &DownloadCard::downloadClicked, this, [this, taskInfo]() {
			// 发出任务转移信号，让DownloadPage处理容器间的转移和开始下载
			emit taskStateChanged(taskInfo.taskId, ContainerState::Downloading);
			});

		connect(card, &DownloadCard::videoDownloadClicked, this, [this, taskInfo]() {
			DownloadTaskInfo updatedTask = taskInfo;
			updatedTask.status = Downloading;

			emit taskStateChanged(taskInfo.taskId, ContainerState::Downloading);

			if (m_downloadManager) {
				m_downloadManager->addDownload(updatedTask);
			}
			});

		connect(card, &DownloadCard::audioDownloadClicked, this, [this, taskInfo]() {
			DownloadTaskInfo updatedTask = taskInfo;
			updatedTask.status = Downloading;

			emit taskStateChanged(taskInfo.taskId, ContainerState::Downloading);

			if (m_downloadManager) {
				m_downloadManager->addDownload(updatedTask);
			}
			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {
			// 从任务列表中移除
			m_downloadTasks.removeOne(taskInfo);

			// 从当前显示的卡片映射中移除
			m_downloadCards.remove(taskInfo.taskId);

			// 从布局中移除卡片
			m_scrollLayout->removeWidget(card);

			// 释放卡片到池中
			m_cardPool->releaseCard(card);

			// 更新分页器总页数
			int totalPages = qMax(1, (m_downloadTasks.size() + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);

			// 如果删除后当前页没有内容且不是第一页，则回到前一页
			if (m_downloadCards.isEmpty() && m_currentPage > 1) {
				m_currentPage--;
				m_paginationWidget->setCurrentPage(m_currentPage);
				updateCurrentPageCards(); // 需要重新加载整个页面
			}
			else {
				// 如果还有后续任务，将下一个任务添加到当前页
				int currentPageStart = (m_currentPage - 1) * m_pageSize;
				int currentPageEnd = currentPageStart + m_pageSize;

				if (m_downloadTasks.size() > currentPageEnd - 1) {
					// 还有任务可以添加到当前页
					const DownloadTaskInfo& nextTask = m_downloadTasks[currentPageEnd - 1];
					auto model = QSharedPointer<DownloadCardModel>::create(nextTask);
					DownloadCard* newCard = m_cardPool->getCard(model, m_scrollWidget);

					if (newCard) {
						m_downloadCards.insert(nextTask.taskId, newCard);
						m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, newCard);
						setupCardConnections(newCard, nextTask);
					}
				}
				// 否则只更新当前页的显示
				updateVisibility();
			}
			});
		break;

	case ContainerState::Downloading:
		card->disconnect();
		connect(card, &DownloadCard::pauseClicked, this, [this, taskInfo]() {
			if (m_downloadManager) {
				m_downloadManager->pauseDownload(taskInfo.taskId);
			}
			});

		connect(card, &DownloadCard::resumeClicked, this, [this, taskInfo]() {
			if (m_downloadManager) {
				m_downloadManager->resumeDownload(taskInfo.taskId);
			}
			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {
			if (m_downloadManager) {
				m_downloadManager->cancelDownload(taskInfo.taskId);
			}

			// 从任务列表中移除
			m_downloadTasks.removeOne(taskInfo);

			// 从当前显示的卡片映射中移除
			m_downloadCards.remove(taskInfo.taskId);

			// 从布局中移除卡片
			m_scrollLayout->removeWidget(card);

			// 释放卡片到池中
			m_cardPool->releaseCard(card);

			// 更新分页和显示
			updateCurrentPageCards();
			// 更新分页器总页数
			int totalPages = qMax(1, (m_downloadTasks.size() + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);
			});

		connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
			// 打开临时文件夹
			QFileInfo fileInfo(taskInfo.request.outputPath);
			QDir dir = fileInfo.absoluteDir();
			if (!dir.exists()) {
				dir.mkpath(".");
			}
			QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
			});
		break;

	case ContainerState::Downloaded:
		card->disconnect();
		connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
			// 打开文件所在文件夹
			QFileInfo fileInfo(taskInfo.request.outputPath);
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
			});

		connect(card, &DownloadCard::openUrlClicked, this, [this, taskInfo]() {
			// 打开原始视频链接
			//if (taskInfo.videoInfo.url.isValid()) {
			//	QDesktopServices::openUrl(taskInfo.videoInfo.url);
			//}
			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {
			// 从任务列表中移除
			m_downloadTasks.removeOne(taskInfo);

			// 从当前显示的卡片映射中移除
			m_downloadCards.remove(taskInfo.taskId);

			// 从布局中移除卡片
			m_scrollLayout->removeWidget(card);

			// 释放卡片到池中
			m_cardPool->releaseCard(card);

			// 更新分页和显示
			updateCurrentPageCards();
			// 更新分页器总页数
			int totalPages = qMax(1, (m_downloadTasks.size() + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);

			// 可选：删除本地文件
			if (QFile::exists(taskInfo.request.outputPath)) {
				QFile::remove(taskInfo.request.outputPath);
			}
			});
		break;
	}
}

QString DownloadCardContainerWidget::getNoDataText() const
{
	return m_noDataText;
}

void DownloadCardContainerWidget::downloadVideo(const QUrl& url)
{
	// 创建目录
	QString savePath = "E:/CProject";
	QDir dir(savePath);
	if (!dir.exists()) {
		dir.mkpath(".");
	}

	qDebug() << url;
	QString fileName = QFileInfo(url.path()).fileName();
	QString filePath = savePath + "/" + fileName;
	// 打开文件
	QFile* m_file = new QFile(filePath, this);
	if (!m_file->open(QIODevice::WriteOnly)) {
		delete m_file;
		m_file = nullptr;
		return;
	}

	// 创建网络请求
	QNetworkRequest request;
	request.setUrl(url);

	request.setRawHeader("User-Agent",
		"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
	request.setRawHeader("Referer", "https://www.bilibili.com");
	request.setRawHeader("Origin", "https://www.bilibili.com");

	QNetworkAccessManager* m_manager = new QNetworkAccessManager(this);

	// 开始下载
	QNetworkReply* m_reply = m_manager->get(request);
	// 连接信号处理下载数据
	connect(m_reply, &QNetworkReply::readyRead, this, [this, m_reply, m_file]() {
		// 将可用数据写入文件
		if (m_reply->bytesAvailable() > 0) {
			m_file->write(m_reply->readAll());
		}
		});

	// 处理下载完成
	connect(m_reply, &QNetworkReply::finished, this, [this, m_reply, m_file, m_manager]() {
		// 写入剩余数据
		if (m_reply->bytesAvailable() > 0) {
			m_file->write(m_reply->readAll());
		}

		m_file->close();

		if (m_reply->error() == QNetworkReply::NoError) {
			qDebug() << "下载完成";
		}
		else {
			qDebug() << "下载失败:" << m_reply->errorString();
			// 删除不完整的文件
			m_file->remove();
		}

		// 清理资源
		m_reply->deleteLater();
		m_file->deleteLater();
		m_manager->deleteLater();
		});

	// 下载进度
	connect(m_reply, &QNetworkReply::downloadProgress, this, [](qint64 bytesReceived, qint64 bytesTotal) {
		if (bytesTotal > 0) {
			double percent = (double(bytesReceived) / double(bytesTotal)) * 100.0;
			qDebug() << "下载进度:" << bytesReceived << "/" << bytesTotal
				<< "(" << QString::number(percent, 'f', 1) << "%)";
		}
		});
}

void DownloadCardContainerWidget::addDownloadCard(const DownloadTaskInfo& downloadTaskInfo)
{
	m_downloadTasks.append(downloadTaskInfo);

	int beforeTotalPages = m_paginationWidget->totalPages();
	// 更新分页器总页数
	int totalPages = qMax(1, (m_downloadTasks.size() + m_pageSize - 1) / m_pageSize);

	// 只有当总页数确实发生变化时才更新分页器
	if (totalPages != beforeTotalPages)
	{
		m_paginationWidget->setTotalPages(totalPages);
	}

	// 如果当前页有空间，直接添加卡片
	int currentPageStart = (m_currentPage - 1) * m_pageSize;
	int currentPageEnd = currentPageStart + m_pageSize;
	int newTaskIndex = m_downloadTasks.size() - 1;

	if (newTaskIndex >= currentPageStart && newTaskIndex < currentPageEnd)
	{
		// 新任务在当前显示页，添加卡片
		if (m_downloadCards.size() < m_pageSize)
		{
			auto model = QSharedPointer<DownloadCardModel>::create(downloadTaskInfo);
			DownloadCard* downloadCard = m_cardPool->getCard(model, this);

			m_downloadCards.insert(downloadTaskInfo.taskId, downloadCard);
			m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, downloadCard);
			setupCardConnections(downloadCard, downloadTaskInfo);
		}
	}

	updateVisibility();
}

void DownloadCardContainerWidget::showLoading()
{
	// 显示加载指示器
	m_spinner->setVisible(true);
	m_spinner->raise();  // 确保在最上层
	// 居中显示
	m_spinner->move((width() - m_spinner->width()) / 2,
		(height() - m_spinner->height()) / 2);
}

// 添加一个批量添加任务的方法，用于优化大量任务添加时的性能
void DownloadCardContainerWidget::addDownloadCards(QList<DownloadTaskInfo>&& tasks)
{
	if (tasks.isEmpty()) return;

	// 开始批量操作前暂停UI更新
	setUpdatesEnabled(false);

	int beforeTotalPages = m_paginationWidget->totalPages();

	// 批量添加任务
	m_downloadTasks.reserve(m_downloadTasks.size() + tasks.size());
	for (auto&& task : tasks)
	{
		m_downloadTasks.append(std::move(task));
	}

	// 计算新的总页数
	int totalPages = qMax(1, (m_downloadTasks.size() + m_pageSize - 1) / m_pageSize);

	// 只有当总页数确实发生变化时才更新分页器
	if (totalPages != beforeTotalPages)
	{
		m_paginationWidget->setTotalPages(totalPages);
	}

	// 更新当前页的卡片
	updateCurrentPageCards();

	// 恢复UI更新
	setUpdatesEnabled(true);

	// 隐藏加载指示器
	m_spinner->setVisible(false);
}

void DownloadCardContainerWidget::updateVisibility()
{
	if (m_downloadCards.isEmpty())
	{
		m_scrollArea->setVisible(false);
		m_noDataWidget->setVisible(true);
	}
	else
	{
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
	// 处理新下载任务添加
}

void DownloadCardContainerWidget::onDownloadRemoved(const QString& taskId)
{
	// 处理下载任务移除
}

void DownloadCardContainerWidget::onDownloadStatusChanged(const QString& taskId)
{
	// 处理下载状态变化
	// 可以在这里更新卡片状态显示
}

void DownloadCardContainerWidget::onDownloadCompleted(const QString& taskId, const QString& filePath)
{
	// 更新任务信息
	for (auto& task : m_downloadTasks) {
		if (task.taskId == taskId) {
			task.status = Completed;
			task.request.outputPath = filePath;
			break;
		}
	}

	// 发出任务完成信号
	emit taskStateChanged(taskId, ContainerState::Downloaded);
}

void DownloadCardContainerWidget::onDownloadFailed(const QString& taskId, const QString& error)
{
	// 处理下载失败，可以显示错误状态或移回待下载
	qDebug() << "Download failed for task:" << taskId << "Error:" << error;
	// 可以选择将任务移回待下载状态
	// emit taskStateChanged(taskId, ContainerState::DownloadReady);
}

void DownloadCardContainerWidget::onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	// 更新对应卡片的进度
	DownloadCard* card = m_downloadCards.value(taskId, nullptr);
	if (card) {
		int progress = total > 0 ? static_cast<int>((downloaded * 100) / total) : 0;
		card->model()->setDownloadedSize(downloaded);
		card->model()->setDownloadSize(total);
		card->model()->setProgress(progress);
		card->model()->setDownloadSpeed(m_currentSpeed);

		// 强制更新UI
		card->updateUI();
	}
}

void DownloadCardContainerWidget::onDownloadSpeedUpdated(qint64 bytesPerSecond)
{
	//m_currentSpeed = bytesPerSecond;

	//DownloadCard* card = m_downloadCards.value(taskId, nullptr);
	//if (card) {
	//	card->model()->setState(DownloadCardState::Downloading);
	//	//card->updateUI();
	//}

	// 更新所有卡片的下载速度
	//for (auto it = m_downloadCards.begin(); it != m_downloadCards.end(); ++it)
	//{
	//	DownloadCard* card = it.value();
	//	auto model = card->model();
	//	if (model && model->state() == DownloadCardState::Downloading)
	//	{
	//		model->setDownloadSpeed(bytesPerSecond);
	//		//card->updateUI();
	//	}
	//}
}

void DownloadCardContainerWidget::onDownloadStarted(const QString& taskId)
{
	// 更新任务状态为下载中
	for (auto& task : m_downloadTasks) {
		if (task.taskId == taskId) {
			task.status = Downloading;
			break;
		}
	}

	// 更新对应卡片的UI
	DownloadCard* card = m_downloadCards.value(taskId, nullptr);
	if (card) {
		card->model()->setState(DownloadCardState::Downloading);
		//card->updateUI();
	}
}

void DownloadCardContainerWidget::updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total)
{
	// 更新特定任务的进度
	DownloadCard* card = m_downloadCards.value(taskId, nullptr);
	if (card) {
		int progress = total > 0 ? static_cast<int>((downloaded * 100) / total) : 0;
		card->model()->setProgress(progress);
		//card->updateUI();
	}
}

void DownloadCardContainerWidget::setState(ContainerState state)
{
	if (m_containerState != state) {
		m_containerState = state;

		// 更新无数据文本
		switch (m_containerState) {
		case ContainerState::DownloadReady:
			m_noDataText = "暂无待下载任务";
			break;
		case ContainerState::Downloading:
			m_noDataText = "暂无下载任务";
			break;
		case ContainerState::Downloaded:
			m_noDataText = "暂无已下载任务";
			break;
		}

		if (m_noDataWidget)
		{
			m_noDataWidget->setText(m_noDataText);
		}
	}
}

void DownloadCardContainerWidget::transferTaskToThis(const DownloadTaskInfo& taskInfo)
{
	// 更新任务状态以匹配容器状态
	DownloadTaskInfo updatedTaskInfo = taskInfo;

	switch (m_containerState) {
	case ContainerState::DownloadReady:
		updatedTaskInfo.status = Queued;
		break;
	case ContainerState::Downloading:
		updatedTaskInfo.status = Downloading;
		break;
	case ContainerState::Downloaded:
		updatedTaskInfo.status = Completed;
		break;
	}

	addDownloadCard(updatedTaskInfo);
}

void DownloadCardContainerWidget::removeTask(const QString& taskId)
{
	// 查找任务
	auto it = std::find_if(m_downloadTasks.begin(), m_downloadTasks.end(),
		[taskId](const DownloadTaskInfo& task) { return task.taskId == taskId; });

	if (it != m_downloadTasks.end()) {
		// 从当前显示的卡片映射中查找并移除
		DownloadCard* cardToRemove = m_downloadCards.value(taskId, nullptr);

		if (cardToRemove) {
			// 从当前显示的卡片映射中移除
			m_downloadCards.remove(taskId);

			// 从布局中移除卡片
			m_scrollLayout->removeWidget(cardToRemove);

			// 释放卡片到池中
			m_cardPool->releaseCard(cardToRemove);
		}

		// 从任务列表中移除
		m_downloadTasks.erase(it);

		// 更新分页器总页数
		int totalPages = qMax(1, (m_downloadTasks.size() + m_pageSize - 1) / m_pageSize);
		m_paginationWidget->setTotalPages(totalPages);

		// 更新显示
		updateCurrentPageCards();
	}
}

DownloadTaskInfo DownloadCardContainerWidget::getTaskInfo(const QString& taskId) const
{
	auto it = std::find_if(m_downloadTasks.begin(), m_downloadTasks.end(),
		[taskId](const DownloadTaskInfo& task) { return task.taskId == taskId; });

	if (it != m_downloadTasks.end()) {
		return *it;
	}

	return DownloadTaskInfo();
}