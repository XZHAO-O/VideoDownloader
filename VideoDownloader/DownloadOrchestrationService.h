// DownloadOrchestrationService.h
#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QFuture>
#include "ModManager.h"
#include "INetworkManager.h"
#include "IMediaProcessor.h"
#include "DownloadRecordRepository.h"
#include "VideoDownloadRequest.h"

class DownloadOrchestrationService : public QObject
{
	Q_OBJECT

public:
	explicit DownloadOrchestrationService(QSharedPointer<ModManager> modManager,
		QSharedPointer<INetworkManager> networkManager,
		QSharedPointer<IMediaProcessor> mediaProcessor,
		QSharedPointer<DownloadRecordRepository> recordRepository,
		QObject* parent = nullptr);

	// 下载管理
	bool startDownload(const VideoDownloadRequest& request);
	bool cancelDownload(const QString& taskId);

	// 状态查询
	QList<QString> getActiveDownloads() const;
	bool isDownloadActive(const QString& taskId) const;

signals:
	void downloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void downloadCompleted(const QString& taskId, const QString& filePath);
	void downloadFailed(const QString& taskId, const QString& error);

private:
	struct DownloadContext {
		VideoDownloadRequest request;
		VideoInfo videoInfo;
		QList<StreamInfo> videoStreams;
		QList<StreamInfo> audioStreams;
		QString videoFilePath;
		QString audioFilePath;
		QString finalFilePath;
		qint64 totalBytes = 0;
		qint64 downloadedBytes = 0;
		bool cancelled = false;
	};

	void executeDownload(const QString& taskId, const VideoDownloadRequest& request);
	void onVideoInfoReceived(const QString& taskId, const VideoInfo& videoInfo);
	void onStreamsReceived(const QString& taskId,
		const QList<StreamInfo>& videoStreams,
		const QList<StreamInfo>& audioStreams);
	void onDownloadProgress(const QString& taskId, qint64 downloaded, qint64 total);
	void onDownloadCompleted(const QString& taskId, const QString& filePath);
	void onDownloadFailed(const QString& taskId, const QString& error);
	void cleanupDownload(const QString& taskId);

	QSharedPointer<ModManager> m_modManager;
	QSharedPointer<INetworkManager> m_networkManager;
	QSharedPointer<IMediaProcessor> m_mediaProcessor;
	QSharedPointer<DownloadRecordRepository> m_recordRepository;

	QMap<QString, DownloadContext> m_activeDownloads;
	QMap<QString, QSharedPointer<IVideoPlatformMod>> m_platformMods;
};