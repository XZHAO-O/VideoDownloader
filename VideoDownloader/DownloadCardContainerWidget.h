#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMap>
#include <QSharedPointer>
#include "DownloadCard.h"
#include "DownloadManager.h"
#include "NoDataWidget.h"
#include "AntScrollArea.h"

class DownloadCardContainerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCardContainerWidget(QSharedPointer<DownloadManager> downloadManager, QWidget* parent = nullptr);
	virtual ~DownloadCardContainerWidget();

	// 公共接口
	void addDownloadCard(DownloadCard* downloadCard);
	void updateTaskList();

protected:
	// 保护成员变量，派生类可以访问
	QSharedPointer<DownloadManager> m_downloadManager;
	QVBoxLayout* m_mainLayout;
	AntScrollArea* m_scrollArea;
	QWidget* m_scrollWidget;
	QVBoxLayout* m_scrollLayout;
	QMap<QString, DownloadCard*> m_taskCards;
	QList<DownloadCard*> m_downloadCards; // 用于手动添加的卡片
	NoDataWidget* m_noDataWidget;
	QString m_noDataText; // 存储无数据文本

	// 保护方法，派生类可以调用
	void initUI();
	void addTaskCard(const DownloadTaskInfo& taskInfo);
	void removeTaskCard(const QString& taskId);
	void downloadVideo(const QString& url);
	void updateVisibility();

	// 虚函数，派生类可以重写
	virtual QList<DownloadTaskInfo> getTaskList() const = 0;
	virtual void setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo) = 0;

	// 设置无数据文本的方法，避免在构造函数中调用纯虚函数
	void setNoDataText(const QString& text) { m_noDataText = text; }

protected slots:
	// 保护槽函数，派生类可以重写
	virtual void onDownloadAdded(const QString& taskId);
	virtual void onDownloadRemoved(const QString& taskId);
	virtual void onDownloadStatusChanged(const QString& taskId);
};