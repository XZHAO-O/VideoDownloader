// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_id.h"

namespace nexusdl::database {

	[[nodiscard]] const QString& databaseIdToString(DatabaseId id)
	{
		static const QString kDownloadDb = QStringLiteral("download.db");
		static const QString kUserDb = QStringLiteral("user.db");
		static const QString kUnknownDb = QStringLiteral("unknown databaseId");

		switch (id)
		{
		case DatabaseId::Download: return kDownloadDb;
		case DatabaseId::User: return kUserDb;
		default: assert(false); return kUnknownDb;
		}
	}

} // namespace nexusdl::database