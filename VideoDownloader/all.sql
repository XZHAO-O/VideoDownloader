CREATE TABLE IF NOT EXISTS download_record (
    taskId TEXT NOT NULL PRIMARY KEY,     -- 任务ID，主键
    videoId TEXT,                         -- 视频ID
    title TEXT,                           -- 视频标题
    sectionName TEXT,                     -- 分区名称
    author TEXT,                          -- 作者
    duration TEXT,                        -- 视频时长
    publishTime TEXT,                     -- 发布时间
    selectedVideoQuality TEXT,            -- 选择的视频质量
    selectedAudioQuality TEXT,            -- 选择的音频质量
    downloadFilePath TEXT,                -- 下载文件路径
    endTime DATETIME DEFAULT CURRENT_TIMESTAMP,                -- 下载完成时间
    createdTime DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,   -- 创建时间
    updatedTime DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP    -- 更新时间
);