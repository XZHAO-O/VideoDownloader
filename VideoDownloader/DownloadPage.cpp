#include "DownloadPage.h"

#include <QtConcurrent>

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

	QHBoxLayout* row8Layout = new QHBoxLayout();
	row8Layout->setSpacing(1);
	row8Layout->setContentsMargins(0, 0, 0, 0);
	// 创建卡片模型
	auto cardModel = QSharedPointer<DownloadCardModel>::create();
	cardModel->setTitle("示例视频标题");
	cardModel->setDuration("12:34");
	cardModel->setPublishTime("2016-01-01 12:00:00");
	cardModel->setPublisher("视频发布者");
	cardModel->setVideoSize(1024 * 1024 * 150); // 150MB
	cardModel->setAudioSize(1024 * 1024 * 20);  // 20MB
	cardModel->setProgress(50);
	cardModel->setState(DownloadCardState::Downloading);

	// 创建卡片
	auto downloadCard = new DownloadCard(cardModel, this);

	// 添加到布局中
	row8Layout->addWidget(downloadCard);

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
	taskInfo->videoInfo.videoPlatform->getVideoUrlInfo(taskInfo);
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
			taskInfo->videoInfo.videoPlatform->getVideoUrlInfo(taskInfo, cancelToken);

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