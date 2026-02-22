// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

namespace nexusdl::database {

	template<typename Entity, typename T>
	struct Field
	{
		using Type = T;
		T Entity::* ptr;      // 成员指针
		const char* name;     // 字段名（字符串）

		constexpr Field(T Entity::* p, const char* n) noexcept
			: ptr{ p }, name{ n }
		{
		}
	};

} // namespace nexusdl::database