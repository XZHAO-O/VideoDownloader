#pragma once

#include "DownloadVideoCover.h"

#include <QVariantMap>
#include <functional>

class QSqlQuery;

class QueryWrapper;
class DatabaseManager;

class DownloadVideoCoverDAO
{
public:
    explicit DownloadVideoCoverDAO(QSharedPointer<DatabaseManager> dbManager);
    ~DownloadVideoCoverDAO();

    // 表操作
    bool createTable();
    bool dropTable();

    // CRUD操作
    bool insert(const DownloadVideoCover& downloadVideoCover);
    bool update(const DownloadVideoCover& downloadVideoCover);
    bool deleteById(const QString& taskId);
    QList<DownloadVideoCover> getById(const QString& taskId);
    bool insertBatch(const QList<DownloadVideoCover>& downloadVideoCovers);
    int count();

    QList<DownloadVideoCover> list(const QueryWrapper& wrapper);
    int count(const QueryWrapper& wrapper);
    bool remove(const QueryWrapper& wrapper);
    bool update(const QueryWrapper& wrapper, const QVariantMap& updateFields);

    // 分页查询
    QList<DownloadVideoCover> page(const QueryWrapper& wrapper, int pageNum, int pageSize);

    // 查询操作
    bool executeQuery(const QString& queryStr, const QVariantMap& params);
    bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());
    QList<DownloadVideoCover> executeSelect(const QString& queryStr, const QVariantMap& params);
    QList<DownloadVideoCover> executeSelect(const QString& queryStr, const QVariantList& params = QVariantList());

private:
    QVariantMap toMap(const DownloadVideoCover& downloadVideoCover);
    void fillFromQueryResult(const QVariantMap& result, DownloadVideoCover& downloadVideoCover);

private:
    QSharedPointer<DatabaseManager> m_dbManager;
};