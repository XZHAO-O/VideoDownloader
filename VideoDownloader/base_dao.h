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

	template<typename Entity>
	class BaseDAO
	{
	public:
		using Ptr = std::shared_ptr<BaseDAO<Entity>>;

		explicit BaseDAO(std::shared_ptr<SQLiteDatabase> db)
			: m_db{ std::move(db) }
		{
		}

		virtual ~BaseDAO() = default;

		// 插入实体
		std::expected<void, DatabaseError> insert(const Entity& entity)
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

		// 根据主键更新（主键字段不更新）
		std::expected<void, DatabaseError> updateById(const Entity& entity)
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
			return update(map, wrapper);
		}

		// 根据主键删除
		std::expected<void, DatabaseError> deleteById(const QVariant& id)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKey(), id);
			return deleteByWrapper(wrapper);
		}

		// 根据主键查询单个实体
		std::expected<std::optional<Entity>, DatabaseError> selectById(const QVariant& id)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKey(), id);
			return selectOne(wrapper);
		}

		// 根据条件查询列表
		std::expected<std::vector<Entity>, DatabaseError> selectList(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildSelectSql(Entity::tableName());
			auto result = m_db->executeQuery(sql, wrapper.getBindValues());
			if (!result)
			{
				return std::unexpected{ result.error() };
			}
			return parseQueryResult(*result);
		}

		// 根据条件查询单个（取第一条）
		std::expected<std::optional<Entity>, DatabaseError> selectOne(const QueryWrapper<Entity>& wrapper)
		{
			auto wrapperCopy = wrapper;
			wrapperCopy.limit(1);
			auto result = selectList(wrapperCopy);
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

		// 根据条件计数
		std::expected<long, DatabaseError> selectCount(const QueryWrapper<Entity>& wrapper)
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
			return -1; // 理论上不应该发生，但返回 -1 表示错误
		}

	protected:
		// 可被子类重写以定制更新行为
		virtual std::expected<void, DatabaseError> update(const QVariantMap& updateFields, const QueryWrapper<Entity>& wrapper)
		{
			if (updateFields.isEmpty())
			{
				LOG_WARN("Update fields is empty, skipping update");
				return {}; // 无操作视为成功
			}
			QString sql = wrapper.buildUpdateSql(Entity::tableName(), updateFields);
			// buildUpdateSql 内部已经修改了 wrapper 的绑定值，但我们不能直接使用 wrapper.getBindValues()，
			// 因为顺序是 SET 值在前，WHERE 值在后。我们需要从 wrapper 的当前状态获取所有绑定值。
			// 注意：buildUpdateSql 已经将 SET 和 WHERE 的值合并到 wrapper 的 m_bindValues 中（按 SET 在前 WHERE 在后顺序），
			// 所以直接使用 wrapper.getBindValues() 即可。
			auto result = m_db->executeWrite(sql, wrapper.getBindValues());
			return result;
		}

		virtual std::expected<void, DatabaseError> deleteByWrapper(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildDeleteSql(Entity::tableName());
			return m_db->executeWrite(sql, wrapper.getBindValues());
		}

		std::shared_ptr<SQLiteDatabase> m_db;

	private:
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