// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_error.h"

namespace nexusdl::database {

	QString databaseErrorToString(DatabaseError error) noexcept
	{
		switch (error)
		{
			// connection error
		case DatabaseError::CreateDirectoryError:
			return QStringLiteral("CreateDirectoryError");
		case DatabaseError::DatabaseOpenError:
			return QStringLiteral("DatabaseOpenError");
		case DatabaseError::PragmaSetError:
			return QStringLiteral("PragmaSetError");
			// query error
		case DatabaseError::ExecuteQueryError:
			return QStringLiteral("ExecuteQueryError");
		case DatabaseError::TransactionError:
			return QStringLiteral("TransactionError");
		default:
			return QStringLiteral("UnknownDatabaseError");
		}
	}

} // namespace nexusdl::database