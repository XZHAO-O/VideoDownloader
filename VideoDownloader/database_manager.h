// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <array>

// Project internal headers
#include "database_id.h"
#include "sqlite_database.h"

namespace nexusdl::database {

	class DatabaseManager
	{
	public:
		static DatabaseManager& instance() noexcept;

		DatabaseManager(const DatabaseManager&) = delete;
		DatabaseManager& operator=(const DatabaseManager&) = delete;
		DatabaseManager(DatabaseManager&&) = delete;
		DatabaseManager& operator=(DatabaseManager&&) = delete;

		SQLiteDatabase& database(DatabaseId id) noexcept;
		const SQLiteDatabase& database(DatabaseId id) const noexcept;

		QString lastError(DatabaseId id) const;
		QString databaseDirPath(DatabaseId id) const noexcept;
		QString databaseName(DatabaseId id) const noexcept;
		QString fullDatabasePath(DatabaseId id) const noexcept;
		qint64 databaseSize(DatabaseId id) const;

	private:
		DatabaseManager() noexcept;
		~DatabaseManager() = default;

	private:
		std::array<SQLiteDatabase, static_cast<size_t>(DatabaseId::Count)> m_databases;
	};

} // namespace nexusdl::database