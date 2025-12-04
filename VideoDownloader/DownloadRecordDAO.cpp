#include "DownloadRecordDAO.h"

#include <QSqlError>
#include <QDebug>

#include "DatabaseManager.h"

// 编译时常量定义
namespace {
    const QString TABLE_NAME = "download_record";

    // SQL语句模板
    const QString CREATE_TABLE_SQL = 
        "CREATE TABLE IF NOT EXISTS download_record ("
        "    taskId TEXT NOT NULL PRIMARY KEY,"
        "    videoId TEXT,"
        "    title TEXT,"
        "    sectionName TEXT,"
        "    author TEXT,"
        "    duration TEXT,"
        "    publishTime TEXT,"
        "    selectedVideoQuality TEXT,"
        "    selectedAudioQuality TEXT,"
        "    downloadFilePath TEXT,"
        "    endTime DATETIME DEFAULT current_timestamp,"
        "    createdTime DATETIME NOT NULL DEFAULT current_timestamp,"
        "    updatedTime DATETIME NOT NULL DEFAULT current_timestamp"
        ");";

    const QString DROP_TABLE_SQL = "DROP TABLE IF EXISTS %1";

    const QString INSERT_SQL = 
        "INSERT OR REPLACE INTO download_record "
        "(taskId, videoId, title, sectionName, author, duration, publishTime, selectedVideoQuality, selectedAudioQuality, downloadFilePath, endTime, createdTime, updatedTime) "
        "VALUES "
        "(:taskId, :videoId, :title, :sectionName, :author, :duration, :publishTime, :selectedVideoQuality, :selectedAudioQuality, :downloadFilePath, :endTime, :createdTime, :updatedTime)";

    const QString DELETE_SQL = "DELETE FROM download_record WHERE taskId = ?";

    const QString SELECT_BY_ID_SQL = "SELECT * FROM download_record WHERE taskId = ?";

    const QString SELECT_ALL_SQL = "SELECT * FROM download_record ORDER BY createdTime DESC";

    const QString COUNT_SQL = "SELECT COUNT(*) FROM download_record";

    const QString SELECT_PAGE_SQL = 
        "SELECT * FROM download_record ORDER BY createdTime DESC LIMIT ? OFFSET ?";
}

DownloadRecordDAO::DownloadRecordDAO(QSharedPointer<DatabaseManager> dbManager)
    : m_dbManager(dbManager)
{
    createTable();
}

DownloadRecordDAO::~DownloadRecordDAO()
{
}

bool DownloadRecordDAO::createTable()
{
    return m_dbManager->executeQuery(CREATE_TABLE_SQL);
}

bool DownloadRecordDAO::dropTable()
{
    QString dropTableSQL = DROP_TABLE_SQL.arg(TABLE_NAME);
    return m_dbManager->executeQuery(dropTableSQL);
}

QVariantMap DownloadRecordDAO::toMap(const DownloadRecord& downloadRecord)
{
    QVariantMap map;
    map[":taskId"] = downloadRecord.taskId;
    map[":videoId"] = downloadRecord.videoId;
    map[":title"] = downloadRecord.title;
    map[":sectionName"] = downloadRecord.sectionName;
    map[":author"] = downloadRecord.author;
    map[":duration"] = downloadRecord.duration;
    map[":publishTime"] = downloadRecord.publishTime;
    map[":selectedVideoQuality"] = downloadRecord.selectedVideoQuality;
    map[":selectedAudioQuality"] = downloadRecord.selectedAudioQuality;
    map[":downloadFilePath"] = downloadRecord.downloadFilePath;
    map[":endTime"] = downloadRecord.endTime;
    map[":createdTime"] = downloadRecord.createdTime;
    map[":updatedTime"] = downloadRecord.updatedTime;

    return map;
}

void DownloadRecordDAO::fillFromQuery(const QSqlQuery& query, DownloadRecord& downloadRecord)
{
    downloadRecord.taskId = query.value("taskId").toString();
    downloadRecord.videoId = query.value("videoId").toString();
    downloadRecord.title = query.value("title").toString();
    downloadRecord.sectionName = query.value("sectionName").toString();
    downloadRecord.author = query.value("author").toString();
    downloadRecord.duration = query.value("duration").toString();
    downloadRecord.publishTime = query.value("publishTime").toString();
    downloadRecord.selectedVideoQuality = query.value("selectedVideoQuality").toString();
    downloadRecord.selectedAudioQuality = query.value("selectedAudioQuality").toString();
    downloadRecord.downloadFilePath = query.value("downloadFilePath").toString();
    downloadRecord.endTime = query.value("endTime").toDateTime();
    downloadRecord.createdTime = query.value("createdTime").toDateTime();
    downloadRecord.updatedTime = query.value("updatedTime").toDateTime();
}

