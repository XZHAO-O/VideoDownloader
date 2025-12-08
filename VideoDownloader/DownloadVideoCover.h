#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>

// 表名: download_video_cover
class DownloadVideoCover
{
public:
    DownloadVideoCover() = default;

    QString taskId; // 任务ID，主键
    QByteArray cover; // 封面图片数据
    QDateTime createdTime; // 创建时间
    QDateTime updatedTime; // 更新时间

    bool isValid() const { return !taskId.isEmpty(); }
};
