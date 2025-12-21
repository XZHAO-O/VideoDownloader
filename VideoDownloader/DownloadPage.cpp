#include "DownloadPage.h"

#include <QtConcurrent>

#include "AntButton.h"
#include "AntScrollArea.h"
#include "MaterialTabWidget.h"
#include "DownloadCard.h"
#include "DownloadEngine.h"
#include "ApplicationController.h"
#include "PlatformAggregatorService.h"
#include "ConfigVideoPlatform.h"
#include "Instrumentor.h"
#include "AntMessageManager.h"
#include "CancelManager.h"
#include "MaterialSpinner.h"

DownloadPage::DownloadPage(QSharedPointer<ApplicationController> applicationController, QWidget* parent)
	: QWidget(parent)
	, m_applicationController(applicationController)
	, m_downloadEngine(applicationController->getDownloadEngine())
{
	BENCHMARKING_FUNCTION();
	setObjectName("DownloadPage");
	setFocusPolicy(Qt::ClickFocus);  // 设置焦点策略 点击空白处可以获取焦点

	// 标签页
	tabWidget = new MaterialTabWidget(this);
	tabWidget->getLayout()->setContentsMargins(15, 0, 5, 0);

	// 滚动区域
	scrollArea1 = new AntScrollArea(AntScrollArea::ScrollVertical, this);
	QWidget* w1 = new QWidget(this);
	scrollArea1->addWidget(w1);

	downloadReadyWidget = new DownloadCardContainerWidget(m_downloadEngine, ContainerState::DownloadReady, this);
	downloadingWidget = new DownloadCardContainerWidget(m_downloadEngine, ContainerState::Downloading, this);
	downloadedWidget = new DownloadCardContainerWidget(m_downloadEngine, ContainerState::Downloaded, this);

	// 连接任务状态改变信号
	connect(downloadReadyWidget, &DownloadCardContainerWidget::taskStateChanged,
		this, &DownloadPage::onTaskStateChanged);
	connect(downloadingWidget, &DownloadCardContainerWidget::taskStateChanged,
		this, &DownloadPage::onTaskStateChanged);

	// 连接下载管理器信号
	/*onnect(m_downloadManager.get(), &DownloadManager::downloadCompleted,
		this, &DownloadPage::onDownloadManagerCompleted);
	connect(m_downloadManager.get(), &DownloadManager::downloadFailed,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadFailed);
	connect(m_downloadManager.get(), &DownloadManager::downloadPaused,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadStatusChanged);
	connect(m_downloadManager.get(), &DownloadManager::downloadResumed,
		downloadingWidget, &DownloadCardContainerWidget::onDownloadStatusChanged);*/

		// 添加标签项
	tabWidget->addTab(downloadReadyWidget, tr("待下载"));
	tabWidget->addTab(downloadingWidget, tr("下载中"));
	tabWidget->addTab(downloadedWidget, tr("已下载"));
	tabWidget->addTab(scrollArea1, tr("常用控件"));

	// 主布局
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(tabWidget);

	// 页面内容布局
	QVBoxLayout* pageLay = new QVBoxLayout(w1);
	pageLay->setSpacing(14);
	pageLay->setContentsMargins(0, 0, 0, 0);

	QVBoxLayout* row8Layout = new QVBoxLayout();
	row8Layout->setSpacing(1);
	row8Layout->setContentsMargins(0, 0, 0, 0);
	// 创建示例任务信息
	auto createSampleTaskInfo = [](const QString& title, DownloadStatus status) {
		auto taskInfo = QSharedPointer<DownloadTaskInfo>::create();
		taskInfo->taskId = StringUtil::generateId("sample");
		taskInfo->videoInfo.title = title;
		taskInfo->videoInfo.duration = "12:34";
		taskInfo->videoInfo.publishTime = "1672502400"; // 2023-01-01 00:00:00
		taskInfo->videoInfo.author = "示例发布者";
		taskInfo->videoInfo.coverUrl = QUrl("https://example.com/cover.jpg");
		taskInfo->videoInfo.cover = QByteArray(); // 空封面数据

		// 添加视频流信息
		StreamInfo videoStream;
		videoStream.url = QUrl("https://example.com/video.mp4");
		videoStream.fileSize = 1024 * 1024 * 150; // 150MB
		taskInfo->videoStreamInfo.insert("4K", videoStream);

		StreamInfo videoStream2;
		videoStream2.url = QUrl("https://example.com/video2.mp4");
		videoStream2.fileSize = 1024 * 1024 * 100; // 100MB
		taskInfo->videoStreamInfo.insert("1080P", videoStream2);

		// 添加音频流信息
		StreamInfo audioStream;
		audioStream.url = QUrl("https://example.com/audio.aac");
		audioStream.fileSize = 1024 * 1024 * 20; // 20MB
		taskInfo->audioStreamInfo.insert("无损", audioStream);

		StreamInfo audioStream2;
		audioStream2.url = QUrl("https://example.com/audio2.aac");
		audioStream2.fileSize = 1024 * 1024 * 10; // 10MB
		taskInfo->audioStreamInfo.insert("高品质", audioStream2);

		taskInfo->selectedVideoQuality = "4K";
		taskInfo->selectedAudioQuality = "无损";
		taskInfo->downloadFormat = DownloadFormat::Separated;
		taskInfo->status = status;
		taskInfo->downloadFilePath = "E:/Downloads/示例视频.mp4";
		taskInfo->endTime = QDateTime::currentDateTime();

		// 如果是下载中状态，设置进度信息
		if (status == DownloadStatus::Downloading) {
			taskInfo->progressInfo.progress = 50;
			taskInfo->progressInfo.text = "75.0MB/150.0MB";
			taskInfo->progressInfo.downloadSpeed = "1.2MB/s";
		}

		return taskInfo;
		};

	// 创建示例卡片
	// 1. 待下载状态卡片
	auto pendingTask = createSampleTaskInfo("示例视频 - 待下载", DownloadStatus::Queued);
	DownloadCardState cardState = DownloadCardState::Pending;
	DownloadCard* downloadCard1 = new DownloadCard(cardState, this);
	downloadCard1->updateFromTaskInfo(pendingTask);

	// 2. 下载中状态卡片
	auto downloadingTask = createSampleTaskInfo("示例视频 - 下载中", DownloadStatus::Downloading);
	cardState = DownloadCardState::Downloading;
	DownloadCard* downloadCard2 = new DownloadCard(cardState, this);
	downloadCard2->updateFromTaskInfo(downloadingTask);

	// 3. 已下载状态卡片
	auto downloadedTask = createSampleTaskInfo("示例视频 - 已下载", DownloadStatus::Completed);
	cardState = DownloadCardState::Downloaded;
	DownloadCard* downloadCard3 = new DownloadCard(cardState, this);
	downloadCard3->updateFromTaskInfo(downloadedTask);

	// 4. 错误状态卡片
	auto errorTask = createSampleTaskInfo("示例视频 - 下载失败", DownloadStatus::Failed);
	cardState = DownloadCardState::Error;
	DownloadCard* downloadCard4 = new DownloadCard(cardState, this);
	downloadCard4->updateFromTaskInfo(errorTask);

	// 5. 另一个待下载卡片
	auto pendingTask2 = createSampleTaskInfo("另一个示例视频 - 待下载", DownloadStatus::Queued);
	cardState = DownloadCardState::Pending;
	DownloadCard* downloadCard5 = new DownloadCard(cardState, this);
	downloadCard5->updateFromTaskInfo(pendingTask2);

	MaterialSpinner* spinner = new MaterialSpinner(QSize(40, 40), 4, DesignSystem::instance()->primaryColor(), this);

	// 添加到布局中
	row8Layout->addWidget(downloadCard1);
	row8Layout->addWidget(downloadCard2);
	row8Layout->addWidget(downloadCard3);
	row8Layout->addWidget(downloadCard4);
	row8Layout->addWidget(downloadCard5);
	row8Layout->addWidget(spinner);

	// 添加到页面布局
	pageLay->addLayout(row8Layout);
	pageLay->addStretch();
}

