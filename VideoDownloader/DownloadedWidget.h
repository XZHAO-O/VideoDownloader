#pragma once

#include "DownloadCardContainerWidget.h"

class DownloadedWidget : public DownloadCardContainerWidget
{
	Q_OBJECT

public:
	explicit DownloadedWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadedWidget();

protected:
	// 实现纯虚函数
	QList<DownloadTaskInfo> getTaskList() const override;
	void setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo) override;
	QString getNoDataText() const;

protected slots:
	void onDownloadCompleted(const QString& taskId, const QString& filePath);
	void onDownloadFailed(const QString& taskId, const QString& error);
};