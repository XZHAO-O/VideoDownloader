#pragma once

#include "DownloadRecord.h"

#include <QVariantMap>
#include <functional>

class QSqlQuery;

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

    // 查询操作
    bool executeQuery(const QString& queryStr, const QVariantMap& params);
    bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());
    QList<DownloadRecord> executeSelect(const QString& queryStr, const QVariantMap& params);
    QList<DownloadRecord> executeSelect(const QString& queryStr, const QVariantList& params = QVariantList());

    // 计数
    int count();

private:
    // 将DownloadRecord转换为QVariantMap用于绑定参数
    QVariantMap toMap(const DownloadRecord& downloadRecord);
    // 从查询结果填充DownloadRecord
    void fillFromQueryResult(const QVariantMap& result, DownloadRecord& downloadRecord);

private:
    QSharedPointer<DatabaseManager> m_dbManager;
};
