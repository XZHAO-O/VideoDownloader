#pragma once

#include <QString>

class StringUtil
{
public:
	StringUtil();
	~StringUtil();
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

	static QString formatDuration(qint64 seconds)
	{
		if (seconds < 0) return "  未知  ";

		if (seconds < 60)
		{
			return QString("%1s").arg(seconds, 2, 10, QChar(' ')).leftJustified(8, ' ');
		}
		else if (seconds < 3600)
		{
			return QString("%1:%2")
				.arg(seconds / 60, 2, 10, QChar(' '))
				.arg(seconds % 60, 2, 10, QChar('0'))
				.leftJustified(8, ' ');
		}
		else
		{
			return QString("%1:%2:%3")
				.arg(seconds / 3600, 2, 10, QChar('0'))
				.arg((seconds % 3600) / 60, 2, 10, QChar('0'))
				.arg(seconds % 60, 2, 10, QChar('0'));
		}
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
		qint64 days = dateTime.daysTo(now);

		if (days == 0)
		{
			return dateTime.toString("今天 hh:mm");
		}
		else if (days == 1)
		{
			return dateTime.toString("昨天 hh:mm");
		}
		else if (days < 7)
		{
			return QString("%1天前").arg(days);
		}
		else
		{
			return dateTime.toString("yyyy-MM-dd");
		}
	}

private:
	static constexpr qint64 KB = 1024;
	static constexpr qint64 MB = 1024 * KB;
	static constexpr qint64 GB = 1024 * MB;
};