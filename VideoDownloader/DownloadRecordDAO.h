#pragma once

#include "DownloadRecord.h"

#include <QVariantMap>
#include <functional>

class QSqlQuery;

class QueryWrapper;
class DatabaseManager;

class DownloadRecordDAO
{
public:
    explicit DownloadRecordDAO(QSharedPointer<DatabaseManager> dbManager);
    ~DownloadRecordDAO();

    // 表操作
    bool createTable();
    bool dropTable();

    // CRUD操作
    bool insert(const DownloadRecord& downloadRecord);
    bool update(const DownloadRecord& downloadRecord);
    bool remove(const QString& taskId);
    QList<DownloadRecord> getById(const QString& taskId);
    bool insertBatch(const QList<DownloadRecord>& downloadRecords);
    int count();

    QList<DownloadRecord> selectList(const QueryWrapper& wrapper);
    int selectCount(const QueryWrapper& wrapper);
    bool deleteByWrapper(const QueryWrapper& wrapper);
    bool updateByWrapper(const QueryWrapper& wrapper, const QVariantMap& updateFields);

    // 分页查询
    QList<DownloadRecord> selectPage(const QueryWrapper& wrapper, int pageNum, int pageSize);

    // 查询操作
    bool executeQuery(const QString& queryStr, const QVariantMap& params);
    bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());
    QList<DownloadRecord> executeSelect(const QString& queryStr, const QVariantMap& params);
    QList<DownloadRecord> executeSelect(const QString& queryStr, const QVariantList& params = QVariantList());

private:
    QVariantMap toMap(const DownloadRecord& downloadRecord);
    void fillFromQueryResult(const QVariantMap& result, DownloadRecord& downloadRecord);

private:
    QSharedPointer<DatabaseManager> m_dbManager;
};