// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <vector>
#include <type_traits>

// Qt headers
#include <QString>
#include <QVariantMap>
#include <QSqlRecord>

namespace nexusdl::database {

	template<typename T>
	concept ConstructibleFromQSqlRecord = std::constructible_from<T, const QSqlRecord&>;

	template<ConstructibleFromQSqlRecord Derived>
	class BaseEntity
	{
	public:
		virtual ~BaseEntity() = default;

		auto toMap() const
		{
			return static_cast<const Derived*>(this)->toMapImpl();
		}

		auto toList() const
		{
			return static_cast<const Derived*>(this)->toListImpl();
		}

		auto primaryKey() const
		{
			return static_cast<const Derived*>(this)->primaryKeyImpl();
		}

		// to do:添加fields来获取std::array<std::pair<QString, QVariant>, N>, 应该有字段更新策略
		// 静态 toVariantList()

		// 静态接口：通过静态断言在编译期检查
	protected:
		BaseEntity()
		{
			// 检查静态方法 tableName()
			static_assert(std::is_same_v<decltype(Derived::tableName()), const QString&>,
				"Derived class must provide a static QString tableName() method.");

			// 检查静态方法 primaryKeyName()
			static_assert(std::is_same_v<decltype(Derived::primaryKeyName()), const QString&>,
				"Derived class must provide a static QString primaryKeyName() method.");

			// 检查静态工厂方法 fromRecord(QSqlRecord)
			static_assert(std::is_same_v<decltype(Derived::fromRecord(std::declval<QSqlRecord>())), Derived>,
				"Derived class must provide a static Derived fromRecord(const QSqlRecord&) method.");
		}
	};

} // namespace nexusdl::database