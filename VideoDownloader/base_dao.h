// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <expected>
#include <memory>
#include <optional>
#include <vector>

// Qt headers
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

// Project internal headers
#include "sqlite_database.h"
#include "query_wrapper.h"
#include "logger.h"

namespace nexusdl::database {

	template<typename Derived, typename Entity>
	class BaseDAO
	{
	public:
		using Ptr = std::shared_ptr<BaseDAO<Derived, Entity>>;

		explicit BaseDAO(std::shared_ptr<SQLiteDatabase> db)
			: m_db{ std::move(db) }
		{
		}

		virtual ~BaseDAO() = default;

		// 插入实体 (public interface)
		std::expected<void, DatabaseError> insert(const Entity& entity)
		{
			return derived()->insertImpl(entity);
		}

		// 根据主键更新（主键字段不更新）
		std::expected<void, DatabaseError> updateById(const Entity& entity)
		{
			return derived()->updateByIdImpl(entity);
		}

		// 根据主键删除
		std::expected<void, DatabaseError> deleteById(const QVariant& id)
		{
			return derived()->deleteByIdImpl(id);
		}

		// 根据主键查询单个实体
		std::expected<std::optional<Entity>, DatabaseError> selectById(const QVariant& id)
		{
			return derived()->selectByIdImpl(id);
		}

		// 根据条件查询列表
		std::expected<std::vector<Entity>, DatabaseError> selectList(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->selectListImpl(wrapper);
		}

		// 根据条件查询单个（取第一条）
		std::expected<std::optional<Entity>, DatabaseError> selectOne(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->selectOneImpl(wrapper);
		}

		// 根据条件计数
		std::expected<long, DatabaseError> selectCount(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->selectCountImpl(wrapper);
		}

	protected:
		// 可被子类重写的虚函数（原有，保持不变）
		virtual std::expected<void, DatabaseError> update(const QVariantMap& updateFields, const QueryWrapper<Entity>& wrapper)
		{
			if (updateFields.isEmpty())
			{
				LOG_WARN("Update fields is empty, skipping update");
				return {};
			}
			QString sql = wrapper.buildUpdateSql(Entity::tableName(), updateFields);
			auto result = m_db->executeWrite(sql, wrapper.getBindValues());
			return result;
		}

		virtual std::expected<void, DatabaseError> deleteByWrapper(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildDeleteSql(Entity::tableName());
			return m_db->executeWrite(sql, wrapper.getBindValues());
		}

		std::shared_ptr<SQLiteDatabase> m_db;

		// ---------- Impl 函数（可被子类重写，通过 CRTP 静态调用） ----------
		std::expected<void, DatabaseError> insertImpl(const Entity& entity)
		{
			QVariantMap map = entity.toMap();
			if (map.isEmpty())
			{
				LOG_ERROR("Entity toMap returned empty map");
				return std::unexpected{ DatabaseError::InvalidArgument };
			}
			QString sql = buildInsertSql(Entity::tableName(), map.keys());
			return m_db->executeWrite(sql, map);
		}

		std::expected<void, DatabaseError> updateByIdImpl(const Entity& entity)
		{
			QVariantMap map = entity.toMap();
			QString idField = Entity::primaryKey();
			if (!map.contains(idField))
			{
				LOG_ERROR("Entity missing primary key field: " + idField);
				return std::unexpected{ DatabaseError::InvalidArgument };
			}
			QVariant id = map.take(idField); // 移除主键
			if (id.isNull())
			{
				LOG_ERROR("Primary key value is null");
				return std::unexpected{ DatabaseError::InvalidArgument };
			}
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(idField, id);
			return update(map, wrapper); // 调用虚函数，允许子类定制更新逻辑
		}

		std::expected<void, DatabaseError> deleteByIdImpl(const QVariant& id)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKey(), id);
			return deleteByWrapper(wrapper); // 调用虚函数
		}

		std::expected<std::optional<Entity>, DatabaseError> selectByIdImpl(const QVariant& id)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKey(), id);
			return selectOne(wrapper);
		}

		std::expected<std::vector<Entity>, DatabaseError> selectListImpl(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildSelectSql(Entity::tableName());
			auto result = m_db->executeQuery(sql, wrapper.getBindValues());
			if (!result)
			{
				return std::unexpected{ result.error() };
			}
			return parseQueryResult(*result);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectOneImpl(const QueryWrapper<Entity>& wrapper)
		{
			auto wrapperCopy = wrapper;
			wrapperCopy.limit(1);
			auto result = selectList(wrapperCopy); // 注意：selectList 是 public，会调用 derived()->selectListImpl，不会递归
			if (!result)
			{
				return std::unexpected{ result.error() };
			}
			if (result->empty())
			{
				return std::optional<Entity>{};
			}
			return std::make_optional((*result)[0]);
		}

		std::expected<long, DatabaseError> selectCountImpl(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildCountSql(Entity::tableName());
			auto result = m_db->executeQuery(sql, wrapper.getBindValues());
			if (!result)
			{
				return std::unexpected{ result.error() };
			}
			QSqlQuery query = *result;
			if (query.next())
			{
				return query.value(0).toLongLong();
			}
			return -1;
		}

	private:
		// 辅助函数：获取派生类指针（CRTP）
		Derived* derived() { return static_cast<Derived*>(this); }
		const Derived* derived() const { return static_cast<const Derived*>(this); }

		QString buildInsertSql(const QString& tableName, const QStringList& fields) const
		{
			QString sql{ "INSERT INTO " + tableName + " (" };
			sql += fields.join(", ");
			sql += ") VALUES (";
			QStringList placeholders{};
			placeholders.fill("?", fields.size());
			sql += placeholders.join(", ");
			sql += ")";
			return sql;
		}

		std::vector<Entity> parseQueryResult(const QSqlQuery& query) const
		{
			std::vector<Entity> entities{};
			while (query.next())
			{
				entities.push_back(Entity::fromRecord(query.record()));
			}
			return entities;
		}
	};

} // namespace nexusdl::database