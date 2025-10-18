#include "DownloadCardContainerWidget.h"
#include <QScrollArea>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QDir>

DownloadCardContainerWidget::DownloadCardContainerWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent)
	: QWidget(parent)
	, m_downloadManager(downloadManager)
	, m_mainLayout(nullptr)
	, m_scrollArea(nullptr)
	, m_scrollWidget(nullptr)
	, m_scrollLayout(nullptr)
	, m_noDataWidget(nullptr)
	, m_noDataText("暂无任务") // 默认文本
{
	// 注意：不要在构造函数中调用任何虚函数
	// updateTaskList() 和 initUI() 将在派生类构造函数中调用
}

DownloadCardContainerWidget::~DownloadCardContainerWidget()
{
}

void DownloadCardContainerWidget::initUI()
{
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);

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
	m_noDataWidget->setText(m_noDataText); // 使用成员变量而不是虚函数
	m_noDataWidget->hide();
	m_mainLayout->addWidget(m_noDataWidget);
}

void DownloadCardContainerWidget::updateTaskList()
{
	// 清空当前卡片
	for (auto card : m_taskCards) {
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}
	m_taskCards.clear();

	// 获取任务列表
	if (m_downloadManager) {
		auto tasks = getTaskList(); // 现在这是安全的，因为对象已经完全构造
		for (const auto& task : tasks) {
			addTaskCard(task);
		}
	}

	updateVisibility();
}

// 其他方法保持不变...
void DownloadCardContainerWidget::addTaskCard(const DownloadTaskInfo& taskInfo)
{
	auto model = QSharedPointer<DownloadCardModel>::create(taskInfo);
	auto card = new DownloadCard(model, this);

	// 在添加弹簧之前插入卡片
	m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, card);
	m_taskCards[taskInfo.taskId] = card;

	// 设置卡片连接
	setupCardConnections(card, taskInfo);

	updateVisibility();
}

void DownloadCardContainerWidget::removeTaskCard(const QString& taskId)
{
	if (m_taskCards.contains(taskId)) {
		auto card = m_taskCards.take(taskId);
		m_scrollLayout->removeWidget(card);
		card->deleteLater();
	}

	updateVisibility();
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

void DownloadCardContainerWidget::addDownloadCard(DownloadTaskInfo downloadTaskInfo, DownloadCard* downloadCard)
{
	m_downloadTasks.append(downloadTaskInfo);
	m_downloadCards.append(downloadCard);
	// 在添加弹簧之前插入卡片
	m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, downloadCard);

	// 连接信号
	connect(downloadCard, &DownloadCard::downloadClicked, this, [this, downloadTaskInfo]() {
		downloadVideo(downloadTaskInfo.request.videoPlayUrl);
		});

	// 连接删除信号
	connect(downloadCard, &DownloadCard::deleteClicked, this, [this, downloadCard]() {
		// 从布局中移除并删除卡片
		m_scrollLayout->removeWidget(downloadCard);
		m_downloadCards.removeOne(downloadCard);
		downloadCard->deleteLater();
		updateVisibility();
		});

	updateVisibility();
}

void DownloadCardContainerWidget::updateVisibility()
{
	bool hasTasks = !m_taskCards.isEmpty() || !m_downloadCards.isEmpty();
	m_scrollArea->setVisible(hasTasks);
	m_noDataWidget->setVisible(!hasTasks);
}

void DownloadCardContainerWidget::onDownloadAdded(const QString& taskId)
{
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		if (!m_taskCards.contains(taskId)) {
			addTaskCard(taskInfo);
		}
	}
}

void DownloadCardContainerWidget::onDownloadRemoved(const QString& taskId)
{
	removeTaskCard(taskId);
}

void DownloadCardContainerWidget::onDownloadStatusChanged(const QString& taskId)
{
	// 基类默认实现，派生类可以重写
	if (m_downloadManager) {
		auto taskInfo = m_downloadManager->getDownloadInfo(taskId);
		// 如果任务不在当前列表应该显示的状态中，移除卡片
		auto taskList = getTaskList();
		bool shouldShow = false;
		for (const auto& task : taskList) {
			if (task.taskId == taskId) {
				shouldShow = true;
				break;
			}
		}

		if (!shouldShow && m_taskCards.contains(taskId)) {
			removeTaskCard(taskId);
		}
		else if (shouldShow && !m_taskCards.contains(taskId)) {
			addTaskCard(taskInfo);
		}
	}
}