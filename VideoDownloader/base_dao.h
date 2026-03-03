// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// C++ standard library
#include <expected>
#include <optional>

// Qt headers
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

// Project internal headers
#include "sqlite_database.h"
#include "query_wrapper.h"

namespace nexusdl::database {

	template<typename Derived, typename Entity>
	class BaseDAO
	{
	public:
		virtual ~BaseDAO() = default;

		std::expected<void, DatabaseError> insert(const Entity& entity)
		{
			return derived()->insertImpl(entity);
		}

		std::expected<void, DatabaseError> update(const Entity& entity, const QueryWrapper<Entity>& wrapper)
		{
			return derived()->updateImpl(entity.fields(), wrapper);
		}

		std::expected<void, DatabaseError> updateById(const Entity& entity)
		{
			return derived()->updateByIdImpl(entity);
		}

		std::expected<void, DatabaseError> deleteById(const QVariant& id)
		{
			return derived()->deleteByIdImpl(id);
		}

		std::expected<void, DatabaseError> deleteByWrapper(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->deleteByWrapperImpl(wrapper);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectById(const QVariant& id)
		{
			return derived()->selectByIdImpl(id);
		}

		std::expected<QList<Entity>, DatabaseError> selectList(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->selectListImpl(wrapper);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectOne(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->selectOneImpl(wrapper);
		}

		std::expected<long, DatabaseError> selectCount(const QueryWrapper<Entity>& wrapper)
		{
			return derived()->selectCountImpl(wrapper);
		}

		// 批量插入（普通）
		std::expected<void, DatabaseError> insertBatch(const QList<Entity>& entities)
		{
			return derived()->insertBatchImpl(entities);
		}

		// 批量插入（事务）
		std::expected<void, DatabaseError> insertBatchWithTransaction(const QList<Entity>& entities)
		{
			return derived()->insertBatchWithTransactionImpl(entities);
		}

		// 批量根据主键删除
		std::expected<void, DatabaseError> deleteBatchByIds(const QList<QVariant>& ids)
		{
			return derived()->deleteBatchByIdsImpl(ids);
		}

		// ---------- 事务控制 ----------
		std::expected<void, DatabaseError> beginTransaction()
		{
			return m_db.beginTransaction();
		}

		std::expected<void, DatabaseError> commitTransaction()
		{
			return m_db.commitTransaction();
		}

		std::expected<void, DatabaseError> rollbackTransaction()
		{
			return m_db.rollbackTransaction();
		}

	protected:
		explicit BaseDAO(SQLiteDatabase& db) : m_db{ db } {}

		SQLiteDatabase& m_db;

		template<typename BatchContainer>
		std::expected<void, DatabaseError> executeWriteBatchWithTransaction(const QString& queryStr, const BatchContainer& batchParams)
		{
			if (auto result = beginTransaction(); !result.has_value())
			{
				return result;
			}

			if (auto result = m_db.executeWriteBatch(queryStr, batchParams); !result.has_value())
			{
				if (auto transactionResult = rollbackTransaction(); !transactionResult.has_value())
				{
					return transactionResult;
				}
				return result;
			}

			return commitTransaction();
		}

		std::expected<void, DatabaseError> insertImpl(const Entity& entity)
		{
			return m_db.executeWrite(getInsertSql(), entity.toList());
		}

		template<size_t N>
		std::expected<void, DatabaseError> updateImpl(std::array<std::pair<QString, QVariant>, N>& updateFields, const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildUpdateSql(updateFields);
			return m_db.executeWrite(sql, wrapper.getBindValues());
		}

		std::expected<void, DatabaseError> updateByIdImpl(const Entity& entity)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKeyName(), entity.primaryKey());
			return updateImpl(entity.fields(), wrapper);
		}

		std::expected<void, DatabaseError> deleteByIdImpl(const QVariant& id)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKeyName(), id);
			return deleteByWrapperImpl(wrapper);
		}

		std::expected<void, DatabaseError> deleteByWrapperImpl(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildDeleteSql();
			return m_db.executeWrite(sql, wrapper.getBindValues());
		}

		std::expected<std::optional<Entity>, DatabaseError> selectByIdImpl(const QVariant& id)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.eq(Entity::primaryKeyName(), id);
			return selectOneImpl(wrapper);
		}

		std::expected<QList<Entity>, DatabaseError> selectListImpl(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildSelectSql();
			auto result = m_db.executeQuery(sql, wrapper.getBindValues());
			if (!result.has_value())
			{
				return std::unexpected{ std::move(result).error() };
			}
			return parseQueryResult(*result);
		}

		std::expected<std::optional<Entity>, DatabaseError> selectOneImpl(const QueryWrapper<Entity>& wrapper)
		{
			wrapper.limit(1);
			QString sql = wrapper.buildSelectSql();
			auto result = m_db.executeQuery(sql, wrapper.getBindValues());
			if (!result.has_value())
			{
				return std::unexpected{ std::move(result).error() };
			}
			if (!(*result).next())
			{
				return {};
			}
			return std::make_optional(Entity::fromRecord((*result).record()));
		}

		std::expected<qint64, DatabaseError> selectCountImpl(const QueryWrapper<Entity>& wrapper)
		{
			QString sql = wrapper.buildCountSql();
			auto result = m_db.executeQuery(sql, wrapper.getBindValues());
			if (!result.has_value())
			{
				return std::unexpected{ std::move(result).error() };
			}
			if (QSqlQuery& query = *result; query.next())
			{
				return query.value(0).toLongLong();
			}
			return -1;
		}

		// 批量插入（普通）
		std::expected<void, DatabaseError> insertBatchImpl(const QList<Entity>& entities)
		{
			return m_db.executeWriteBatch(getInsertSql(), Entity::toVariantList(entities));
		}

		// 批量插入（事务）
		std::expected<void, DatabaseError> insertBatchWithTransactionImpl(const QList<Entity>& entities)
		{
			return executeWriteBatchWithTransaction(getInsertSql(), Entity::toVariantList(entities));
		}

		// 批量根据主键删除
		std::expected<void, DatabaseError> deleteBatchByIdsImpl(const QList<QVariant>& ids)
		{
			QueryWrapper<Entity> wrapper{};
			wrapper.in(Entity::primaryKeyName(), ids);
			return deleteByWrapperImpl(wrapper);
		}

	private:
		Derived* derived() { return static_cast<Derived*>(this); }
		const Derived* derived() const { return static_cast<const Derived*>(this); }

		QString buildInsertSql(const QStringList& fields) const
		{
			QString sql{ "INSERT INTO " % Entity::tableName() % " (" };
			sql = sql % fields.join(", ");
			sql = sql % ") VALUES (";
			QStringList placeholders{};
			placeholders.fill("?", fields.size());
			sql = sql % placeholders.join(", ");
			sql = sql % ")";
			return sql;
		}

		QList<Entity> parseQueryResult(QSqlQuery& query) const
		{
			QList<Entity> entities{};
			// optimization for small result sets
			entities.reserve(100);
			while (query.next())
			{
				entities.emplace_back(query.record());
			}
			return entities;
		}
	};

} // namespace nexusdl::database