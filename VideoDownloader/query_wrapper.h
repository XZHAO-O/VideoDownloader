// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

// Qt headers
#include <QString>
#include <QVariant>
#include <QList>
#include <QPair>

// Project internal headers
#include "field.h"

namespace nexusdl::database {

	template<typename Entity>
	class QueryWrapper
	{
	public:
		explicit QueryWrapper();
		~QueryWrapper();

		// 重置包装器
		QueryWrapper<Entity>& reset();

		// SELECT 字段相关
		QueryWrapper<Entity>& select(const QString& columns = "*");
		QueryWrapper<Entity>& select(const QStringList& columns);

		// ---------- 条件方法：字符串版本 ----------
		QueryWrapper<Entity>& eq(const QString& column, const QVariant& value);
		QueryWrapper<Entity>& eq(bool condition, const QString& column, const QVariant& value);

		QueryWrapper<Entity>& ne(const QString& column, const QVariant& value);
		QueryWrapper<Entity>& ne(bool condition, const QString& column, const QVariant& value);

		QueryWrapper<Entity>& gt(const QString& column, const QVariant& value);
		QueryWrapper<Entity>& gt(bool condition, const QString& column, const QVariant& value);

		QueryWrapper<Entity>& ge(const QString& column, const QVariant& value);
		QueryWrapper<Entity>& ge(bool condition, const QString& column, const QVariant& value);

		QueryWrapper<Entity>& lt(const QString& column, const QVariant& value);
		QueryWrapper<Entity>& lt(bool condition, const QString& column, const QVariant& value);

		QueryWrapper<Entity>& le(const QString& column, const QVariant& value);
		QueryWrapper<Entity>& le(bool condition, const QString& column, const QVariant& value);

		QueryWrapper<Entity>& like(const QString& column, const QString& value);
		QueryWrapper<Entity>& like(bool condition, const QString& column, const QString& value);

		QueryWrapper<Entity>& notLike(const QString& column, const QString& value);
		QueryWrapper<Entity>& notLike(bool condition, const QString& column, const QString& value);

		QueryWrapper<Entity>& in(const QString& column, const QList<QVariant>& values);
		QueryWrapper<Entity>& in(bool condition, const QString& column, const QList<QVariant>& values);

		QueryWrapper<Entity>& notIn(const QString& column, const QList<QVariant>& values);
		QueryWrapper<Entity>& notIn(bool condition, const QString& column, const QList<QVariant>& values);

		QueryWrapper<Entity>& between(const QString& column, const QVariant& start, const QVariant& end);
		QueryWrapper<Entity>& between(bool condition, const QString& column, const QVariant& start, const QVariant& end);

		QueryWrapper<Entity>& notBetween(const QString& column, const QVariant& start, const QVariant& end);
		QueryWrapper<Entity>& notBetween(bool condition, const QString& column, const QVariant& start, const QVariant& end);

		QueryWrapper<Entity>& isNull(const QString& column);
		QueryWrapper<Entity>& isNull(bool condition, const QString& column);

		QueryWrapper<Entity>& isNotNull(const QString& column);
		QueryWrapper<Entity>& isNotNull(bool condition, const QString& column);

		// ---------- 条件方法：Field版本（类型安全） ----------
		template<typename T>
		QueryWrapper<Entity>& eq(const Field<Entity, T>& field, const T& value)
		{
			return eq(field.name, QVariant::fromValue(value));
		}

