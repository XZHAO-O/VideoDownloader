// MediaProcessingService.cpp
#include "MediaProcessingService.h"
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QtConcurrent\QtConcurrent>
#include "LogSystem.h"

MediaProcessingService::MediaProcessingService(QSharedPointer<ConfigManager> configManager,
	QObject* parent)
	: IMediaProcessor(parent)  // 现在应该可以正常工作了
	, m_configManager(configManager)
{
	// 配置FFmpeg路径
	m_ffmpegPath = m_configManager->getValue("media/ffmpegPath", "ffmpeg").toString();
	m_ffprobePath = m_configManager->getValue("media/ffprobePath", "ffprobe").toString();

	m_ffmpegAvailable = checkFFmpegAvailability();

	if (m_ffmpegAvailable) {
		LogSystem::instance().info("FFmpeg is available", "MediaProcessing");
	}
	else {
		LogSystem::instance().warning("FFmpeg is not available", "MediaProcessing");
	}
}

QFuture<bool> MediaProcessingService::mergeVideoAudio(const QString& videoPath,
	const QString& audioPath,
	const QString& outputPath)
{
	return QtConcurrent::run([this, videoPath, audioPath, outputPath]() -> bool {
		if (!m_ffmpegAvailable) {
			LogSystem::instance().error("FFmpeg not available", "MediaProcessing");
			return false;
		}

		QStringList arguments;
		arguments << "-i" << videoPath
			<< "-i" << audioPath
			<< "-c" << "copy"
			<< "-map" << "0:v:0"
			<< "-map" << "1:a:0"
			<< "-y"  // 覆盖输出文件
			<< outputPath;

		bool success = executeFFmpeg(arguments);

		if (success) {
			LogSystem::instance().info(
				QString("Successfully merged video and audio: %1").arg(outputPath),
				"MediaProcessing");
			emit processingCompleted("merge", true);
		}
		else {
			LogSystem::instance().error(
				QString("Failed to merge video and audio: %1").arg(outputPath),
				"MediaProcessing");
			emit processingFailed("merge", "FFmpeg execution failed");
		}

		return success;
		});
}

QFuture<bool> MediaProcessingService::extractAudio(const QString& videoPath,
	const QString& outputPath)
{
	return QtConcurrent::run([this, videoPath, outputPath]() -> bool {
		if (!m_ffmpegAvailable) {
			return false;
		}

		QStringList arguments;
		arguments << "-i" << videoPath
			<< "-vn"  // 禁用视频
			<< "-acodec" << "copy"
			<< "-y"
			<< outputPath;

		bool success = executeFFmpeg(arguments);

		if (success) {
			LogSystem::instance().info(
				QString("Successfully extracted audio: %1").arg(outputPath),
				"MediaProcessing");
			emit processingCompleted("extract_audio", true);
		}
		else {
			LogSystem::instance().error(
				QString("Failed to extract audio: %1").arg(outputPath),
				"MediaProcessing");
			emit processingFailed("extract_audio", "FFmpeg execution failed");
		}

		return success;
		});
}

QFuture<MediaInfo> MediaProcessingService::getMediaInfo(const QString& filePath)
{
	return QtConcurrent::run([this, filePath]() -> MediaInfo {
		if (!m_ffmpegAvailable) {
			return MediaInfo();
		}

		QJsonObject ffprobeOutput = executeFFprobe(filePath);
		if (ffprobeOutput.isEmpty()) {
			return MediaInfo();
		}

		return MediaInfo::fromFFprobeJson(ffprobeOutput);
		});
}

QFuture<QImage> MediaProcessingService::getVideoThumbnail(const QString& videoPath,
	int timestampMs)
{
	return QtConcurrent::run([this, videoPath, timestampMs]() -> QImage {
		if (!m_ffmpegAvailable) {
			return QImage();
		}

		// 创建临时文件
		QTemporaryFile tempFile;
		if (!tempFile.open()) {
			return QImage();
		}

		QString tempPath = tempFile.fileName() + ".jpg";

		QStringList arguments;
		arguments << "-ss" << QString::number(timestampMs / 1000.0)
			<< "-i" << videoPath
			<< "-vframes" << "1"
			<< "-q:v" << "2"
			<< "-y"
			<< tempPath;

		if (executeFFmpeg(arguments)) {
			QImage image(tempPath);
			QFile::remove(tempPath);
			return image;
		}

		return QImage();
		});
}

