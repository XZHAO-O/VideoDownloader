// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt headers
#include <QString>

namespace nexusdl::database {

	enum class DatabaseId
	{
		Download = 0,
		User,
		Count
	};

	[[nodiscard]] const QString& databaseIdToString(DatabaseId id);

} // namespace nexusdl::database