DownloadPage::~DownloadPage()
{
	// 取消当前操作
	cancelCurrentOperation();
	m_applicationController->getNetworkManager()->clear();
}

void DownloadPage::getVideoUrlInfo(QSharedPointer<DownloadTaskInfo> taskInfo)
{
	BENCHMARKING_FUNCTION();
	taskInfo->videoInfo.videoPlatform->getDownloadInfo(taskInfo);
}

void DownloadPage::getVideoCover(QSharedPointer<DownloadTaskInfo> taskInfo)
{
	//网络错误情况和videoPlatfrom为空的情况处理待增加
	BENCHMARKING_FUNCTION();
	taskInfo->videoInfo.videoPlatform->getVideoCover(taskInfo);
}

void DownloadPage::createDownloadCards(QList<VideoInfo>&& videoInfoList)
{
	BENCHMARKING_FUNCTION();

	// 1. 取消之前的操作
	if (m_currentWatcher && m_currentWatcher->isRunning())
	{
		cancelCurrentOperation();
	}

	m_currentOperationToken = CancelManager::instance().createCancelToken("video_download_cards");
	downloadReadyWidget->showLoading();

	// 2. 预处理：在主线程将 VideoInfo 移动到 TaskInfo 中 (避免在多线程中修改源列表导致的深拷贝)
	QList<QSharedPointer<DownloadTaskInfo>> inputTaskList;
	inputTaskList.reserve(videoInfoList.size());

	for (auto& info : videoInfoList)
	{
		auto taskInfo = QSharedPointer<DownloadTaskInfo>::create();
		taskInfo->taskId = StringUtil::generateId("task");
		// 核心优化：在主线程完成 Move，耗时极低
		taskInfo->videoInfo = std::move(info);
		inputTaskList.append(taskInfo);
	}

	// 3. 准备结果列表 (用于最终展示)
	// 注意：Watcher 的 resultReadyAt 返回的将是 mapped 处理后的结果
	auto validResultList = QSharedPointer<QList<QSharedPointer<DownloadTaskInfo>>>::create();
	validResultList->reserve(inputTaskList.size());

	m_currentWatcher = new QFutureWatcher<QSharedPointer<DownloadTaskInfo>>(this);

	// 4. 信号连接
	connect(m_currentWatcher, &QFutureWatcher<QSharedPointer<DownloadTaskInfo>>::resultReadyAt, this, [this, validResultList](int index) {
		// 获取处理完的任务
		QSharedPointer<DownloadTaskInfo> taskInfo = m_currentWatcher->resultAt(index);

		// 二次检查：如果任务本身有效且未取消，加入最终显示列表
		if (taskInfo && !m_currentOperationToken.isEmpty() &&
			!CancelManager::instance().isCancelled(m_currentOperationToken))
		{
			validResultList->append(taskInfo);
		}
		});

	connect(m_currentWatcher, &QFutureWatcher<QSharedPointer<DownloadTaskInfo>>::finished,
		this, [this, validResultList]() {
			if (!m_currentOperationToken.isEmpty() &&
				!CancelManager::instance().isCancelled(m_currentOperationToken))
			{
				downloadReadyWidget->addDownloadCards(*validResultList);
				AntMessageManager::instance()->showMessage(AntMessage::Success, AntMessage::Singleton, "数据解析成功");
			}
			else
			{
				AntMessageManager::instance()->showMessage(AntMessage::Info, AntMessage::Singleton, "操作已取消");
			}

			if (!m_currentOperationToken.isEmpty())
			{
				CancelManager::instance().cleanupToken(m_currentOperationToken);
				m_currentOperationToken.clear();
			}

			m_currentWatcher->deleteLater();
			m_currentWatcher = nullptr;
		});

	// 5. 并发处理
	// mapped 直接处理 QSharedPointer 列表，复制成本仅为指针复制（引用计数增加），非常廉价
	QFuture<QSharedPointer<DownloadTaskInfo>> future = QtConcurrent::mapped(inputTaskList,
		[cancelToken = m_currentOperationToken](QSharedPointer<DownloadTaskInfo> taskInfo) {

			// 检查取消
			if (!cancelToken.isEmpty() && CancelManager::instance().isCancelled(cancelToken))
			{
				return taskInfo; // 或者返回 nullptr 并在外部过滤
			}

			// 执行耗时网络操作 (数据已经在 taskInfo 里面了，无需再 copy/move)
			taskInfo->videoInfo.videoPlatform->getDownloadInfo(taskInfo, cancelToken);

			if (!cancelToken.isEmpty() && CancelManager::instance().isCancelled(cancelToken))
			{
				return taskInfo;
			}

			taskInfo->videoInfo.videoPlatform->getVideoCover(taskInfo, cancelToken);
			taskInfo->downloadFilePath = "E:/CProject/" + taskInfo->videoInfo.title + ".mp4";

			return taskInfo;
		});

	m_currentWatcher->setFuture(future);
}

