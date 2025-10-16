#pragma once

#include "DownloadCardContainerWidget.h"

class DownloadQueuePage : public DownloadCardContainerWidget
{
	Q_OBJECT

public:
	explicit DownloadQueuePage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadQueuePage();

protected:
	// 实现纯虚函数
	QList<DownloadTaskInfo> getTaskList() const override;
	void setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo) override;
	QString getNoDataText() const;

protected slots:
	void onDownloadStatusChanged(const QString& taskId) override;
};