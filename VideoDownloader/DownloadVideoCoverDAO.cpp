#include "DownloadVideoCoverDAO.h"

#include "QueryWrapper.h"
#include "DatabaseManager.h"

// 编译时常量定义
namespace
{
    const QString TABLE_NAME = "download_video_cover";

    // SQL语句模板
    const QString CREATE_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS download_video_cover ("
        "    taskId TEXT NOT NULL PRIMARY KEY,"
        "    cover BLOB,"
        "    createdTime DATETIME NOT NULL DEFAULT current_timestamp,"
        "    updatedTime DATETIME NOT NULL DEFAULT current_timestamp"
        ");";

    const QString DROP_TABLE_SQL = "DROP TABLE IF EXISTS download_video_cover";

    const QString INSERT_SQL =
        "INSERT OR REPLACE INTO download_video_cover "
        "(taskId, cover, createdTime, updatedTime) "
        "VALUES "
        "(:taskId, :cover, :createdTime, :updatedTime)";

    const QString DELETE_SQL = "DELETE FROM download_video_cover WHERE taskId = ?";

    const QString SELECT_BY_ID_SQL = "SELECT * FROM download_video_cover WHERE taskId = ?";

    const QString COUNT_SQL = "SELECT COUNT(*) FROM download_video_cover";
}

DownloadVideoCoverDAO::DownloadVideoCoverDAO(QSharedPointer<DatabaseManager> dbManager)
    : m_dbManager(dbManager)
{
    createTable();
}

DownloadVideoCoverDAO::~DownloadVideoCoverDAO()
{
}

bool DownloadVideoCoverDAO::createTable()
{
    return m_dbManager->executeQuery(CREATE_TABLE_SQL);
}

bool DownloadVideoCoverDAO::dropTable()
{
    return m_dbManager->executeQuery(DROP_TABLE_SQL);
}

QVariantMap DownloadVideoCoverDAO::toMap(const DownloadVideoCover& downloadVideoCover)
{
    QVariantMap map;
    map[":taskId"] = downloadVideoCover.taskId;
    map[":cover"] = downloadVideoCover.cover;
    map[":createdTime"] = downloadVideoCover.createdTime;
    map[":updatedTime"] = downloadVideoCover.updatedTime;

    return map;
}

void DownloadVideoCoverDAO::fillFromQueryResult(const QVariantMap& result, DownloadVideoCover& downloadVideoCover)
{
    downloadVideoCover.taskId = result.value("taskId").toString();
    downloadVideoCover.cover = result.value("cover").toByteArray();
    downloadVideoCover.createdTime = result.value("createdTime").toDateTime();
    downloadVideoCover.updatedTime = result.value("updatedTime").toDateTime();
}

bool DownloadVideoCoverDAO::insert(const DownloadVideoCover& downloadVideoCover)
{
    return m_dbManager->executeQuery(INSERT_SQL, toMap(downloadVideoCover));
}

bool DownloadVideoCoverDAO::update(const DownloadVideoCover& downloadVideoCover)
{
    return insert(downloadVideoCover);
}

bool DownloadVideoCoverDAO::remove(const QString& taskId)
{
    QVariantList params;
    params << taskId;

    return m_dbManager->executeQuery(DELETE_SQL, params);
}

QList<DownloadVideoCover> DownloadVideoCoverDAO::getById(const QString& taskId)
{
    QVariantList params;
    params << taskId;

    return executeSelect(SELECT_BY_ID_SQL, params);
}

bool DownloadVideoCoverDAO::insertBatch(const QList<DownloadVideoCover>& downloadVideoCovers)
{
    if (!m_dbManager->beginTransaction())
    {
        return false;
    }

    for (const auto& downloadVideoCover : downloadVideoCovers)
    {
        if (!insert(downloadVideoCover))
        {
            m_dbManager->rollbackTransaction();
            return false;
        }
    }

    return m_dbManager->commitTransaction();
}

int DownloadVideoCoverDAO::count()
{
    int result = 0;
    auto results = m_dbManager->executeQueryToMap(COUNT_SQL);
    if (!results.isEmpty())
    {
        result = results.first().value(0).toInt();
    }
    return result;
}

QList<DownloadVideoCover> DownloadVideoCoverDAO::selectList(const QueryWrapper& wrapper)
{
    QString sql = wrapper.buildSelectSql(TABLE_NAME);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return QList<DownloadVideoCover>();
    }

    return executeSelect(sql, params);
}

int DownloadVideoCoverDAO::selectCount(const QueryWrapper& wrapper)
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

bool DownloadVideoCoverDAO::deleteByWrapper(const QueryWrapper& wrapper)
{
    QString sql = wrapper.buildDeleteSql(TABLE_NAME);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return false;
    }

    return m_dbManager->executeQuery(sql, params);
}

bool DownloadVideoCoverDAO::updateByWrapper(const QueryWrapper& wrapper, const QVariantMap& updateFields)
{
    QString sql = wrapper.buildUpdateSql(TABLE_NAME, updateFields);
    QVariantList params = wrapper.getBindValues();

    if (sql.isEmpty())
    {
        return false;
    }

    return m_dbManager->executeQuery(sql, params);
}

QList<DownloadVideoCover> DownloadVideoCoverDAO::selectPage(const QueryWrapper& wrapper, int pageNum, int pageSize)
{
    QueryWrapper pageWrapper = wrapper;
    pageWrapper.limit((pageNum - 1) * pageSize, pageSize);

    return selectList(pageWrapper);
}

bool DownloadVideoCoverDAO::executeQuery(const QString& queryStr, const QVariantList& params)
{
    return m_dbManager->executeQuery(queryStr, params);
}

bool DownloadVideoCoverDAO::executeQuery(const QString& queryStr, const QVariantMap& params)
{
    return m_dbManager->executeQuery(queryStr, params);
}

QList<DownloadVideoCover> DownloadVideoCoverDAO::executeSelect(const QString& queryStr,
    const QVariantList& params)
{
    auto results = m_dbManager->executeQueryToMap(queryStr, params);
    QList<DownloadVideoCover> downloadVideoCovers;
    for (const auto& result : results)
    {
        DownloadVideoCover downloadVideoCover;
        fillFromQueryResult(result, downloadVideoCover);
        downloadVideoCovers.append(downloadVideoCover);
    }
    return downloadVideoCovers;
}

QList<DownloadVideoCover> DownloadVideoCoverDAO::executeSelect(const QString& queryStr,
    const QVariantMap& params)
{
    auto results = m_dbManager->executeQueryToMap(queryStr, params);
    QList<DownloadVideoCover> downloadVideoCovers;
    for (const auto& result : results)
    {
        DownloadVideoCover downloadVideoCover;
        fillFromQueryResult(result, downloadVideoCover);
        downloadVideoCovers.append(downloadVideoCover);
    }
    return downloadVideoCovers;
}