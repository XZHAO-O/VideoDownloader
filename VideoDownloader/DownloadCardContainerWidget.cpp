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
	, m_currentSpeed(0)
{
	BENCHMARKING_FUNCTION();

	DownloadCardState cardState;

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
		//connect(m_downloadEngine.get(), &DownloadEngine::downloadFailed, this, &DownloadCardContainerWidget::onDownloadFailed);
		//connect(m_downloadEngine.get(), &DownloadEngine::downloadPaused, this, &DownloadCardContainerWidget::onDownloadStatusChanged);
		//connect(m_downloadManager.get(), &DownloadManager::downloadResumed, this, &DownloadCardContainerWidget::onDownloadStatusChanged);
		//connect(m_downloadManager.get(), &DownloadManager::downloadStarted, this, &DownloadCardContainerWidget::onDownloadStarted);
		break;
	case ContainerState::Downloaded:
		m_noDataText = tr("暂无已下载任务");
		cardState = DownloadCardState::Downloaded;
		break;
	}

	// 预先创建固定数量的卡片并隐藏
	for (int i = 0; i < m_pageSize; ++i)
	{
		auto emptyModel = QSharedPointer<DownloadCardModel>::create();
		emptyModel->setState(cardState);
		DownloadCard* card = new DownloadCard(emptyModel, m_scrollWidget);
		card->setVisible(false);
		m_precreatedCards.append(card);
	}

	initUI();
}

DownloadCardContainerWidget::~DownloadCardContainerWidget()
{
	// 清理所有卡片
	clearAllCards();
}

void DownloadCardContainerWidget::initUI()
{
	BENCHMARKING_FUNCTION();
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 4, 0, 0);
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
	BENCHMARKING_FUNCTION();
	// 计算当前页的任务范围
	int startIndex = (m_currentPage - 1) * m_pageSize;
	int endIndex = qMin(startIndex + m_pageSize, static_cast<int>(m_downloadTasks.size()));
	int currentPageTaskCount = endIndex - startIndex;

	// 清空当前显示的卡片映射
	m_downloadCards.clear();

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
			auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);
			card->setModel(model);
			card->setVisible(true);

			// 重新设置连接
			setupCardConnections(card, taskInfo);

			// 更新质量下拉框选项 - 新增代码
			if (card->model()->state() == DownloadCardState::Pending)
			{
				// 获取视频和音频质量选项
				QStringList videoQualities = taskInfo->videoStreamInfo.keys();
				QStringList audioQualities = taskInfo->audioStreamInfo.keys();

				// 设置质量下拉框选项
				card->setVideoQualityOptions(videoQualities);
				card->setAudioQualityOptions(audioQualities);

				// 设置当前选中的质量
				if (!taskInfo->selectedVideoQuality.isEmpty())
				{
					card->setCurrentVideoQuality(taskInfo->selectedVideoQuality);
				}
				else if (!videoQualities.isEmpty())
				{
					// 如果没有选中的质量，默认选择第一个
					card->setCurrentVideoQuality(videoQualities.first());
					taskInfo->selectedVideoQuality = videoQualities.first();
				}

				if (!taskInfo->selectedAudioQuality.isEmpty())
				{
					card->setCurrentAudioQuality(taskInfo->selectedAudioQuality);
				}
				else if (!audioQualities.isEmpty())
				{
					// 如果没有选中的音质，默认选择第一个
					card->setCurrentAudioQuality(audioQualities.first());
					taskInfo->selectedAudioQuality = audioQualities.first();
				}
			}

			// 添加到当前显示的映射
			m_downloadCards.insert(taskInfo->taskId, card);

			++it;
			++cardIndex;
		}
		else
		{
			// 隐藏多余的卡片
			card->setVisible(false);
			card->disconnect(); // 断开连接
		}
	}

	updateVisibility();
}