void DownloadPage::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
}

void DownloadPage::cancelCurrentOperation()
{
	if (!m_currentOperationToken.isEmpty())
	{
		CancelManager::instance().cancelOperation(m_currentOperationToken);
	}

	if (m_currentWatcher && m_currentWatcher->isRunning())
	{
		m_currentWatcher->cancel();
		m_currentWatcher->waitForFinished(); // 阻塞等待直到所有任务取消完成
	}

	if (!m_currentOperationToken.isEmpty())
	{
		CancelManager::instance().cleanupToken(m_currentOperationToken);
		m_currentOperationToken.clear();
	}

	//downloadReadyWidget->hideLoading();
}

// 添加任务状态改变处理函数
void DownloadPage::onTaskStateChanged(const QString& taskId, ContainerState newState)
{
	BENCHMARKING_FUNCTION();
	QSharedPointer<DownloadTaskInfo> taskInfo;
	ContainerState sourceState = ContainerState::DownloadReady;

	// 查找任务在哪个容器中
	if (!(taskInfo = downloadReadyWidget->getTaskInfo(taskId))->taskId.isEmpty()) {
		sourceState = ContainerState::DownloadReady;
	}
	else if (!(taskInfo = downloadingWidget->getTaskInfo(taskId))->taskId.isEmpty()) {
		sourceState = ContainerState::Downloading;
	}
	else {
		qDebug() << "Task not found:" << taskId;
		return;
	}

	// 如果源状态和目标状态相同，不处理
	if (sourceState == newState) {
		return;
	}

	// 从源容器移除任务
	switch (sourceState) {
	case ContainerState::DownloadReady:
		downloadReadyWidget->removeTask(taskId);
		break;
	case ContainerState::Downloading:
		downloadingWidget->removeTask(taskId);
		break;
	}

	// 更新任务状态以匹配目标容器状态
	switch (newState) {
	case ContainerState::Downloading:
		taskInfo->status = DownloadStatus::Downloading;
		break;
	case ContainerState::Downloaded:
		taskInfo->status = DownloadStatus::Completed;
		break;
	}

	// 添加到目标容器
	switch (newState) {
	case ContainerState::Downloading:
		downloadingWidget->transferTaskToThis(taskInfo);
		// 如果是转移到下载中，开始下载
		if (m_downloadEngine && sourceState == ContainerState::DownloadReady) {
			// 只有从待下载转移时才调用addDownload
			m_downloadEngine->addDownloadTask(taskInfo);
		}
		break;
	case ContainerState::Downloaded:
		downloadedWidget->transferTaskToThis(taskInfo);
		break;
	}
}

// 处理下载管理器完成的信号
void DownloadPage::onDownloadManagerCompleted(const QString& taskId, const QString& filePath)
{
	BENCHMARKING_FUNCTION();
	// 更新任务信息中的文件路径
	QSharedPointer<DownloadTaskInfo> taskInfo = downloadingWidget->getTaskInfo(taskId);
	if (!taskInfo->taskId.isEmpty()) {
		taskInfo->downloadFilePath = filePath;
		taskInfo->status = DownloadStatus::Completed;
		// 转移到已下载容器
		onTaskStateChanged(taskId, ContainerState::Downloaded);
	}
}