// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_id.h"

namespace {
	const QString kDownloadDb = QStringLiteral("download.db");
	const QString kUserDb = QStringLiteral("user.db");
	const QString kUnknownDb = QStringLiteral("unknown databaseId");
}

namespace nexusdl::database {

	[[nodiscard]] const QString& databaseIdToString(DatabaseId id)
	{
		switch (id)
		{
		case DatabaseId::Download: return kDownloadDb;
		case DatabaseId::User: return kUserDb;
		default: assert(false); return kUnknownDb;
		}
	}

} // namespace nexusdl::database