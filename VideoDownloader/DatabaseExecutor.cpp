// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "DatabaseExecutor.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QCoreApplication>

#include "Logger.h"

namespace nexusdl::database {

	// 静态thread_local成员初始化
	thread_local QSqlDatabase DatabaseExecutor::m_database;
	thread_local bool DatabaseExecutor::m_isInitialized{ false };

	DatabaseExecutor::DatabaseExecutor(QObject* parent)
		: QObject{ parent }
		, m_databasePath{ QCoreApplication::applicationDirPath() + "/dadtabase" }
	{
	}

	DatabaseExecutor::~DatabaseExecutor()
	{
		if (m_database.isOpen()) {
			m_database.close();
		}
	}

	bool DatabaseExecutor::initialize()
	{
		if (m_isInitialized) {
			return true;
		}

		// 检查并创建数据库目录
		QFileInfo fileInfo(m_databasePath);
		QString dirPath = fileInfo.absolutePath();
		QDir dir(dirPath);

		if (!dir.exists()) {
			if (!dir.mkpath(".")) {
				qCritical() << "Failed to create database directory:" << dirPath;
				return false;
			}
		}

		// 初始化数据库连接
		if (!initConnection()) {
			return false;
		}

		m_isInitialized = true;
		LOG_INFO(QString("Database initialized successfully at:").arg(m_databasePath));
		return true;
	}

	bool DatabaseExecutor::initConnection()
	{
		// 检查是否需要创建数据库文件
		bool dbExists = QFile::exists(m_databasePath);

		// 使用线程ID作为连接名，确保每个线程有独立的连接
		QString connectionName = QString("NexusDLDBConnection_%1")
			.arg(static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())));

		// 如果连接已存在，先移除
		if (QSqlDatabase::contains(connectionName)) {
			QSqlDatabase::removeDatabase(connectionName);
		}

		m_database = QSqlDatabase::addDatabase("QSQLITE", connectionName);
		m_database.setDatabaseName(m_databasePath);

		if (!m_database.open()) {
			qCritical() << "Failed to open database:" << m_database.lastError().text();
			return false;
		}

		// 如果是新创建的数据库，设置SQLite参数
		if (!dbExists) {
			QSqlQuery query(m_database);
			query.exec("PRAGMA foreign_keys = ON");
			query.exec("PRAGMA journal_mode = WAL");
			query.exec("PRAGMA synchronous = NORMAL");
			query.exec("PRAGMA cache_size = -64000");
			query.exec("PRAGMA busy_timeout = 5000");
		}

		return true;
	}

	bool DatabaseExecutor::executeQuery(const QString& queryStr, const QVariantMap& params)
	{
		QWriteLocker locker(&m_rwLock);

		if (!m_database.isOpen()) {
			qCritical() << "Database is not open";
			return false;
		}

		QSqlQuery query(m_database);
		query.prepare(queryStr);

		for (auto it = params.constBegin(); it != params.constEnd(); ++it)
		{
			query.bindValue(it.key(), it.value());
		}

		if (!query.exec()) {
			qCritical() << "Failed to execute query:" << query.lastError().text();
			return false;
		}

		return true;
	}

	bool DatabaseExecutor::executeQuery(const QString& queryStr, const QVariantList& params)
	{
		QWriteLocker locker(&m_rwLock);

		if (!m_database.isOpen()) {
			qCritical() << "Database is not open";
			return false;
		}

		QSqlQuery query(m_database);
		query.prepare(queryStr);

		for (int i = 0; i < params.size(); ++i) {
			query.bindValue(i, params[i]);
		}

		if (!query.exec()) {
			qCritical() << "Failed to execute query:" << query.lastError().text();
			return false;
		}

		return true;
	}

	QList<QVariantMap> DatabaseExecutor::executeQueryToMap(const QString& queryStr, const QVariantMap& params)
	{
		QReadLocker locker(&m_rwLock);

		QList<QVariantMap> result;

		if (!m_database.isOpen())
		{
			qCritical() << "Database is not open";
			return result;
		}

		QSqlQuery query(m_database);
		query.prepare(queryStr);

		for (auto it = params.constBegin(); it != params.constEnd(); ++it)
		{
			query.bindValue(it.key(), it.value());
		}

		if (!query.exec())
		{
			qCritical() << "Failed to execute select:" << query.lastError().text();
			return result;
		}

		while (query.next())
		{
			QVariantMap row;
			QSqlRecord record = query.record();

			for (int i = 0; i < record.count(); ++i)
			{
				row[record.fieldName(i)] = query.value(i);
			}

			result.append(row);
		}

		return result;
	}

	QList<QVariantMap> DatabaseExecutor::executeQueryToMap(const QString& queryStr,
		const QVariantList& params)
	{
		QReadLocker locker(&m_rwLock);

		QList<QVariantMap> result;

		if (!m_database.isOpen())
		{
			qCritical() << "Database is not open";
			return result;
		}

		QSqlQuery query(m_database);
		query.prepare(queryStr);

		for (int i = 0; i < params.size(); ++i)
		{
			query.bindValue(i, params[i]);
		}

		if (!query.exec())
		{
			qCritical() << "Failed to execute select:" << query.lastError().text();
			return result;
		}

		while (query.next())
		{
			QVariantMap row;
			QSqlRecord record = query.record();

			for (int i = 0; i < record.count(); ++i)
			{
				row[record.fieldName(i)] = query.value(i);
			}

			result.append(row);
		}

		return result;
	}

	QString DatabaseExecutor::lastError() const
	{
		QReadLocker locker(&m_rwLock);
		return m_database.lastError().text();
	}

	QString DatabaseExecutor::databasePath() const
	{
		return m_databasePath;
	}

	qint64 DatabaseExecutor::databaseSize() const
	{
		QFileInfo fileInfo(m_databasePath);
		return fileInfo.exists() ? fileInfo.size() : -1;
	}

	bool DatabaseExecutor::beginTransaction()
	{
		m_rwLock.lockForWrite();
		return m_database.transaction();
	}

	bool DatabaseExecutor::commitTransaction()
	{
		bool result = m_database.commit();
		m_rwLock.unlock();
		return result;
	}

	bool DatabaseExecutor::rollbackTransaction()
	{
		bool result = m_database.rollback();
		m_rwLock.unlock();
		return result;
	}

	void DatabaseExecutor::endTransaction()
	{
		m_rwLock.unlock();
	}

}