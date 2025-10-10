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

class DownloadQueuePage : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadQueuePage(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	~DownloadQueuePage();

private slots:
	void onDownloadAdded(const QString& taskId);
	void onDownloadRemoved(const QString& taskId);
	void onDownloadStatusChanged(const QString& taskId);
	void updateTaskList();

private:
	void initUI();
	void addTaskCard(const DownloadTaskInfo& taskInfo);
	void removeTaskCard(const QString& taskId);

private:
	QSharedPointer<DownloadManager> m_downloadManager;
	QVBoxLayout* m_mainLayout;
	AntScrollArea* m_scrollArea;
	QWidget* m_scrollWidget;
	QVBoxLayout* m_scrollLayout;
	QMap<QString, DownloadCard*> m_taskCards;
	NoDataWidget* m_noDataWidget;
};