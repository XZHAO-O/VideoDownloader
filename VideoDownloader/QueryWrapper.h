#pragma once

#include <QString>
#include <QVariant>
#include <QList>
#include <QPair>

class QueryWrapper
{
public:
	explicit QueryWrapper(const QString& tableName = "");
	~QueryWrapper();

	// 重置包装器
	QueryWrapper& reset();

	// 设置表名
	QueryWrapper& table(const QString& tableName);

	// SELECT 字段相关
	QueryWrapper& select(const QString& columns = "*");
	QueryWrapper& select(const QStringList& columns);

	// WHERE 条件 - 等于
	QueryWrapper& eq(const QString& column, const QVariant& value);
	QueryWrapper& eq(bool condition, const QString& column, const QVariant& value);

	// WHERE 条件 - 不等于
	QueryWrapper& ne(const QString& column, const QVariant& value);
	QueryWrapper& ne(bool condition, const QString& column, const QVariant& value);

	// WHERE 条件 - 大于
	QueryWrapper& gt(const QString& column, const QVariant& value);
	QueryWrapper& gt(bool condition, const QString& column, const QVariant& value);

	// WHERE 条件 - 大于等于
	QueryWrapper& ge(const QString& column, const QVariant& value);
	QueryWrapper& ge(bool condition, const QString& column, const QVariant& value);

	// WHERE 条件 - 小于
	QueryWrapper& lt(const QString& column, const QVariant& value);
	QueryWrapper& lt(bool condition, const QString& column, const QVariant& value);

	// WHERE 条件 - 小于等于
	QueryWrapper& le(const QString& column, const QVariant& value);
	QueryWrapper& le(bool condition, const QString& column, const QVariant& value);

	// WHERE 条件 - LIKE
	QueryWrapper& like(const QString& column, const QString& value);
	QueryWrapper& like(bool condition, const QString& column, const QString& value);

	// WHERE 条件 - NOT LIKE
	QueryWrapper& notLike(const QString& column, const QString& value);
	QueryWrapper& notLike(bool condition, const QString& column, const QString& value);

	// WHERE 条件 - IN
	QueryWrapper& in(const QString& column, const QList<QVariant>& values);
	QueryWrapper& in(bool condition, const QString& column, const QList<QVariant>& values);

	// WHERE 条件 - NOT IN
	QueryWrapper& notIn(const QString& column, const QList<QVariant>& values);
	QueryWrapper& notIn(bool condition, const QString& column, const QList<QVariant>& values);

	// WHERE 条件 - BETWEEN
	QueryWrapper& between(const QString& column, const QVariant& start, const QVariant& end);
	QueryWrapper& between(bool condition, const QString& column, const QVariant& start, const QVariant& end);

	// WHERE 条件 - NOT BETWEEN
	QueryWrapper& notBetween(const QString& column, const QVariant& start, const QVariant& end);
	QueryWrapper& notBetween(bool condition, const QString& column, const QVariant& start, const QVariant& end);

	// WHERE 条件 - IS NULL
	QueryWrapper& isNull(const QString& column);
	QueryWrapper& isNull(bool condition, const QString& column);

	// WHERE 条件 - IS NOT NULL
	QueryWrapper& isNotNull(const QString& column);
	QueryWrapper& isNotNull(bool condition, const QString& column);

	// 逻辑连接符
	QueryWrapper& andWrapper();
	QueryWrapper& orWrapper();

	// 添加自定义条件（原生SQL）
	QueryWrapper& condition(const QString& condition, const QVariantList& values = QVariantList());

	// ORDER BY
	QueryWrapper& orderBy(bool condition, const QString& column, bool asc = true);
	QueryWrapper& orderBy(const QString& column, bool asc = true);
	QueryWrapper& orderByAsc(const QString& column);
	QueryWrapper& orderByDesc(const QString& column);

	// GROUP BY
	QueryWrapper& groupBy(const QString& columns);
	QueryWrapper& groupBy(const QStringList& columns);

	// HAVING
	QueryWrapper& having(const QString& havingCondition);
	QueryWrapper& having(bool condition, const QString& havingCondition);

	// LIMIT
	QueryWrapper& limit(int limit);
	QueryWrapper& limit(int offset, int limit);

	// OFFSET
	QueryWrapper& offset(int offset);

	// DISTINCT
	QueryWrapper& distinct();

	// 构建 SQL 语句
	QString buildSelectSql() const;
	QString buildDeleteSql() const;
	QString buildCountSql() const;
	QString buildUpdateSql(const QVariantMap& updateFields) const;

	// 获取绑定的参数
	QList<QVariant> getBindValues() const;

	// 获取最后构建的SQL
	QString getLastSql() const { return m_lastSql; }

	// 获取表名
	QString getTableName() const { return m_tableName; }

	// 获取分页参数
	int getLimit() const { return m_limit; }
	int getOffset() const { return m_offset; }

private:
	struct ConditionGroup {
		QString logicalOp; // "AND" or "OR"
		QList<QPair<QString, QVariantList>> conditions;
	};

	void addCondition(const QString& condition, const QVariantList& values = QVariantList());
	QString buildWhereSql() const;
	QString buildOrderBySql() const;
	QString buildGroupBySql() const;
	QString buildHavingSql() const;
	QString buildLimitSql() const;
	QString buildSelectColumns() const;

	QString m_tableName;
	QString m_selectColumns;
	QList<ConditionGroup> m_conditionGroups;
	QList<QPair<QString, QVariantList>> m_havingConditions;
	QStringList m_orderByColumns;
	QStringList m_groupByColumns;
	int m_limit;
	int m_offset;
	bool m_distinct;
	mutable QString m_lastSql;
	mutable QList<QVariant> m_bindValues; // 注意：这里改为mutable，因为在const方法中需要修改
	QString m_currentLogicalOp; // 当前使用的逻辑运算符
};