bool DownloadRecordDAO::insert(const DownloadRecord& downloadRecord)
{
    QVariantMap params = toMap(downloadRecord);
    QVariantList paramList;
    QStringList paramNames;

    for (auto it = params.constBegin(); it != params.constEnd(); ++it)
    {
        paramNames << it.key().mid(1); // 移除前面的冒号
        paramList << it.value();
    }

    // 重新构建SQL语句以使用位置参数
    QString preparedSQL = INSERT_SQL;
    for (int i = 0; i < paramNames.size(); ++i)
    {
        QString placeholder = ":" + paramNames[i];
        preparedSQL = preparedSQL.replace(placeholder, "?");
    }

    return m_dbManager->executeQuery(preparedSQL, paramList);
}

bool DownloadRecordDAO::update(const DownloadRecord& downloadRecord)
{
    return insert(downloadRecord); // SQLite的INSERT OR REPLACE已经实现了更新功能
}

bool DownloadRecordDAO::remove(const QString& taskId)
{
    QVariantList params;
    params << taskId;

    return m_dbManager->executeQuery(DELETE_SQL, params);
}

bool DownloadRecordDAO::get(const QString& taskId, DownloadRecord& downloadRecord)
{
    bool success = false;
    QVariantList params;
    params << taskId;

    success = m_dbManager->executeSelect(SELECT_BY_ID_SQL, params,
        [&](QSqlQuery& query) {
            if (query.next())
            {
                fillFromQuery(query, downloadRecord);
                success = true;
            }
        });

    return success;
}

QList<DownloadRecord> DownloadRecordDAO::getAll()
{
    QList<DownloadRecord> downloadRecords;

    m_dbManager->executeSelect(
        SELECT_ALL_SQL,
        QVariantList(),
        [&](QSqlQuery& query) {
            while (query.next())
            {
                DownloadRecord downloadRecord;
                fillFromQuery(query, downloadRecord);
                downloadRecords.append(downloadRecord);
            }
        });

    return downloadRecords;
}

bool DownloadRecordDAO::insertBatch(const QList<DownloadRecord>& downloadRecords)
{
    if (!m_dbManager->beginTransaction())
    {
        return false;
    }

    for (const auto& downloadRecord : downloadRecords)
    {
        if (!insert(downloadRecord))
        {
            m_dbManager->rollbackTransaction();
            return false;
        }
    }

    return m_dbManager->commitTransaction();
}

bool DownloadRecordDAO::executeQuery(const QString& queryStr, const QVariantList& params)
{
    return m_dbManager->executeQuery(queryStr, params);
}

bool DownloadRecordDAO::executeSelect(const QString& queryStr,
    const QVariantList& params,
    std::function<void(QSqlQuery&)> resultProcessor)
{
    return m_dbManager->executeSelect(queryStr, params, resultProcessor);
}

int DownloadRecordDAO::count()
{
    int result = 0;

    m_dbManager->executeSelect(
        COUNT_SQL,
        QVariantList(),
        [&](QSqlQuery& query) {
            if (query.next())
            {
                result = query.value(0).toInt();
            }
        });

    return result;
}

QList<DownloadRecord> DownloadRecordDAO::getPage(int page, int pageSize)
{
    QList<DownloadRecord> downloadRecords;

    if (page < 1 || pageSize < 1)
    {
        return downloadRecords;
    }

    int offset = (page - 1) * pageSize;
    QVariantList params;
    params << pageSize << offset;

    m_dbManager->executeSelect(
        SELECT_PAGE_SQL,
        params,
        [&](QSqlQuery& query) {
            while (query.next())
            {
                DownloadRecord downloadRecord;
                fillFromQuery(query, downloadRecord);
                downloadRecords.append(downloadRecord);
            }
        });

    return downloadRecords;
}
