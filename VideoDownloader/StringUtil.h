#pragma once

#include <QRegularExpression>
#include <QUuid>
#include <QRandomGenerator>

class StringUtil
{
public:
	StringUtil();
	~StringUtil();

	static QString generateId(const QString& headName)
	{
		using namespace std::chrono;

		// 微秒级时间戳（性能与精度的平衡）
		auto now = high_resolution_clock::now();
		auto micros = duration_cast<microseconds>(now.time_since_epoch()).count();

		// 紧凑型UUID（移除分隔符和连字符）
		QString uuid = QUuid::createUuid().toString(QUuid::Id128);

		// 额外随机数
		uint32_t randomNum = QRandomGenerator::global()->generate();

		return headName + QString("_%1_%2_%3")
			.arg(micros)
			.arg(uuid)
			.arg(randomNum, 8, 16, QChar('0'));
	}

	static QString formatFileName(const QString& fileName)
	{
		// 移除或替换非法字符
		QString cleanName = fileName;

		// 根据编译环境定义不同的非法字符规则
		#ifdef Q_OS_WIN
		// Windows 环境下的非法字符: < > : " / \ | ? *
		QRegularExpression invalidChars("[<>:\"/\\\\|?*]");
		#elif defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
		// Linux/Unix 环境下的非法字符: / 和空字符
		// 注意：Linux 实际上允许更多字符，但为了安全还是过滤一些特殊字符
		QRegularExpression invalidChars("[\\x00-\\x1F\\x7F/]"); // 控制字符和斜杠
		#else
		// 其他平台的默认规则
		QRegularExpression invalidChars("[<>:\"/\\\\|?*\\x00-\\x1F\\x7F]");
		#endif

		// 移除首尾空格和点
		return cleanName.replace(invalidChars, "_").trimmed();
	}

	// 格式化文件大小
	static QString formatFileSize(qint64 bytes)
	{
		if (bytes >= GB)
			return QString("%1 GB").arg(QString::number(bytes / static_cast<double>(GB), 'f', 2));
		else if (bytes >= MB)
			return QString("%1 MB").arg(QString::number(bytes / static_cast<double>(MB), 'f', 2));
		else if (bytes >= KB)
			return QString("%1 KB").arg(QString::number(bytes / KB), 6);
		else
			return QString("%1 B ").arg(QString::number(bytes), 6);
	}

	static QString formatDownloadProgress(qint64 downloadedSize, qint64 fileSize)
	{
		if (fileSize > 0)
		{
			return QString("%1/%2").arg(formatFileSize(downloadedSize)).arg(formatFileSize(fileSize));
		}
		else
		{
			return QString("%1").arg(formatFileSize(downloadedSize));
		}
	}

	static QString formatDownloadSpeed(qint64 speed)
	{
		if (speed >= MB)
		{
			return QString("%1 MB/s").arg(QString::number(speed / static_cast<double>(MB), 'f', 2));
		}
		else if (speed >= KB)
		{
			return QString("%1 KB/s").arg(QString::number(speed / KB), 6);
		}
		else
		{
			return QString("%1 B/s").arg(QString::number(speed), 6);
		}
	}

	static QString formatDuration(const QString& duration)
	{
		return formatDuration(duration.toLongLong());
	}

	static QString formatDuration(qint64 totalSeconds)
	{
		int hours = totalSeconds / 3600;
		int minutes = (totalSeconds % 3600) / 60;
		int seconds = totalSeconds % 60;

		if (hours > 0)
			return QString("%1:%2:%3")
			.arg(hours, 2, 10, QLatin1Char('0'))
			.arg(minutes, 2, 10, QLatin1Char('0'))
			.arg(seconds, 2, 10, QLatin1Char('0'));
		else
			return QString("%1:%2")
			.arg(minutes, 2, 10, QLatin1Char('0'))
			.arg(seconds, 2, 10, QLatin1Char('0'));
	}

	static QString formatDateTime(const QString& dutation)
	{
		return formatDateTime(dutation.toLongLong());
	}

	static QString formatDateTime(qint64 duration)
	{
		QDateTime dateTime = QDateTime::fromSecsSinceEpoch(duration);
		return formatDateTime(dateTime);
	}

	static QString formatDateTime(const QDateTime& dateTime)
	{
		if (!dateTime.isValid()) return "";

		QDateTime now = QDateTime::currentDateTime();

		// 本地时间
		// 计算时间差
		qint64 secs = dateTime.secsTo(now);
		if (secs < 0) return dateTime.toString("yyyy-MM-dd"); // 未来时间

		if (secs < 60)
		{
			return "刚刚";
		}
		else if (secs < 3600)
		{
			return QString("%1分钟前").arg(secs / 60);
		}
		else if (secs < 86400)
		{
			return QString("%1小时前").arg(secs / 3600);
		}

		qint64 days = dateTime.daysTo(now);

		if (days == 1)
		{
			return dateTime.toString("昨天 HH:mm");
		}
		else if (days < 7)
		{
			return QString("%1天前").arg(days);
		}
		else if (days < 30)
		{
			return QString("%1周前").arg(days / 7);
		}
		else
		{
			return dateTime.toString("yyyy-MM-dd");
		}
	}

	static constexpr unsigned int MAX_FILENAME_LENGTH = 255;

	static constexpr qint64 KB = 1024;
	static constexpr qint64 MB = 1024 * KB;
	static constexpr qint64 GB = 1024 * MB;
};