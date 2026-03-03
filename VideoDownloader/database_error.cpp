// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#include "database_error.h"

namespace nexusdl::database {

	[[nodiscard]] const QString& databaseErrorToString(DatabaseError error) noexcept
	{
		static const QString kCreateDirectoryError = QStringLiteral("CreateDirectoryError");
		static const QString kDatabaseOpenError = QStringLiteral("DatabaseOpenError");
		static const QString kPragmaSetError = QStringLiteral("PragmaSetError");
		static const QString kExecuteQueryError = QStringLiteral("ExecuteQueryError");
		static const QString kTransactionError = QStringLiteral("TransactionError");
		static const QString kInvalidArgument = QStringLiteral("InvalidArgument");
		static const QString kUnknownDatabaseError = QStringLiteral("UnknownDatabaseError");

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