#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QFuture>
#include "MediaInfo.h"

class IMediaProcessor : public QObject
{
	Q_OBJECT

public:
	// 添加带参数的构造函数
	explicit IMediaProcessor(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~IMediaProcessor() = default;

	// 基本媒体处理
	virtual QFuture<bool> mergeVideoAudio(const QString& videoPath,
		const QString& audioPath,
		const QString& outputPath) = 0;

	virtual QFuture<bool> extractAudio(const QString& videoPath,
		const QString& outputPath) = 0;

	virtual QFuture<MediaInfo> getMediaInfo(const QString& filePath) = 0;

	virtual QFuture<QImage> getVideoThumbnail(const QString& videoPath,
		int timestampMs = 0) = 0;

	// 高级功能
	virtual QFuture<bool> addSubtitles(const QString& videoPath,
		const QString& subtitlePath,
		const QString& outputPath) = 0;

	virtual QFuture<bool> convertFormat(const QString& inputPath,
		const QString& outputPath,
		const QString& format) = 0;

	virtual QFuture<bool> compressVideo(const QString& inputPath,
		const QString& outputPath,
		int quality) = 0;

	// 工具方法
	virtual bool supportsFormat(const QString& format) const = 0;
	virtual QList<QString> supportedFormats() const = 0;
	virtual bool isAvailable() const = 0;

signals:
	void processingProgress(const QString& operation, int percentage);
	void processingCompleted(const QString& operation, bool success);
	void processingFailed(const QString& operation, const QString& error);
};