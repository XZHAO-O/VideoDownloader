#include "DownloadRecordDAO.h"

#include "QueryWrapper.h"
#include "DatabaseManager.h"

// 编译时常量定义
namespace
{
    const QString TABLE_NAME = "download_record";

    // SQL语句模板
    const QString CREATE_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS download_record ("
        "    taskId TEXT NOT NULL PRIMARY KEY,"
        "    videoId TEXT,"
        "    url TEXT,"
        "    title TEXT,"
        "    sectionName TEXT,"
        "    author TEXT,"
        "    duration TEXT,"
        "    publishTime TEXT,"
        "    selectedVideoQuality TEXT,"
        "    selectedAudioQuality TEXT,"
        "    downloadFilePath TEXT,"
        "    endTime DATETIME NOT NULL DEFAULT current_timestamp,"
        "    createdTime DATETIME NOT NULL DEFAULT current_timestamp,"
        "    updatedTime DATETIME NOT NULL DEFAULT current_timestamp"
        ");";

    const QString DROP_TABLE_SQL = "DROP TABLE IF EXISTS download_record";

    const QString INSERT_SQL =
        "INSERT OR REPLACE INTO download_record "
        "(taskId, videoId, url, title, sectionName, author, duration, publishTime, selectedVideoQuality, selectedAudioQuality, downloadFilePath, endTime, createdTime, updatedTime) "
        "VALUES "
        "(:taskId, :videoId, :url, :title, :sectionName, :author, :duration, :publishTime, :selectedVideoQuality, :selectedAudioQuality, :downloadFilePath, :endTime, :createdTime, :updatedTime)";

    const QString DELETE_SQL = "DELETE FROM download_record WHERE taskId = ?";

    const QString SELECT_BY_ID_SQL = "SELECT * FROM download_record WHERE taskId = ?";

    const QString COUNT_SQL = "SELECT COUNT(*) FROM download_record";
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
    return m_dbManager->executeQuery(DROP_TABLE_SQL);
}

QVariantMap DownloadRecordDAO::toMap(const DownloadRecord& downloadRecord)
{
    QVariantMap map;
    map[":taskId"] = downloadRecord.taskId;
    map[":videoId"] = downloadRecord.videoId;
    map[":url"] = downloadRecord.url;
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

void DownloadRecordDAO::fillFromQueryResult(const QVariantMap& result, DownloadRecord& downloadRecord)
{
    downloadRecord.taskId = result.value("taskId").toString();
    downloadRecord.videoId = result.value("videoId").toString();
    downloadRecord.url = result.value("url").toString();
    downloadRecord.title = result.value("title").toString();
    downloadRecord.sectionName = result.value("sectionName").toString();
    downloadRecord.author = result.value("author").toString();
    downloadRecord.duration = result.value("duration").toString();
    downloadRecord.publishTime = result.value("publishTime").toString();
    downloadRecord.selectedVideoQuality = result.value("selectedVideoQuality").toString();
    downloadRecord.selectedAudioQuality = result.value("selectedAudioQuality").toString();
    downloadRecord.downloadFilePath = result.value("downloadFilePath").toString();
    downloadRecord.endTime = result.value("endTime").toDateTime();
    downloadRecord.createdTime = result.value("createdTime").toDateTime();
    downloadRecord.updatedTime = result.value("updatedTime").toDateTime();
}

bool DownloadRecordDAO::insert(const DownloadRecord& downloadRecord)
{
    return m_dbManager->executeQuery(INSERT_SQL, toMap(downloadRecord));
}

bool DownloadRecordDAO::update(const DownloadRecord& downloadRecord)
{
    return insert(downloadRecord);
}

bool DownloadRecordDAO::remove(const QString& taskId)
{
    QVariantList params;
    params << taskId;

    return m_dbManager->executeQuery(DELETE_SQL, params);
}

QList<DownloadRecord> DownloadRecordDAO::getById(const QString& taskId)
{
    QVariantList params;
    params << taskId;

    return executeSelect(SELECT_BY_ID_SQL, params);
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

int DownloadRecordDAO::count()
{
    int result = 0;
    auto results = m_dbManager->executeQueryToMap(COUNT_SQL);
    if (!results.isEmpty())
    {
        result = results.first().value(0).toInt();
    }
    return result;
}

QList<DownloadRecord> DownloadRecordDAO::selectList(const QueryWrapper& wrapper)
{
    QString sql = wrapper.buildSelectSql(TABLE_NAME);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return QList<DownloadRecord>();
    }

    return executeSelect(sql, params);
}

int DownloadRecordDAO::selectCount(const QueryWrapper& wrapper)
{
    QString sql = wrapper.buildCountSql(TABLE_NAME);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return 0;
    }

    auto results = m_dbManager->executeQueryToMap(sql, params);
    if (!results.isEmpty())
    {
        return results.first().value(0).toInt();
    }
    return 0;
}

bool DownloadRecordDAO::deleteByWrapper(const QueryWrapper& wrapper)
{
    QString sql = wrapper.buildDeleteSql(TABLE_NAME);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return false;
    }

    return m_dbManager->executeQuery(sql, params);
}

bool DownloadRecordDAO::updateByWrapper(const QueryWrapper& wrapper, const QVariantMap& updateFields)
{
    QString sql = wrapper.buildUpdateSql(TABLE_NAME, updateFields);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return false;
    }

    return m_dbManager->executeQuery(sql, params);
}

QList<DownloadRecord> DownloadRecordDAO::selectPage(const QueryWrapper& wrapper, int pageNum, int pageSize)
{
    QueryWrapper pageWrapper = wrapper;
    pageWrapper.limit((pageNum - 1) * pageSize, pageSize);

    return selectList(pageWrapper);
}

bool DownloadRecordDAO::executeQuery(const QString& queryStr, const QVariantList& params)
{
    return m_dbManager->executeQuery(queryStr, params);
}

bool DownloadRecordDAO::executeQuery(const QString& queryStr, const QVariantMap& params)
{
    return m_dbManager->executeQuery(queryStr, params);
}

QList<DownloadRecord> DownloadRecordDAO::executeSelect(const QString& queryStr,
    const QVariantList& params)
{
    auto results = m_dbManager->executeQueryToMap(queryStr, params);
    QList<DownloadRecord> downloadRecords;
    for (const auto& result : results)
    {
        DownloadRecord downloadRecord;
        fillFromQueryResult(result, downloadRecord);
        downloadRecords.append(downloadRecord);
    }
    return downloadRecords;
}

QList<DownloadRecord> DownloadRecordDAO::executeSelect(const QString& queryStr,
    const QVariantMap& params)
{
    auto results = m_dbManager->executeQueryToMap(queryStr, params);
    QList<DownloadRecord> downloadRecords;
    for (const auto& result : results)
    {
        DownloadRecord downloadRecord;
        fillFromQueryResult(result, downloadRecord);
        downloadRecords.append(downloadRecord);
    }
    return downloadRecords;
}