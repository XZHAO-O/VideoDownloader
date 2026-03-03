// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_manager.h"

namespace nexusdl::database {

	DatabaseManager& DatabaseManager::instance() noexcept
	{
		static DatabaseManager instance{};
		return instance;
	}

	DatabaseManager::DatabaseManager() noexcept
		: m_databases{ SQLiteDatabase{databaseIdToString(DatabaseId::Download)},
			  SQLiteDatabase{databaseIdToString(DatabaseId::User)} }
	{
	}

	SQLiteDatabase& DatabaseManager::database(DatabaseId id) noexcept
	{
		return m_databases[static_cast<int>(id)];
	}

	const SQLiteDatabase& DatabaseManager::database(DatabaseId id) const noexcept
	{
		return m_databases[static_cast<int>(id)];
	}

	QString DatabaseManager::lastError(DatabaseId id) const
	{
		return database(id).lastError();
	}

	QString DatabaseManager::databaseDirPath(DatabaseId id) const noexcept
	{
		return database(id).databaseDirPath();
	}

	QString DatabaseManager::databaseName(DatabaseId id) const noexcept
	{
		return database(id).databaseName();
	}

	QString DatabaseManager::fullDatabasePath(DatabaseId id) const noexcept
	{
		return database(id).fullDatabasePath();
	}

	qint64 DatabaseManager::databaseSize(DatabaseId id) const
	{
		return database(id).databaseSize();
	}

} // namespace nexusdl::database