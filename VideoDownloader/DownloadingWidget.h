#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QList>
#include <QSharedPointer>
#include "DownloadCard.h"
#include "DownloadManager.h"
#include "NoDataWidget.h"
#include "AntScrollArea.h"

class DownloadingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadingWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadingWidget();

private slots:
	void onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void onDownloadStatusChanged(const QString& taskId);
	void onDownloadSpeedUpdated(qint64 bytesPerSecond);
	void updateTaskList();

private:
	void initUI();
	void addTaskCard(const DownloadTaskInfo& taskInfo);
	void removeTaskCard(const QString& taskId);
	void updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total);

private:
	QSharedPointer<DownloadManager> m_downloadManager;
	QVBoxLayout* m_mainLayout;
	AntScrollArea* m_scrollArea;
	QWidget* m_scrollWidget;
	QVBoxLayout* m_scrollLayout;
	QMap<QString, DownloadCard*> m_taskCards;
	NoDataWidget* m_noDataWidget;
	qint64 m_currentSpeed;
};