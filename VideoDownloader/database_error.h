// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

namespace nexusdl::database {

	enum class DatabaseError
	{
		CreateDirectoryError,
		DatabaseOpenError,
		PragmaSetError,
	};

	QString databaseErrorToString(DatabaseError error) noexcept;

} // namespace nexusdl::database