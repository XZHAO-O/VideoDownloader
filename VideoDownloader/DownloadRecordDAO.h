#pragma once

#include "DownloadRecord.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QList>
#include <QVariantMap>
#include <QSharedPointer>
#include <functional>

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
    bool get(const QString& taskId, DownloadRecord& downloadRecord);
    QList<DownloadRecord> getAll();
    bool insertBatch(const QList<DownloadRecord>& downloadRecords);

    // 查询操作
    bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());
    bool executeSelect(const QString& queryStr,
        const QVariantList& params = QVariantList(),
        std::function<void(QSqlQuery&)> resultProcessor = nullptr);

    // 计数和分页查询
    int count();
    QList<DownloadRecord> getPage(int page, int pageSize);

private:
    // 将DownloadRecord转换为QVariantMap用于绑定参数
    QVariantMap toMap(const DownloadRecord& downloadRecord);
    // 从查询结果填充DownloadRecord
    void fillFromQuery(const QSqlQuery& query, DownloadRecord& downloadRecord);

private:
    QSharedPointer<DatabaseManager> m_dbManager;
};
