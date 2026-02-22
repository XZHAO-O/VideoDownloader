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
		static DatabaseManager& instance();

		DatabaseManager(const DatabaseManager&) = delete;
		DatabaseManager& operator=(const DatabaseManager&) = delete;
		DatabaseManager(DatabaseManager&&) = delete;
		DatabaseManager& operator=(DatabaseManager&&) = delete;

		SQLiteDatabase& database(DatabaseId id);

	private:
		DatabaseManager();
		~DatabaseManager() = default;

	private:
		std::array<SQLiteDatabase, static_cast<size_t>(DatabaseId::Count)> m_databases;
	};

} // namespace nexusdl::database