#pragma once

#include "IMediaProcessor.h"

class ConfigManager;

class MediaProcessingService : public IMediaProcessor
{
	Q_OBJECT

public:
	explicit MediaProcessingService(QSharedPointer<ConfigManager> configManager,
		QObject* parent = nullptr);

	QFuture<bool> mergeVideoAudio(const QString& videoPath,
		const QString& audioPath,
		const QString& outputPath) override;

	QFuture<bool> extractAudio(const QString& videoPath,
		const QString& outputPath) override;

	QFuture<MediaInfo> getMediaInfo(const QString& filePath) override;

	QFuture<QImage> getVideoThumbnail(const QString& videoPath,
		int timestampMs = 0) override;

	// 扩展功能
	QFuture<bool> addSubtitles(const QString& videoPath,
		const QString& subtitlePath,
		const QString& outputPath) override;

	QFuture<bool> convertFormat(const QString& inputPath,
		const QString& outputPath,
		const QString& format) override;

	QFuture<bool> compressVideo(const QString& inputPath,
		const QString& outputPath,
		int quality) override;

	// 工具方法
	bool supportsFormat(const QString& format) const override;
	QList<QString> supportedFormats() const override;
	bool isAvailable() const override;

private:
	bool checkFFmpegAvailability();
	bool executeFFmpeg(const QStringList& arguments);
	QJsonObject executeFFprobe(const QString& filePath);

	QSharedPointer<ConfigManager> m_configManager;
	QString m_ffmpegPath;
	QString m_ffprobePath;
	bool m_ffmpegAvailable = false;
};