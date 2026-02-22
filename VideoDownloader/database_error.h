// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt headers
#include <QString>

namespace nexusdl::database {

	enum class DatabaseError
	{
		// connection error
		CreateDirectoryError = 0,
		DatabaseOpenError,
		PragmaSetError,

		// query error
		ExecuteQueryError,
		TransactionError
	};

	QString databaseErrorToString(DatabaseError error) noexcept;

} // namespace nexusdl::database