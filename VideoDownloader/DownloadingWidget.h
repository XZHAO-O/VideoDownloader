#pragma once

#include "DownloadCardContainerWidget.h"

class DownloadingWidget : public DownloadCardContainerWidget
{
	Q_OBJECT

public:
	explicit DownloadingWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadingWidget();

protected:
	// 实现纯虚函数
	QList<DownloadTaskInfo> getTaskList() const override;
	void setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo) override;
	QString getNoDataText() const;

protected slots:
	void onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void onDownloadStatusChanged(const QString& taskId) override;
	void onDownloadSpeedUpdated(qint64 bytesPerSecond);

private:
	void updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total);

private:
	qint64 m_currentSpeed;
};