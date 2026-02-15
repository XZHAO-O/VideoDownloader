// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "connection_context.h"

// Qt headers
#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

// Project internal headers
#include "logger.h"

namespace nexusdl::database {

	ConnectionContext::ConnectionContext()
		: m_connection{}
		, m_isInitialized{ false }
	{
	}

	ConnectionContext::~ConnectionContext()
	{
		if (m_connection.isOpen())
		{
			m_connection.close();
		}

		if (const QString connectionName = m_connection.connectionName(); QSqlDatabase::contains(connectionName))
		{
			QSqlDatabase::removeDatabase(connectionName);
		}
	}

	std::expected<void, DatabaseError> ConnectionContext::initialize(const QString& databaseDirPath, const QString& fullPath, const QString& connectionName)
	{
		if (m_isInitialized)
		{
			return {};
		}

		// 3. 如果已有同名Qt连接，先移除
		if (QSqlDatabase::contains(connectionName))
		{
			LOG_WARN(QString{ "Removing existing connection: " % connectionName });
			QSqlDatabase::removeDatabase(connectionName);
		}

		// 4. 添加SQLite连接并打开
		m_connection = QSqlDatabase::addDatabase("QSQLITE", connectionName);
		m_connection.setDatabaseName(fullPath);

		static QMutex fileMutex{};
		{
			QMutexLocker locker{ &fileMutex };

			// 1. 创建数据库目录（若不存在）
			if (QDir dir{ databaseDirPath }; !dir.exists() && !dir.mkpath("."))
			{
				LOG_ERROR(QString{ "Failed to create database directory: " % databaseDirPath });
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::CreateDirectoryError };
			}

			if (!m_connection.open())
			{
				LOG_ERROR(QString{ "Failed to open SQLite database: " % m_connection.lastError().text() });
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::DatabaseOpenError };
			}

			QSqlQuery query{ m_connection };

			// 每次连接都应确保 WAL 模式
			if (!query.exec("PRAGMA journal_mode = WAL") || (query.next() && query.value(0).toString().toLower() != "wal"))
			{
				LOG_ERROR(QString{ "Failed to set WAL journal mode: " % query.lastError().text() });
				m_connection.close();
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::PragmaSetError };
			}

			// 每次连接都必须启用外键约束
			if (!query.exec("PRAGMA foreign_keys = ON"))
			{
				LOG_ERROR(QString{ "Failed to enable foreign keys: " % query.lastError().text() });
				m_connection.close();
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::PragmaSetError };
			}

			if (!query.exec("PRAGMA foreign_keys"))
			{
				LOG_ERROR("Failed to query PRAGMA foreign_keys");
				m_connection.close();
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::PragmaSetError };
			}

			if (query.next() && query.value(0).toInt() != 1)
			{
				LOG_ERROR("Foreign keys are not enabled");
				m_connection.close();
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::PragmaSetError };
			}

			// 以下为“可容忍失败”的优化配置（仅警告，不阻止初始化）
			if (!query.exec("PRAGMA synchronous = NORMAL"))
				LOG_WARN(QString{ "Failed to set synchronous=NORMAL: " % query.lastError().text() });
			if (!query.exec("PRAGMA cache_size = -64000"))
				LOG_WARN(QString{ "Failed to set cache_size: " % query.lastError().text() });
			if (!query.exec("PRAGMA busy_timeout = 5000"))
				LOG_WARN(QString{ "Failed to set busy_timeout: " % query.lastError().text() });

			LOG_DEBUG("Existing SQLite database opened");
		}
		m_isInitialized = true;
		return {};
	}

	QSqlDatabase& ConnectionContext::connection() noexcept
	{
		return m_connection;
	}

	const QSqlDatabase& ConnectionContext::connection() const noexcept
	{
		return m_connection;
	}

	QString ConnectionContext::lastError() const
	{
		return m_connection.lastError().text();
	}

	bool ConnectionContext::isInitialized() const noexcept
	{
		return m_isInitialized;
	}

} // namespace nexusdl::database