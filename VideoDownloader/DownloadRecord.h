#pragma once

#include <QDateTime>
#include <QString>

// 表名: download_record
class DownloadRecord
{
public:
    DownloadRecord() = default;

    QString taskId; // 任务ID，主键
    QString videoId; // 视频ID
    QString url; // 视频链接
    QString title; // 视频标题
    QString sectionName; // 分区名称
    QString author; // 作者
    QString duration; // 视频时长
    QString publishTime; // 发布时间
    QString selectedVideoQuality; // 选择的视频质量
    QString selectedAudioQuality; // 选择的音频质量
    QString downloadFilePath; // 下载文件路径
    QDateTime endTime; // 下载完成时间
    QDateTime createdTime; // 创建时间
    QDateTime updatedTime; // 更新时间

    bool isValid() const { return !taskId.isEmpty(); }
};
