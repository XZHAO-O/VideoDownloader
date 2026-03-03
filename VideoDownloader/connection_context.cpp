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

	ConnectionContext::ConnectionContext() noexcept
		: m_connection{}
		, m_isInitialized{ false }
	{
	}

	ConnectionContext::~ConnectionContext()
	{
		if (m_connection.isOpen())
		{
			LOG_DEBUG("Closing database connection");
			m_connection.close();
		}

		if (const QString connectionName = m_connection.connectionName(); QSqlDatabase::contains(connectionName))
		{
			LOG_DEBUG(QString{ "Removing connection: " % connectionName });
			QSqlDatabase::removeDatabase(connectionName);
		}
	}

	std::expected<void, DatabaseError> ConnectionContext::initialize(const QString& databaseDirPath, const QString& fullPath, const QString& connectionName)
	{
		if (m_isInitialized)
		{
			return {};
		}

		// 如果已有同名Qt连接，先移除
		if (QSqlDatabase::contains(connectionName))
		{
			LOG_WARN(QString{ "Removing existing connection: " % connectionName });
			QSqlDatabase::removeDatabase(connectionName);
		}

		// 添加SQLite连接（此时尚未打开文件）
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

			// 2. 检查数据库文件是否已存在（用于区分首次创建与已有数据库）
			const bool fileExists = QFile::exists(fullPath);

			// 3. 打开数据库
			if (!m_connection.open())
			{
				LOG_ERROR(QString{ "Failed to open SQLite database: " % m_connection.lastError().text() });
				QSqlDatabase::removeDatabase(connectionName);
				return std::unexpected{ DatabaseError::DatabaseOpenError };
			}

			QSqlQuery query{ m_connection };

			// 4. 仅当文件首次创建时，设置持久化参数（page_size, journal_mode）
			if (!fileExists)
			{
				LOG_DEBUG("New database file is being created, setting persistent PRAGMAs");

				// 设置 page_size（必须在新数据库上设置，且应在任何表创建前）
				if (!query.exec("PRAGMA page_size = 4096"))
				{
					LOG_ERROR(QString{ "Failed to set page_size: " % query.lastError().text() });
					m_connection.close();
					QSqlDatabase::removeDatabase(connectionName);
					return std::unexpected{ DatabaseError::PragmaSetError };
				}

				// 设置 journal_mode = WAL，并验证
				if (!query.exec("PRAGMA journal_mode = WAL"))
				{
					LOG_ERROR(QString{ "Failed to set WAL journal mode: " % query.lastError().text() });
					m_connection.close();
					QSqlDatabase::removeDatabase(connectionName);
					return std::unexpected{ DatabaseError::PragmaSetError };
				}

				// 验证 journal_mode 是否真的变成 WAL
				if (query.exec("PRAGMA journal_mode") && query.next() && query.value(0).toString().toLower() != "wal")
				{
					LOG_ERROR("Failed to verify WAL journal mode");
					m_connection.close();
					QSqlDatabase::removeDatabase(connectionName);
					return std::unexpected{ DatabaseError::PragmaSetError };
				}
			}
			else
			{
				LOG_DEBUG("Existing database file, skipping persistent PRAGMAs (page_size, journal_mode)");
			}

			//// 5. 始终设置的连接级参数（外键约束必须启用）
			//if (!query.exec("PRAGMA foreign_keys = ON"))
			//{
			//	LOG_ERROR(QString{ "Failed to enable foreign keys: " % query.lastError().text() });
			//	m_connection.close();
			//	QSqlDatabase::removeDatabase(connectionName);
			//	return std::unexpected{ DatabaseError::PragmaSetError };
			//}

			// 以下优化配置失败时仅警告，不阻止初始化
			if (!query.exec("PRAGMA synchronous = NORMAL"))
				LOG_WARN(QString{ "Failed to set synchronous: " % query.lastError().text() });
			if (!query.exec("PRAGMA cache_size = 16384"))
				LOG_WARN(QString{ "Failed to set cache_size: " % query.lastError().text() });
			if (!query.exec("PRAGMA temp_store = MEMORY"))
				LOG_WARN(QString{ "Failed to set temp_store: " % query.lastError().text() });

			LOG_DEBUG("SQLite database connection initialized");
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