#pragma once

#include <QWidget>
#include <QEvent>
#include "MaterialTabWidget.h"
#include "DownloadQueuePage.h"
#include "DownloadingWidget.h"
#include "DownloadedWidget.h"
#include "AntScrollArea.h"
#include "CarouselWidget.h"
#include "SkeletonWidget.h"
#include "AntProfileTable.h"
#include "TabContentWidget.h"
#include "AntTabWidgetContainer.h"
#include "DownloadManager.h"

class DownloadPage : public QWidget
{
	Q_OBJECT

public:
	DownloadPage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadPage();
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
	DownloadQueuePage* downloadQueuePage = nullptr;
	DownloadingWidget* downloadingWidget = nullptr;
	DownloadedWidget* downloadedWidget = nullptr;
	AntScrollArea* scrollArea1 = nullptr;
	AntScrollArea* scrollArea2 = nullptr;
	QList<SkeletonWidget*> skeletons;
	QVector<AntProfileTable::TableColumnItems> rowItems;
	AntProfileTable* table = nullptr;
	QStandardItemModel* tableModel = nullptr;
	AntTabWidgetContainer* antTabContainer = nullptr;
	QWidget* container = nullptr;
};
