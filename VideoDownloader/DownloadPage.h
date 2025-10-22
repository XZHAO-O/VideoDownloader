#pragma once

#include <QDateTime>

#include "AntProfileTable.h"
#include "VideoInfo.h"

class AntScrollArea;
class SkeletonWidget;
class AntTabWidgetContainer;
class MaterialTabWidget;
class DownloadCardContainerWidget;
class DownloadTaskInfo;
class DownloadManager;

class DownloadPage : public QWidget
{
	Q_OBJECT

public:
	DownloadPage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadPage();

	void getVideoPlayUrl(DownloadTaskInfo& taskInfo);

	void createDownloadCards(QList<VideoInfo> videoInfoList);

protected:
	void resizeEvent(QResizeEvent* event) override;
private:
	void initViewPage();
signals:
	void resized(int w, int h);				// 用于通知其他组件调整大小
	void windowMoved(QPoint globalPos);		// 窗口移动时发出信号
private:
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
};
