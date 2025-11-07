#pragma once

#include "DownloadCard.h"
#include "DownloadTaskInfo.h"

class QVBoxLayout;
class NoDataWidget;
class AntScrollArea;
class PaginationWidget;
class MaterialSpinner;
class DownloadCardPool;
class DownloadCard;
class DownloadEngine;

// 容器状态枚举
enum class ContainerState
{
	DownloadReady, // 待下载
	Downloading,   // 下载中
	Downloaded     // 已下载
};

class DownloadCardContainerWidget : public QWidget
{
	Q_OBJECT

public:
	explicit DownloadCardContainerWidget(QSharedPointer<DownloadEngine> downloadEngine,
		ContainerState state,
		QWidget* parent = nullptr);
	~DownloadCardContainerWidget();

	// 公共接口
	void addDownloadCard(QSharedPointer<DownloadTaskInfo> downloadTaskInfo);
	void showLoading();
	void addDownloadCards(QList<QSharedPointer<DownloadTaskInfo>> tasks);
	ContainerState state() const { return m_containerState; }

	// 任务转移相关方法
	void transferTaskToThis(QSharedPointer<DownloadTaskInfo> taskInfo);
	void removeTask(const QString& taskId);
	QSharedPointer<DownloadTaskInfo> getTaskInfo(const QString& taskId) const;

	// 更新显示当前页的卡片
	void updateCurrentPageCards();

signals:
	// 任务状态改变信号
	void taskStateChanged(const QString& taskId, ContainerState newState);

public slots:
	void onDownloadAdded(const QString& taskId);
	void onDownloadRemoved(const QString& taskId);
	void onDownloadStatusChanged(const QString& taskId);
	void onDownloadCompleted(const QString& taskId);
	void onDownloadFailed(const QString& taskId, const QString& error);
	void onDownloadProgress(const QString& taskId, const QString& progressInfo, int progress, const QString& downloadSpeed);

	// 分页改变槽函数
	void onPageChanged(int page);

private:
	// 保护方法
	void initUI();
	void downloadVideo(const QUrl& url);
	void updateVisibility();

	// 根据状态设置卡片连接
	void setupCardConnections(DownloadCard* card, QSharedPointer<DownloadTaskInfo> taskInfo);

	// 获取无数据文本
	QString getNoDataText() const;

	// 设置无数据文本
	void setNoDataText(const QString& text) { m_noDataText = text; }

	// 清理当前显示的卡片（只隐藏，不删除）
	void clearCurrentCards();

	// 清理所有卡片（包括预创建的）
	void clearAllCards();

	QSharedPointer<DownloadEngine> m_downloadEngine;
	ContainerState m_containerState;
	QVBoxLayout* m_mainLayout;
	AntScrollArea* m_scrollArea;
	QWidget* m_scrollWidget;
	QVBoxLayout* m_scrollLayout;
	QHash<QString, DownloadCard*> m_downloadCards;        // 当前显示的卡片，使用taskId作为键
	QList<QSharedPointer<DownloadTaskInfo>> m_downloadTasks;     // 所有任务信息
	NoDataWidget* m_noDataWidget;
	PaginationWidget* m_paginationWidget;        // 分页器
	MaterialSpinner* m_spinner;
	QString m_noDataText;

	// 分页相关
	int m_currentPage = 1;
	int m_pageSize = 10; // 每页显示10个卡片

	// 预创建的卡片列表
	QList<DownloadCard*> m_precreatedCards;

	qint64 m_currentSpeed;
};