QFuture<bool> MediaProcessingService::addSubtitles(const QString& videoPath,
	const QString& subtitlePath,
	const QString& outputPath)
{
	return QtConcurrent::run([this, videoPath, subtitlePath, outputPath]() -> bool {
		if (!m_ffmpegAvailable) {
			return false;
		}

		QStringList arguments;
		arguments << "-i" << videoPath
			<< "-vf" << QString("subtitles=%1").arg(subtitlePath)
			<< "-c:a" << "copy"
			<< "-y"
			<< outputPath;

		bool success = executeFFmpeg(arguments);

		if (success) {
			LogSystem::instance().info(
				QString("Successfully added subtitles: %1").arg(outputPath),
				"MediaProcessing");
			emit processingCompleted("add_subtitles", true);
		}
		else {
			LogSystem::instance().error(
				QString("Failed to add subtitles: %1").arg(outputPath),
				"MediaProcessing");
			emit processingFailed("add_subtitles", "FFmpeg execution failed");
		}

		return success;
		});
}

QFuture<bool> MediaProcessingService::convertFormat(const QString& inputPath,
	const QString& outputPath,
	const QString& format)
{
	return QtConcurrent::run([this, inputPath, outputPath, format]() -> bool {
		if (!m_ffmpegAvailable) {
			return false;
		}

		QStringList arguments;
		arguments << "-i" << inputPath
			<< "-y"
			<< outputPath;

		bool success = executeFFmpeg(arguments);

		if (success) {
			LogSystem::instance().info(
				QString("Successfully converted format: %1").arg(outputPath),
				"MediaProcessing");
			emit processingCompleted("convert_format", true);
		}
		else {
			LogSystem::instance().error(
				QString("Failed to convert format: %1").arg(outputPath),
				"MediaProcessing");
			emit processingFailed("convert_format", "FFmpeg execution failed");
		}

		return success;
		});
}

QFuture<bool> MediaProcessingService::compressVideo(const QString& inputPath,
	const QString& outputPath,
	int quality)
{
	return QtConcurrent::run([this, inputPath, outputPath, quality]() -> bool {
		if (!m_ffmpegAvailable) {
			return false;
		}

		QStringList arguments;
		arguments << "-i" << inputPath
			<< "-c:v" << "libx264"
			<< "-crf" << QString::number(quality)
			<< "-c:a" << "copy"
			<< "-y"
			<< outputPath;

		bool success = executeFFmpeg(arguments);

		if (success) {
			LogSystem::instance().info(
				QString("Successfully compressed video: %1").arg(outputPath),
				"MediaProcessing");
			emit processingCompleted("compress_video", true);
		}
		else {
			LogSystem::instance().error(
				QString("Failed to compress video: %1").arg(outputPath),
				"MediaProcessing");
			emit processingFailed("compress_video", "FFmpeg execution failed");
		}

		return success;
		});
}

bool MediaProcessingService::supportsFormat(const QString& format) const
{
	// 支持常见视频格式
	static const QSet<QString> supportedFormats = {
		"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v"
	};
	return supportedFormats.contains(format.toLower());
}

QList<QString> MediaProcessingService::supportedFormats() const
{
	return {
		"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v",
		"mp3", "aac", "wav", "flac", "ogg"
	};
}

bool MediaProcessingService::isAvailable() const
{
	return m_ffmpegAvailable;
}

bool MediaProcessingService::checkFFmpegAvailability()
{
	QProcess process;
	process.start(m_ffmpegPath, { "-version" });
	if (process.waitForFinished(5000)) {
		return process.exitCode() == 0;
	}
	return false;
}

bool MediaProcessingService::executeFFmpeg(const QStringList& arguments)
{
	QProcess process;
	process.start(m_ffmpegPath, arguments);

	if (!process.waitForFinished(300000)) { // 5分钟超时
		LogSystem::instance().error("FFmpeg process timeout", "MediaProcessing");
		return false;
	}

	return process.exitCode() == 0;
}

QJsonObject MediaProcessingService::executeFFprobe(const QString& filePath)
{
	QProcess process;
	process.start(m_ffprobePath, {
		"-v", "quiet",
		"-print_format", "json",
		"-show_format",
		"-show_streams",
		filePath
		});

	if (!process.waitForFinished(30000)) { // 30秒超时
		LogSystem::instance().error("FFprobe process timeout", "MediaProcessing");
		return QJsonObject();
	}

	if (process.exitCode() != 0) {
		LogSystem::instance().error("FFprobe execution failed", "MediaProcessing");
		return QJsonObject();
	}

	QByteArray output = process.readAllStandardOutput();
	QJsonDocument doc = QJsonDocument::fromJson(output);

	if (doc.isNull()) {
		LogSystem::instance().error("Failed to parse FFprobe output", "MediaProcessing");
		return QJsonObject();
	}

	return doc.object();
}