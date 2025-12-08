#include "QueryWrapper.h"

QueryWrapper::QueryWrapper()
	: m_selectColumns("*")
	, m_limit(-1)
	, m_offset(-1)
	, m_distinct(false)
	, m_currentLogicalOp("AND")
{
	// 初始化第一个条件组
	m_conditionGroups.append(ConditionGroup{ "AND", {} });
}

QueryWrapper::~QueryWrapper()
{
}

QueryWrapper& QueryWrapper::reset()
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

QueryWrapper& QueryWrapper::select(const QString& columns)
{
	m_selectColumns = columns;
	return *this;
}

QueryWrapper& QueryWrapper::select(const QStringList& columns)
{
	m_selectColumns = columns.join(", ");
	return *this;
}

QueryWrapper& QueryWrapper::eq(const QString& column, const QVariant& value)
{
	QString condition = QString("%1 = ?").arg(column);
	QVariantList values = { value };
	addCondition(condition, values);
	return *this;
}

QueryWrapper& QueryWrapper::eq(bool condition, const QString& column, const QVariant& value)
{
	if (condition) {
		return eq(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::ne(const QString& column, const QVariant& value)
{
	QString cond = QString("%1 != ?").arg(column);
	QVariantList values = { value };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::ne(bool condition, const QString& column, const QVariant& value)
{
	if (condition) {
		return ne(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::gt(const QString& column, const QVariant& value)
{
	QString cond = QString("%1 > ?").arg(column);
	QVariantList values = { value };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::gt(bool condition, const QString& column, const QVariant& value)
{
	if (condition) {
		return gt(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::ge(const QString& column, const QVariant& value)
{
	QString cond = QString("%1 >= ?").arg(column);
	QVariantList values = { value };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::ge(bool condition, const QString& column, const QVariant& value)
{
	if (condition) {
		return ge(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::lt(const QString& column, const QVariant& value)
{
	QString cond = QString("%1 < ?").arg(column);
	QVariantList values = { value };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::lt(bool condition, const QString& column, const QVariant& value)
{
	if (condition) {
		return lt(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::le(const QString& column, const QVariant& value)
{
	QString cond = QString("%1 <= ?").arg(column);
	QVariantList values = { value };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::le(bool condition, const QString& column, const QVariant& value)
{
	if (condition) {
		return le(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::like(const QString& column, const QString& value)
{
	QString cond = QString("%1 LIKE ?").arg(column);
	QVariantList values = { "%" + value + "%" };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::like(bool condition, const QString& column, const QString& value)
{
	if (condition) {
		return like(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::notLike(const QString& column, const QString& value)
{
	QString cond = QString("%1 NOT LIKE ?").arg(column);
	QVariantList values = { "%" + value + "%" };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::notLike(bool condition, const QString& column, const QString& value)
{
	if (condition) {
		return notLike(column, value);
	}
	return *this;
}

QueryWrapper& QueryWrapper::in(const QString& column, const QList<QVariant>& values)
{
	if (values.isEmpty()) {
		return *this;
	}

	QString placeholders;
	QVariantList bindValues;
	for (int i = 0; i < values.size(); ++i) {
		if (i > 0) placeholders += ", ";
		placeholders += "?";
		bindValues.append(values[i]);
	}

	QString cond = QString("%1 IN (%2)").arg(column).arg(placeholders);
	addCondition(cond, bindValues);
	return *this;
}

QueryWrapper& QueryWrapper::in(bool condition, const QString& column, const QList<QVariant>& values)
{
	if (condition) {
		return in(column, values);
	}
	return *this;
}

QueryWrapper& QueryWrapper::notIn(const QString& column, const QList<QVariant>& values)
{
	if (values.isEmpty()) {
		return *this;
	}

	QString placeholders;
	QVariantList bindValues;
	for (int i = 0; i < values.size(); ++i) {
		if (i > 0) placeholders += ", ";
		placeholders += "?";
		bindValues.append(values[i]);
	}

	QString cond = QString("%1 NOT IN (%2)").arg(column).arg(placeholders);
	addCondition(cond, bindValues);
	return *this;
}

QueryWrapper& QueryWrapper::notIn(bool condition, const QString& column, const QList<QVariant>& values)
{
	if (condition) {
		return notIn(column, values);
	}
	return *this;
}

QueryWrapper& QueryWrapper::between(const QString& column, const QVariant& start, const QVariant& end)
{
	QString cond = QString("%1 BETWEEN ? AND ?").arg(column);
	QVariantList values = { start, end };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::between(bool condition, const QString& column, const QVariant& start, const QVariant& end)
{
	if (condition) {
		return between(column, start, end);
	}
	return *this;
}

QueryWrapper& QueryWrapper::notBetween(const QString& column, const QVariant& start, const QVariant& end)
{
	QString cond = QString("%1 NOT BETWEEN ? AND ?").arg(column);
	QVariantList values = { start, end };
	addCondition(cond, values);
	return *this;
}

QueryWrapper& QueryWrapper::notBetween(bool condition, const QString& column, const QVariant& start, const QVariant& end)
{
	if (condition) {
		return notBetween(column, start, end);
	}
	return *this;
}

QueryWrapper& QueryWrapper::isNull(const QString& column)
{
	QString cond = QString("%1 IS NULL").arg(column);
	addCondition(cond);
	return *this;
}

QueryWrapper& QueryWrapper::isNull(bool condition, const QString& column)
{
	if (condition) {
		return isNull(column);
	}
	return *this;
}

QueryWrapper& QueryWrapper::isNotNull(const QString& column)
{
	QString cond = QString("%1 IS NOT NULL").arg(column);
	addCondition(cond);
	return *this;
}

QueryWrapper& QueryWrapper::isNotNull(bool condition, const QString& column)
{
	if (condition) {
		return isNotNull(column);
	}
	return *this;
}

QueryWrapper& QueryWrapper::andWrapper()
{
	m_currentLogicalOp = "AND";
	return *this;
}

QueryWrapper& QueryWrapper::orWrapper()
{
	m_currentLogicalOp = "OR";
	return *this;
}

QueryWrapper& QueryWrapper::condition(const QString& condition, const QVariantList& values)
{
	addCondition(condition, values);
	return *this;
}

QueryWrapper& QueryWrapper::orderBy(bool condition, const QString& column, bool asc)
{
	if (condition) {
		return orderBy(column, asc);
	}
	return *this;
}

QueryWrapper& QueryWrapper::orderBy(const QString& column, bool asc)
{
	m_orderByColumns.append(column + (asc ? " ASC" : " DESC"));
	return *this;
}

QueryWrapper& QueryWrapper::orderByAsc(const QString& column)
{
	return orderBy(column, true);
}

QueryWrapper& QueryWrapper::orderByDesc(const QString& column)
{
	return orderBy(column, false);
}

QueryWrapper& QueryWrapper::groupBy(const QString& columns)
{
	m_groupByColumns.append(columns);
	return *this;
}

QueryWrapper& QueryWrapper::groupBy(const QStringList& columns)
{
	m_groupByColumns.append(columns);
	return *this;
}

QueryWrapper& QueryWrapper::having(const QString& havingCondition)
{
	m_havingConditions.append({ havingCondition, QVariantList() });
	return *this;
}

QueryWrapper& QueryWrapper::having(bool condition, const QString& havingCondition)
{
	if (condition) {
		return having(havingCondition);
	}
	return *this;
}

QueryWrapper& QueryWrapper::limit(int limit)
{
	m_limit = limit;
	return *this;
}

QueryWrapper& QueryWrapper::limit(int offset, int limit)
{
	m_offset = offset;
	m_limit = limit;
	return *this;
}

QueryWrapper& QueryWrapper::offset(int offset)
{
	m_offset = offset;
	return *this;
}

QueryWrapper& QueryWrapper::distinct()
{
	m_distinct = true;
	return *this;
}

QString QueryWrapper::buildSelectSql(const QString& tableName) const
{
	if (tableName.isEmpty()) {
		m_lastSql = "";
		return "";
	}

	QString sql;

	// SELECT
	sql = "SELECT ";
	if (m_distinct) {
		sql += "DISTINCT ";
	}
	sql += buildSelectColumns();

	// FROM
	sql += " FROM " + tableName;

	// WHERE
	QString whereSql = buildWhereSql();
	if (!whereSql.isEmpty()) {
		sql += " WHERE " + whereSql;
	}

	// GROUP BY
	QString groupBySql = buildGroupBySql();
	if (!groupBySql.isEmpty()) {
		sql += " GROUP BY " + groupBySql;
	}

	// HAVING
	QString havingSql = buildHavingSql();
	if (!havingSql.isEmpty()) {
		sql += " HAVING " + havingSql;
	}

	// ORDER BY
	QString orderBySql = buildOrderBySql();
	if (!orderBySql.isEmpty()) {
		sql += " ORDER BY " + orderBySql;
	}

	// LIMIT & OFFSET
	QString limitSql = buildLimitSql();
	if (!limitSql.isEmpty()) {
		sql += " " + limitSql;
	}

	m_lastSql = sql;
	return sql;
}

QString QueryWrapper::buildDeleteSql(const QString& tableName) const
{
	if (tableName.isEmpty()) {
		m_lastSql = "";
		return "";
	}

	QString sql = "DELETE FROM " + tableName;

	QString whereSql = buildWhereSql();
	if (!whereSql.isEmpty()) {
		sql += " WHERE " + whereSql;
	}

	m_lastSql = sql;
	return sql;
}

QString QueryWrapper::buildCountSql(const QString& tableName) const
{
	if (tableName.isEmpty()) {
		m_lastSql = "";
		return "";
	}

	QString sql = "SELECT COUNT(*) FROM " + tableName;

	QString whereSql = buildWhereSql();
	if (!whereSql.isEmpty()) {
		sql += " WHERE " + whereSql;
	}

	m_lastSql = sql;
	return sql;
}

QString QueryWrapper::buildUpdateSql(const QString& tableName, const QVariantMap& updateFields) const
{
	if (tableName.isEmpty() || updateFields.isEmpty()) {
		return "";
	}

	QString sql = "UPDATE " + tableName + " SET ";

	QStringList setClauses;
	QVariantList bindValues;

	// 构建SET子句
	for (auto it = updateFields.constBegin(); it != updateFields.constEnd(); ++it) {
		setClauses.append(it.key() + " = ?");
		bindValues.append(it.value());
	}

	sql += setClauses.join(", ");

	// 添加绑定值（注意：这里需要复制一份bindValues，因为m_bindValues是mutable的）
	m_bindValues = bindValues;

	// 构建WHERE条件（这会添加WHERE条件的绑定值）
	QString whereSql = buildWhereSql();
	if (!whereSql.isEmpty()) {
		sql += " WHERE " + whereSql;
	}

	m_lastSql = sql;
	return sql;
}

QList<QVariant> QueryWrapper::getBindValues() const
{
	return m_bindValues;
}

void QueryWrapper::addCondition(const QString& condition, const QVariantList& values)
{
	// 如果当前条件组为空，或者当前逻辑操作符与条件组的不同，创建新的条件组
	if (m_conditionGroups.isEmpty() ||
		m_conditionGroups.last().logicalOp != m_currentLogicalOp) {
		m_conditionGroups.append(ConditionGroup{ m_currentLogicalOp, {} });
	}

	m_conditionGroups.last().conditions.append({ condition, values });

	// 添加绑定值
	for (const auto& value : values) {
		m_bindValues.append(value);
	}

	// 重置逻辑操作符为默认值（AND）
	m_currentLogicalOp = "AND";
}

QString QueryWrapper::buildWhereSql() const
{
	if (m_conditionGroups.isEmpty() ||
		(m_conditionGroups.size() == 1 && m_conditionGroups[0].conditions.isEmpty())) {
		return "";
	}

	QString whereSql;
	bool firstGroup = true;

	for (const auto& group : m_conditionGroups) {
		if (group.conditions.isEmpty()) {
			continue;
		}

		if (!firstGroup) {
			whereSql += " " + group.logicalOp + " ";
		}

		// 如果条件组只有一个条件，直接添加
		if (group.conditions.size() == 1) {
			whereSql += group.conditions[0].first;
		}
		else {
			// 多个条件用括号括起来
			whereSql += "(";
			for (int i = 0; i < group.conditions.size(); ++i) {
				if (i > 0) {
					whereSql += " AND ";
				}
				whereSql += group.conditions[i].first;
			}
			whereSql += ")";
		}

		firstGroup = false;
	}

	return whereSql;
}

QString QueryWrapper::buildOrderBySql() const
{
	if (m_orderByColumns.isEmpty()) {
		return "";
	}
	return m_orderByColumns.join(", ");
}

QString QueryWrapper::buildGroupBySql() const
{
	if (m_groupByColumns.isEmpty()) {
		return "";
	}
	return m_groupByColumns.join(", ");
}

QString QueryWrapper::buildHavingSql() const
{
	if (m_havingConditions.isEmpty()) {
		return "";
	}

	QString havingSql;
	for (int i = 0; i < m_havingConditions.size(); ++i) {
		if (i > 0) {
			havingSql += " AND ";
		}
		havingSql += m_havingConditions[i].first;

		// 添加绑定值
		for (const auto& value : m_havingConditions[i].second) {
			m_bindValues.append(value);
		}
	}

	return havingSql;
}

QString QueryWrapper::buildLimitSql() const
{
	QString limitSql;

	if (m_limit > 0) {
		limitSql = "LIMIT " + QString::number(m_limit);
		if (m_offset >= 0) {
			limitSql += " OFFSET " + QString::number(m_offset);
		}
	}

	return limitSql;
}

QString QueryWrapper::buildSelectColumns() const
{
	return m_selectColumns;
}