		template<typename T>
		QueryWrapper<Entity>& eq(bool condition, const Field<Entity, T>& field, const T& value)
		{
			if (condition)
				return eq(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& ne(const Field<Entity, T>& field, const T& value)
		{
			return ne(field.name, QVariant::fromValue(value));
		}

		template<typename T>
		QueryWrapper<Entity>& ne(bool condition, const Field<Entity, T>& field, const T& value)
		{
			if (condition)
				return ne(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& gt(const Field<Entity, T>& field, const T& value)
		{
			return gt(field.name, QVariant::fromValue(value));
		}

		template<typename T>
		QueryWrapper<Entity>& gt(bool condition, const Field<Entity, T>& field, const T& value)
		{
			if (condition)
				return gt(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& ge(const Field<Entity, T>& field, const T& value)
		{
			return ge(field.name, QVariant::fromValue(value));
		}

		template<typename T>
		QueryWrapper<Entity>& ge(bool condition, const Field<Entity, T>& field, const T& value)
		{
			if (condition)
				return ge(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& lt(const Field<Entity, T>& field, const T& value)
		{
			return lt(field.name, QVariant::fromValue(value));
		}

		template<typename T>
		QueryWrapper<Entity>& lt(bool condition, const Field<Entity, T>& field, const T& value)
		{
			if (condition)
				return lt(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& le(const Field<Entity, T>& field, const T& value)
		{
			return le(field.name, QVariant::fromValue(value));
		}

		template<typename T>
		QueryWrapper<Entity>& le(bool condition, const Field<Entity, T>& field, const T& value)
		{
			if (condition)
				return le(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& like(const Field<Entity, T>& field, const QString& value)
		{
			return like(field.name, value);
		}

		template<typename T>
		QueryWrapper<Entity>& like(bool condition, const Field<Entity, T>& field, const QString& value)
		{
			if (condition)
				return like(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& notLike(const Field<Entity, T>& field, const QString& value)
		{
			return notLike(field.name, value);
		}

		template<typename T>
		QueryWrapper<Entity>& notLike(bool condition, const Field<Entity, T>& field, const QString& value)
		{
			if (condition)
				return notLike(field, value);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& in(const Field<Entity, T>& field, const QList<QVariant>& values)
		{
			return in(field.name, values);
		}

		template<typename T>
		QueryWrapper<Entity>& in(bool condition, const Field<Entity, T>& field, const QList<QVariant>& values)
		{
			if (condition)
				return in(field, values);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& notIn(const Field<Entity, T>& field, const QList<QVariant>& values)
		{
			return notIn(field.name, values);
		}

		template<typename T>
		QueryWrapper<Entity>& notIn(bool condition, const Field<Entity, T>& field, const QList<QVariant>& values)
		{
			if (condition)
				return notIn(field, values);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& between(const Field<Entity, T>& field, const T& start, const T& end)
		{
			return between(field.name, QVariant::fromValue(start), QVariant::fromValue(end));
		}

		template<typename T>
		QueryWrapper<Entity>& between(bool condition, const Field<Entity, T>& field, const T& start, const T& end)
		{
			if (condition)
				return between(field, start, end);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& notBetween(const Field<Entity, T>& field, const T& start, const T& end)
		{
			return notBetween(field.name, QVariant::fromValue(start), QVariant::fromValue(end));
		}

		template<typename T>
		QueryWrapper<Entity>& notBetween(bool condition, const Field<Entity, T>& field, const T& start, const T& end)
		{
			if (condition)
				return notBetween(field, start, end);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& isNull(const Field<Entity, T>& field)
		{
			return isNull(field.name);
		}

		template<typename T>
		QueryWrapper<Entity>& isNull(bool condition, const Field<Entity, T>& field)
		{
			if (condition)
				return isNull(field);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& isNotNull(const Field<Entity, T>& field)
		{
			return isNotNull(field.name);
		}

		template<typename T>
		QueryWrapper<Entity>& isNotNull(bool condition, const Field<Entity, T>& field)
		{
			if (condition)
				return isNotNull(field);
			return *this;
		}

		// 逻辑连接符
		QueryWrapper<Entity>& andWrapper();
		QueryWrapper<Entity>& orWrapper();

		// 添加自定义条件（原生SQL）
		QueryWrapper<Entity>& condition(const QString& condition, const QVariantList& values = QVariantList{});

		// ORDER BY（支持字符串和Field版本）
		QueryWrapper<Entity>& orderBy(bool condition, const QString& column, bool asc = true);
		QueryWrapper<Entity>& orderBy(const QString& column, bool asc = true);
		QueryWrapper<Entity>& orderByAsc(const QString& column);
		QueryWrapper<Entity>& orderByDesc(const QString& column);

		template<typename T>
		QueryWrapper<Entity>& orderBy(bool condition, const Field<Entity, T>& field, bool asc = true)
		{
			if (condition)
				return orderBy(field.name, asc);
			return *this;
		}

		template<typename T>
		QueryWrapper<Entity>& orderBy(const Field<Entity, T>& field, bool asc = true)
		{
			return orderBy(field.name, asc);
		}

		template<typename T>
		QueryWrapper<Entity>& orderByAsc(const Field<Entity, T>& field)
		{
			return orderBy(field, true);
		}

		template<typename T>
		QueryWrapper<Entity>& orderByDesc(const Field<Entity, T>& field)
		{
			return orderBy(field, false);
		}

		// GROUP BY
		QueryWrapper<Entity>& groupBy(const QString& columns);
		QueryWrapper<Entity>& groupBy(const QStringList& columns);

		template<typename T>
		QueryWrapper<Entity>& groupBy(const Field<Entity, T>& field)
		{
			return groupBy(field.name);
		}

		// HAVING
		QueryWrapper<Entity>& having(const QString& havingCondition);
		QueryWrapper<Entity>& having(bool condition, const QString& havingCondition);

		// LIMIT / OFFSET
		QueryWrapper<Entity>& limit(int limit);
		QueryWrapper<Entity>& limit(int offset, int limit);
		QueryWrapper<Entity>& offset(int offset);

		// DISTINCT
		QueryWrapper<Entity>& distinct();

		// 构建 SQL 语句
		QString buildSelectSql(const QString& tableName) const;
		QString buildDeleteSql(const QString& tableName) const;
		QString buildCountSql(const QString& tableName) const;
		QString buildUpdateSql(const QString& tableName, const QVariantMap& updateFields) const;

		// 获取绑定的参数
		QList<QVariant> getBindValues() const;

		// 获取最后构建的SQL
		QString getLastSql() const { return m_lastSql; }

		// 获取分页参数
		int getLimit() const { return m_limit; }
		int getOffset() const { return m_offset; }

	private:
		struct ConditionGroup
		{
			QString logicalOp; // "AND" or "OR"
			QList<QPair<QString, QVariantList>> conditions;
		};

		void addCondition(const QString& condition, const QVariantList& values = QVariantList{});
		QString buildWhereSql() const;
		QString buildOrderBySql() const;
		QString buildGroupBySql() const;
		QString buildHavingSql() const;
		QString buildLimitSql() const;
		QString buildSelectColumns() const;

		QString m_selectColumns;
		QList<ConditionGroup> m_conditionGroups;
		QList<QPair<QString, QVariantList>> m_havingConditions;
		QStringList m_orderByColumns;
		QStringList m_groupByColumns;
		int m_limit;
		int m_offset;
		bool m_distinct;
		mutable QString m_lastSql;
		mutable QList<QVariant> m_bindValues;
		QString m_currentLogicalOp;
	};

	// ------------------------ 模板实现 ------------------------

	template<typename Entity>
	inline QueryWrapper<Entity>::QueryWrapper()
		: m_selectColumns{ "*" }
		, m_conditionGroups{}
		, m_havingConditions{}
		, m_orderByColumns{}
		, m_groupByColumns{}
		, m_limit{ -1 }
		, m_offset{ -1 }
		, m_distinct{ false }
		, m_lastSql{}
		, m_bindValues{}
		, m_currentLogicalOp{ "AND" }
	{
		// 初始化第一个条件组
		m_conditionGroups.append(ConditionGroup{ "AND", {} });
	}

	template<typename Entity>
	inline QueryWrapper<Entity>::~QueryWrapper()
	{
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::reset()
	{
		m_selectColumns = "*";
		m_conditionGroups.clear();
		m_conditionGroups.append(ConditionGroup{ "AND", {} });
		m_havingConditions.clear();
		m_orderByColumns.clear();
		m_groupByColumns.clear();
		m_limit = -1;
		m_offset = -1;
		m_distinct = false;
		m_currentLogicalOp = "AND";
		m_bindValues.clear();
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::select(const QString& columns)
	{
		m_selectColumns = columns;
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::select(const QStringList& columns)
	{
		m_selectColumns = columns.join(", ");
		return *this;
	}

	// ---------- 字符串条件方法 ----------
	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::eq(const QString& column, const QVariant& value)
	{
		QString condition = column % " = ?";
		QVariantList values = { value };
		addCondition(condition, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::eq(bool condition, const QString& column, const QVariant& value)
	{
		if (condition)
		{
			return eq(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::ne(const QString& column, const QVariant& value)
	{
		QString cond = column % " != ?";
		QVariantList values = { value };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::ne(bool condition, const QString& column, const QVariant& value)
	{
		if (condition)
		{
			return ne(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::gt(const QString& column, const QVariant& value)
	{
		QString cond = column % " > ?";
		QVariantList values = { value };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::gt(bool condition, const QString& column, const QVariant& value)
	{
		if (condition)
		{
			return gt(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::ge(const QString& column, const QVariant& value)
	{
		QString cond = column % " >= ?";
		QVariantList values = { value };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::ge(bool condition, const QString& column, const QVariant& value)
	{
		if (condition)
		{
			return ge(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::lt(const QString& column, const QVariant& value)
	{
		QString cond = column % " < ?";
		QVariantList values = { value };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::lt(bool condition, const QString& column, const QVariant& value)
	{
		if (condition)
		{
			return lt(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::le(const QString& column, const QVariant& value)
	{
		QString cond = column % " <= ?";
		QVariantList values = { value };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::le(bool condition, const QString& column, const QVariant& value)
	{
		if (condition)
		{
			return le(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::like(const QString& column, const QString& value)
	{
		QString cond = column % " LIKE ?";
		QVariantList values = { QString{"%" % value % "%"} };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::like(bool condition, const QString& column, const QString& value)
	{
		if (condition)
		{
			return like(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::notLike(const QString& column, const QString& value)
	{
		QString cond = column % " NOT LIKE ?";
		QVariantList values = { QString{"%" % value % "%"} };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::notLike(bool condition, const QString& column, const QString& value)
	{
		if (condition)
		{
			return notLike(column, value);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::in(const QString& column, const QList<QVariant>& values)
	{
		if (values.isEmpty())
		{
			return *this;
		}

		QString placeholders{};
		QVariantList bindValues{};
		for (int i = 0; i < values.size(); ++i)
		{
			if (i > 0)
			{
				placeholders = placeholders % ", ";
			}
			placeholders = placeholders % "?";
			bindValues.append(values[i]);
		}

		QString cond = column % " IN (" % placeholders % ")";
		addCondition(cond, bindValues);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::in(bool condition, const QString& column, const QList<QVariant>& values)
	{
		if (condition)
		{
			return in(column, values);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::notIn(const QString& column, const QList<QVariant>& values)
	{
		if (values.isEmpty())
		{
			return *this;
		}

		QString placeholders{};
		QVariantList bindValues{};
		for (int i = 0; i < values.size(); ++i)
		{
			if (i > 0)
			{
				placeholders = placeholders % ", ";
			}
			placeholders = placeholders % "?";
			bindValues.append(values[i]);
		}

		QString cond = column % " NOT IN (" % placeholders % ")";
		addCondition(cond, bindValues);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::notIn(bool condition, const QString& column, const QList<QVariant>& values)
	{
		if (condition)
		{
			return notIn(column, values);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::between(const QString& column, const QVariant& start, const QVariant& end)
	{
		QString cond = column % " BETWEEN ? AND ?";
		QVariantList values = { start, end };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::between(bool condition, const QString& column, const QVariant& start, const QVariant& end)
	{
		if (condition)
		{
			return between(column, start, end);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::notBetween(const QString& column, const QVariant& start, const QVariant& end)
	{
		QString cond = column % " NOT BETWEEN ? AND ?";
		QVariantList values = { start, end };
		addCondition(cond, values);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::notBetween(bool condition, const QString& column, const QVariant& start, const QVariant& end)
	{
		if (condition)
		{
			return notBetween(column, start, end);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::isNull(const QString& column)
	{
		QString cond = column % " IS NULL";
		addCondition(cond);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::isNull(bool condition, const QString& column)
	{
		if (condition)
		{
			return isNull(column);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::isNotNull(const QString& column)
	{
		QString cond = column % " IS NOT NULL";
		addCondition(cond);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::isNotNull(bool condition, const QString& column)
	{
		if (condition)
		{
			return isNotNull(column);
		}
		return *this;
	}

	// 逻辑连接符
	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::andWrapper()
	{
		m_currentLogicalOp = "AND";
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::orWrapper()
	{
		m_currentLogicalOp = "OR";
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::condition(const QString& condition, const QVariantList& values)
	{
		addCondition(condition, values);
		return *this;
	}

	// ORDER BY
	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::orderBy(bool condition, const QString& column, bool asc)
	{
		if (condition)
		{
			return orderBy(column, asc);
		}
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::orderBy(const QString& column, bool asc)
	{
		m_orderByColumns.append(column % (asc ? " ASC" : " DESC"));
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::orderByAsc(const QString& column)
	{
		return orderBy(column, true);
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::orderByDesc(const QString& column)
	{
		return orderBy(column, false);
	}

	// GROUP BY
	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::groupBy(const QString& columns)
	{
		m_groupByColumns.append(columns);
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::groupBy(const QStringList& columns)
	{
		m_groupByColumns.append(columns);
		return *this;
	}

	// HAVING
	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::having(const QString& havingCondition)
	{
		m_havingConditions.append({ havingCondition, QVariantList{} });
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::having(bool condition, const QString& havingCondition)
	{
		if (condition)
		{
			return having(havingCondition);
		}
		return *this;
	}

	// LIMIT / OFFSET
	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::limit(int limit)
	{
		m_limit = limit;
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::limit(int offset, int limit)
	{
		m_offset = offset;
		m_limit = limit;
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::offset(int offset)
	{
		m_offset = offset;
		return *this;
	}

	template<typename Entity>
	inline QueryWrapper<Entity>& QueryWrapper<Entity>::distinct()
	{
		m_distinct = true;
		return *this;
	}

	// SQL 构建
	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildSelectSql(const QString& tableName) const
	{
		if (tableName.isEmpty())
		{
			m_lastSql = QString{};
			return QString{};
		}

		QString sql{};

		// SELECT
		sql = "SELECT ";
		if (m_distinct)
		{
			sql = sql % "DISTINCT ";
		}
		sql = sql % buildSelectColumns();

		// FROM
		sql = sql % " FROM " % tableName;

		// WHERE
		QString whereSql = buildWhereSql();
		if (!whereSql.isEmpty())
		{
			sql = sql % " WHERE " % whereSql;
		}

		// GROUP BY
		QString groupBySql = buildGroupBySql();
		if (!groupBySql.isEmpty())
		{
			sql = sql % " GROUP BY " % groupBySql;
		}

		// HAVING
		QString havingSql = buildHavingSql();
		if (!havingSql.isEmpty())
		{
			sql = sql % " HAVING " % havingSql;
		}

		// ORDER BY
		QString orderBySql = buildOrderBySql();
		if (!orderBySql.isEmpty())
		{
			sql = sql % " ORDER BY " % orderBySql;
		}

		// LIMIT & OFFSET
		QString limitSql = buildLimitSql();
		if (!limitSql.isEmpty())
		{
			sql = sql % " " % limitSql;
		}

		m_lastSql = sql;
		return sql;
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildDeleteSql(const QString& tableName) const
	{
		if (tableName.isEmpty())
		{
			m_lastSql = QString{};
			return QString{};
		}

		QString sql = "DELETE FROM " % tableName;

		QString whereSql = buildWhereSql();
		if (!whereSql.isEmpty())
		{
			sql = sql % " WHERE " % whereSql;
		}

		m_lastSql = sql;
		return sql;
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildCountSql(const QString& tableName) const
	{
		if (tableName.isEmpty())
		{
			m_lastSql = QString{};
			return QString{};
		}

		QString sql = "SELECT COUNT(*) FROM " % tableName;

		QString whereSql = buildWhereSql();
		if (!whereSql.isEmpty())
		{
			sql = sql % " WHERE " % whereSql;
		}

		m_lastSql = sql;
		return sql;
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildUpdateSql(const QString& tableName, const QVariantMap& updateFields) const
	{
		if (tableName.isEmpty() || updateFields.isEmpty())
		{
			return QString{};
		}

		QString sql = "UPDATE " % tableName % " SET ";

		QStringList setClauses{};
		QVariantList bindValues{};

		// 构建SET子句
		for (auto it = updateFields.constBegin(); it != updateFields.constEnd(); ++it)
		{
			setClauses.append(it.key() % " = ?");
			bindValues.append(it.value());
		}

		sql = sql % setClauses.join(", ");

		// 重新构造绑定值：SET值在前，WHERE值在后
		m_bindValues.clear();
		m_bindValues.append(bindValues);

		// 构建WHERE子句（不会修改m_bindValues，但会从条件组中收集值）
		QString whereSql = buildWhereSql();
		if (!whereSql.isEmpty())
		{
			sql = sql % " WHERE " % whereSql;
			// 将WHERE条件的值追加到m_bindValues
			for (const auto& group : m_conditionGroups)
			{
				for (const auto& cond : group.conditions)
				{
					m_bindValues.append(cond.second);
				}
			}
		}

		m_lastSql = sql;
		return sql;
	}

	template<typename Entity>
	inline QList<QVariant> QueryWrapper<Entity>::getBindValues() const
	{
		return m_bindValues;
	}

	// 私有辅助方法
	template<typename Entity>
	inline void QueryWrapper<Entity>::addCondition(const QString& condition, const QVariantList& values)
	{
		// 如果当前条件组为空，或者当前逻辑操作符与条件组的不同，创建新的条件组
		if (m_conditionGroups.isEmpty() ||
			m_conditionGroups.last().logicalOp != m_currentLogicalOp)
		{
			m_conditionGroups.append(ConditionGroup{ m_currentLogicalOp, {} });
		}

		m_conditionGroups.last().conditions.append({ condition, values });

		// 添加绑定值
		for (const auto& value : values)
		{
			m_bindValues.append(value);
		}

		// 重置逻辑操作符为默认值（AND）
		m_currentLogicalOp = "AND";
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildWhereSql() const
	{
		if (m_conditionGroups.isEmpty() ||
			(m_conditionGroups.size() == 1 && m_conditionGroups[0].conditions.isEmpty()))
		{
			return QString{};
		}

		QString whereSql{};
		bool firstGroup = true;

		for (const auto& group : m_conditionGroups)
		{
			if (group.conditions.isEmpty())
			{
				continue;
			}

			if (!firstGroup)
			{
				whereSql = whereSql % " " % group.logicalOp % " ";
			}

			// 如果条件组只有一个条件，直接添加
			if (group.conditions.size() == 1)
			{
				whereSql = whereSql % group.conditions[0].first;
			}
			else
			{
				// 多个条件用括号括起来
				whereSql = whereSql % "(";
				for (int i = 0; i < group.conditions.size(); ++i)
				{
					if (i > 0)
					{
						whereSql = whereSql % " AND ";
					}
					whereSql = whereSql % group.conditions[i].first;
				}
				whereSql = whereSql % ")";
			}

			firstGroup = false;
		}

		return whereSql;
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildOrderBySql() const
	{
		if (m_orderByColumns.isEmpty())
		{
			return QString{};
		}
		return m_orderByColumns.join(", ");
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildGroupBySql() const
	{
		if (m_groupByColumns.isEmpty())
		{
			return QString{};
		}
		return m_groupByColumns.join(", ");
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildHavingSql() const
	{
		if (m_havingConditions.isEmpty())
		{
			return QString{};
		}

		QString havingSql{};
		for (int i = 0; i < m_havingConditions.size(); ++i)
		{
			if (i > 0)
			{
				havingSql = havingSql % " AND ";
			}
			havingSql = havingSql % m_havingConditions[i].first;

			// 添加绑定值
			for (const auto& value : m_havingConditions[i].second)
			{
				m_bindValues.append(value);
			}
		}

		return havingSql;
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildLimitSql() const
	{
		QString limitSql{};

		if (m_limit > 0)
		{
			limitSql = "LIMIT " % QString::number(m_limit);
			if (m_offset >= 0)
			{
				limitSql = limitSql % " OFFSET " % QString::number(m_offset);
			}
		}

		return limitSql;
	}

	template<typename Entity>
	inline QString QueryWrapper<Entity>::buildSelectColumns() const
	{
		return m_selectColumns;
	}

} // namespace nexusdl::database