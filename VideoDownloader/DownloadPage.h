#pragma once

#include <QDateTime>
#include <QMutex>

#include "AntProfileTable.h"
#include "DownloadCardContainerWidget.h"
#include "VideoInfo.h"

class AntScrollArea;
class MaterialTabWidget;
class DownloadTaskInfo;
class DownloadManager;
class ApplicationController;

class DownloadPage : public QWidget
{
	Q_OBJECT

public:
	DownloadPage(QSharedPointer<ApplicationController> applicationController, QWidget* parent = nullptr);
	~DownloadPage();

	void getVideoPlayUrl(QSharedPointer<DownloadTaskInfo> taskInfo);

	void getVideoCover(QSharedPointer<DownloadTaskInfo> taskInfo);

	void createDownloadCards(QList<VideoInfo>&& videoInfoList);

signals:
	void resized(int w, int h);				// 用于通知其他组件调整大小
	void windowMoved(QPoint globalPos);		// 窗口移动时发出信号

protected:
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void onTaskStateChanged(const QString& taskId, ContainerState newState);
	void onDownloadManagerCompleted(const QString& taskId, const QString& filePath);

private:
	QSharedPointer<ApplicationController> m_applicationController;
	QSharedPointer<DownloadManager> m_downloadManager;
	MaterialTabWidget* tabWidget = nullptr;
	DownloadCardContainerWidget* downloadReadyWidget = nullptr;
	DownloadCardContainerWidget* downloadingWidget = nullptr;
	DownloadCardContainerWidget* downloadedWidget = nullptr;
	AntScrollArea* scrollArea1 = nullptr;

	QMutex m_mutex;
};
