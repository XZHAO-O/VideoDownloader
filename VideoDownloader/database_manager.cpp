// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_manager.h"

namespace nexusdl::database {

	DatabaseManager& DatabaseManager::instance()
	{
		static DatabaseManager instance{};
		return instance;
	}

	DatabaseManager::DatabaseManager()
		: m_databases{ SQLiteDatabase{databaseIdToString(DatabaseId::Download)},
			  SQLiteDatabase{databaseIdToString(DatabaseId::User)} }
	{
	}

	SQLiteDatabase& DatabaseManager::database(DatabaseId id)
	{
		return m_databases[static_cast<int>(id)];
	}

} // namespace nexusdl::database