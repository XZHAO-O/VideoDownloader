#pragma once

#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

#include "AntProfileTable.h"
#include "VideoInfo.h"
#include "DownloadCardContainerWidget.h"

class AntScrollArea;
class SkeletonWidget;
class AntTabWidgetContainer;
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

	void getVideoPlayUrl(DownloadTaskInfo& taskInfo);

	void createDownloadCards(const QList<VideoInfo>& videoInfoList);

signals:
	void resized(int w, int h);				// 用于通知其他组件调整大小
	void windowMoved(QPoint globalPos);		// 窗口移动时发出信号

protected:
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void onTaskStateChanged(const QString& taskId, ContainerState newState);
	void onDownloadManagerCompleted(const QString& taskId, const QString& filePath);
	void onDownloadManagerStarted(const QString& taskId);

private:
	void initViewPage();

	QSharedPointer<ApplicationController> m_applicationController;
	QSharedPointer<DownloadManager> m_downloadManager;
	MaterialTabWidget* tabWidget = nullptr;
	DownloadCardContainerWidget* downloadReadyWidget = nullptr;
	DownloadCardContainerWidget* downloadingWidget = nullptr;
	DownloadCardContainerWidget* downloadedWidget = nullptr;
	AntScrollArea* scrollArea1 = nullptr;
	AntScrollArea* scrollArea2 = nullptr;
	QList<SkeletonWidget*> skeletons;
	QVector<AntProfileTable::TableColumnItems> rowItems;
	AntProfileTable* table = nullptr;
	QStandardItemModel* tableModel = nullptr;
	AntTabWidgetContainer* antTabContainer = nullptr;
	QWidget* container = nullptr;

	QMutex m_mutex;
};
