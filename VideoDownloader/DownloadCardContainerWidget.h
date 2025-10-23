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
#include "PaginationWidget.h"
#include "DownloadCardPool.h"

// 容器状态枚举
enum class ContainerState {
	DownloadReady, // 待下载
	Downloading,   // 下载中
	Downloaded     // 已下载
};

class DownloadCardContainerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCardContainerWidget(QSharedPointer<DownloadManager> downloadManager,
		ContainerState state,
		QWidget* parent = nullptr);
	virtual ~DownloadCardContainerWidget();

	// 公共接口
	void addDownloadCard(DownloadTaskInfo downloadTaskInfo);
	void setState(ContainerState state);
	ContainerState state() const { return m_containerState; }

	// 更新显示当前页的卡片
	void updateCurrentPageCards();

protected:
	// 保护成员变量
	QSharedPointer<DownloadManager> m_downloadManager;
	ContainerState m_containerState;
	QVBoxLayout* m_mainLayout;
	AntScrollArea* m_scrollArea;
	QWidget* m_scrollWidget;
	QVBoxLayout* m_scrollLayout;
	QList<DownloadCard*> m_downloadCards;        // 当前显示的卡片
	QList<DownloadTaskInfo> m_downloadTasks;     // 所有任务信息
	NoDataWidget* m_noDataWidget;
	PaginationWidget* m_paginationWidget;        // 分页器
	DownloadCardPool* m_cardPool;                // 卡片池
	QString m_noDataText;

	// 分页相关
	int m_currentPage = 1;
	int m_pageSize = 10; // 每页显示10个卡片

	// 保护方法
	void initUI();
	void downloadVideo(const QUrl& url);
	void updateVisibility();

	// 根据状态设置卡片连接
	void setupCardConnections(DownloadCard* card, const DownloadTaskInfo& taskInfo);

	// 获取无数据文本
	QString getNoDataText() const;

	// 设置无数据文本
	void setNoDataText(const QString& text) { m_noDataText = text; }

	// 清理当前显示的卡片
	void clearCurrentCards();

protected slots:
	// 保护槽函数
	void onDownloadAdded(const QString& taskId);
	void onDownloadRemoved(const QString& taskId);
	void onDownloadStatusChanged(const QString& taskId);
	void onDownloadCompleted(const QString& taskId, const QString& filePath);
	void onDownloadFailed(const QString& taskId, const QString& error);
	void onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void onDownloadSpeedUpdated(qint64 bytesPerSecond);

	// 分页改变槽函数
	void onPageChanged(int page);

private:
	void updateTaskProgress(const QString& taskId, qint64 downloaded, qint64 total);
	qint64 m_currentSpeed;
};