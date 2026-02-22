// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_error.h"

namespace {
	// connection error
	const QString kCreateDirectoryError = QStringLiteral("CreateDirectoryError");
	const QString kDatabaseOpenError = QStringLiteral("DatabaseOpenError");
	const QString kPragmaSetError = QStringLiteral("PragmaSetError");
	// query error
	const QString kExecuteQueryError = QStringLiteral("ExecuteQueryError");
	const QString kTransactionError = QStringLiteral("TransactionError");
	const QString kInvalidArgument = QStringLiteral("InvalidArgument");

	const QString kUnknownDatabaseError = QStringLiteral("UnknownDatabaseError");
}

namespace nexusdl::database {

	[[nodiscard]] const QString& databaseErrorToString(DatabaseError error)
	{
		switch (error)
		{
			// connection error
		case DatabaseError::CreateDirectoryError: return kCreateDirectoryError;
		case DatabaseError::DatabaseOpenError: return kDatabaseOpenError;
		case DatabaseError::PragmaSetError: return kPragmaSetError;
			// query error
		case DatabaseError::ExecuteQueryError: return kExecuteQueryError;
		case DatabaseError::TransactionError: return kTransactionError;
		case DatabaseError::InvalidArgument: return kInvalidArgument;

		default: assert(false); return kUnknownDatabaseError;
		}
	}

} // namespace nexusdl::database