void DownloadCardContainerWidget::clearCurrentCards()
{
	BENCHMARKING_FUNCTION();
	// 断开所有连接并隐藏卡片
	for (auto card : m_precreatedCards) {
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
		connect(card, &DownloadCard::downloadClicked, this, [this, taskInfo, card]() {
			// 发出任务转移信号，让DownloadPage处理容器间的转移和开始下载
			taskInfo->selectedVideoQuality = card->currentVideoQuality();
			taskInfo->selectedAudioQuality = card->currentAudioQuality();
			taskInfo->downloadFormat = DownloadFormat::Separated;
			emit taskStateChanged(taskInfo->taskId, ContainerState::Downloading);
			});

		connect(card, &DownloadCard::videoDownloadClicked, this, [this, taskInfo, card]() {
			taskInfo->selectedVideoQuality = card->currentVideoQuality();
			taskInfo->downloadFormat = DownloadFormat::VideoOnly;
			emit taskStateChanged(taskInfo->taskId, ContainerState::Downloading);
			});

		connect(card, &DownloadCard::audioDownloadClicked, this, [this, taskInfo, card]() {
			taskInfo->selectedAudioQuality = card->currentAudioQuality();
			taskInfo->downloadFormat = DownloadFormat::AudioOnly;
			emit taskStateChanged(taskInfo->taskId, ContainerState::Downloading);
			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {

			m_downloadTasks.remove(taskInfo->taskId);
			m_downloadCards.remove(taskInfo->taskId);

			// 隐藏卡片
			card->setVisible(false);
			card->disconnect();

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
			});
		break;

	case ContainerState::Downloading:

		card->disconnect();

		connect(card, &DownloadCard::pauseClicked, this, [this, taskInfo]() {
			auto downloadEngine = m_downloadEngine.get();
			QMetaObject::invokeMethod(downloadEngine, [this, downloadEngine, taskInfo]() {
				downloadEngine->pauseDownload(taskInfo->taskId);
				}, Qt::QueuedConnection);
			});

		connect(card, &DownloadCard::resumeClicked, this, [this, taskInfo]() {

			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {
			auto downloadEngine = m_downloadEngine.get();
			QMetaObject::invokeMethod(downloadEngine, [this, downloadEngine, taskInfo]() {
				downloadEngine->cancelDownload(taskInfo->taskId);
				}, Qt::QueuedConnection);

			m_downloadTasks.remove(taskInfo->taskId);
			m_downloadCards.remove(taskInfo->taskId);

			// 隐藏卡片
			card->setVisible(false);
			card->disconnect();

			// 更新分页和显示
			updateCurrentPageCards();
			// 更新分页器总页数
			int totalPages = qMax(1, (static_cast<int>(m_downloadTasks.size()) + m_pageSize - 1) / m_pageSize);
			m_paginationWidget->setTotalPages(totalPages);
			});

		connect(card, &DownloadCard::openFolderClicked, this, [this, taskInfo]() {
			// 打开临时文件夹
			QFileInfo fileInfo(taskInfo->downloadFilePath);
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
			QFileInfo fileInfo(taskInfo->downloadFilePath);
			QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
			});

		connect(card, &DownloadCard::openUrlClicked, this, [this, taskInfo]() {
			// 打开原始视频链接
			//if (taskInfo.videoInfo.url.isValid()) {
			//    QDesktopServices::openUrl(taskInfo.videoInfo.url);
			//}
			});

		connect(card, &DownloadCard::deleteClicked, this, [this, taskInfo, card]() {

			m_downloadTasks.remove(taskInfo->taskId);
			m_downloadCards.remove(taskInfo->taskId);

			// 隐藏卡片
			card->setVisible(false);
			card->disconnect();

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

void DownloadCardContainerWidget::onDownloadCompleted(const QString& taskId)
{
	BENCHMARKING_FUNCTION();

	auto card = m_downloadCards.find(taskId);
	if (card != m_downloadCards.end())
	{
		//card->second->updateState(DownloadStatus::Completed);
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

void DownloadCardContainerWidget::onDownloadProgress(const QString& taskId, const QString& progressInfo, int progress, const QString& downloadSpeed)
{
	// 处理下载进度更新
	auto it = m_downloadCards.find(taskId);
	if (it != m_downloadCards.end())
	{
		auto card = *it;
		card->model()->setProgressInfo(progressInfo);
		card->model()->setProgress(progress);
		card->model()->setDownloadSpeed(downloadSpeed);
		card->updateUI();
		//card->second->updateProgress(progress, downloadSpeed);
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