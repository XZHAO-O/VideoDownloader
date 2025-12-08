#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMap>
#include <QDebug>
#include <QDir>
#include <QSet>

class CodeGenerator
{
public:
	// 生成选项枚举
	enum GenerateOption {
		GenerateClassOnly,      // 只生成实体类
		GenerateDaoOnly,        // 只生成DAO类
		GenerateBoth           // 两者都生成
	};

	// 对外提供的唯一函数
	static bool generateFromSql(const QString& sqlFilePath,
		GenerateOption option = GenerateBoth,
		const QString& outputDir = "",
		bool overwrite = false)
	{
		// 验证SQL文件存在
		if (!QFile::exists(sqlFilePath)) {
			qCritical() << "SQL file does not exist:" << sqlFilePath;
			return false;
		}

		// 读取SQL文件内容
		QFile sqlFile(sqlFilePath);
		if (!sqlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			qCritical() << "Failed to open SQL file:" << sqlFilePath;
			return false;
		}

		QTextStream in(&sqlFile);
		QString sqlContent = in.readAll();
		sqlFile.close();

		// 提取所有CREATE TABLE语句
		QList<TableDefinition> tableDefs = parseCreateTableStatements(sqlContent);

		if (tableDefs.isEmpty()) {
			qWarning() << "No CREATE TABLE statements found in" << sqlFilePath;
			return false;
		}

		// 设置输出目录
		QString outputPath = outputDir.isEmpty() ? QDir::currentPath() : outputDir;
		QDir dir(outputPath);
		if (!dir.exists()) {
			if (!dir.mkpath(".")) {
				qCritical() << "Failed to create output directory:" << outputPath;
				return false;
			}
		}

		bool success = true;

		// 根据选项生成代码
		for (const TableDefinition& tableDef : tableDefs) {
			switch (option) {
			case GenerateClassOnly:
				if (!generateCppClass(tableDef, outputPath, overwrite)) {
					qWarning() << "Failed to generate class for table:" << tableDef.tableName;
					success = false;
				}
				break;

			case GenerateDaoOnly:
				if (!generateDaoHeader(tableDef, outputPath, overwrite) ||
					!generateDaoImplementation(tableDef, outputPath, overwrite)) {
					qWarning() << "Failed to generate DAO for table:" << tableDef.tableName;
					success = false;
				}
				break;

			case GenerateBoth:
				if (!generateCppClass(tableDef, outputPath, overwrite)) {
					qWarning() << "Failed to generate class for table:" << tableDef.tableName;
					success = false;
				}
				if (!generateDaoHeader(tableDef, outputPath, overwrite) ||
					!generateDaoImplementation(tableDef, outputPath, overwrite)) {
					qWarning() << "Failed to generate DAO for table:" << tableDef.tableName;
					success = false;
				}
				break;
			}
		}

		qInfo() << "Code generation completed. Tables processed:" << tableDefs.size();
		return success;
	}

private:
	// 列定义
	struct ColumnDefinition
	{
		QString name;           // 列名
		QString sqlType;        // SQL类型
		QString cppType;        // C++类型
		bool isPrimaryKey;      // 是否主键
		bool isNotNull;         // 是否非空
		QString defaultValue;   // 默认值
		QString comment;        // 列注释（--后的内容）
	};

	// 表定义
	struct TableDefinition
	{
		QString tableName;      // 表名
		QString className;      // 类名
		QList<ColumnDefinition> columns; // 列列表
	};

	// SQL类型到C++类型的映射
	static QMap<QString, QString> getTypeMap()
	{
		static QMap<QString, QString> typeMap = {
			{"text", "QString"},
			{"varchar", "QString"},
			{"char", "QString"},
			{"string", "QString"},
			{"datetime", "QDateTime"},
			{"date", "QDate"},
			{"time", "QTime"},
			{"timestamp", "QDateTime"},
			{"blob", "QByteArray"},
			{"bytea", "QByteArray"},
			{"int", "int"},
			{"integer", "int"},
			{"bigint", "qlonglong"},
			{"long", "qlonglong"},
			{"smallint", "short"},
			{"tinyint", "quint8"},
			{"bool", "bool"},
			{"boolean", "bool"},
			{"real", "float"},
			{"float", "float"},
			{"double", "double"},
			{"decimal", "double"},
			{"numeric", "double"}
		};
		return typeMap;
	}

	// 将下划线命名转换为驼峰命名
	static QString toCamelCase(const QString& str, bool firstUpper = true)
	{
		QStringList parts = str.split('_', Qt::SkipEmptyParts);
		QString result;

		for (int i = 0; i < parts.size(); ++i) {
			QString part = parts[i];
			if (part.isEmpty()) continue;

			if (i == 0 && !firstUpper) {
				// 第一个单词首字母小写
				result += part[0].toLower() + part.mid(1);
			}
			else {
				// 其他单词首字母大写
				result += part[0].toUpper() + part.mid(1);
			}
		}

		return result.isEmpty() ? str : result;
	}

	// 解析SQL类型到C++类型
	static QString sqlTypeToCppType(const QString& sqlType)
	{
		QString lowerType = sqlType.toLower();

		// 处理带括号的类型，如VARCHAR(255)
		int parenIndex = lowerType.indexOf('(');
		if (parenIndex != -1) {
			lowerType = lowerType.left(parenIndex);
		}

		// 处理数组类型，如text[]
		if (lowerType.endsWith("[]")) {
			lowerType = lowerType.left(lowerType.length() - 2);
		}

		QMap<QString, QString> typeMap = getTypeMap();

		if (typeMap.contains(lowerType)) {
			return typeMap[lowerType];
		}

		// 默认返回QString
		qWarning() << "Unknown SQL type:" << sqlType << ", using QString as default";
		return "QString";
	}

	// 提取注释（从--开始到行尾）
	static QString extractComment(const QString& line)
	{
		int commentIndex = line.indexOf("--");
		if (commentIndex != -1) {
			QString comment = line.mid(commentIndex + 2).trimmed();
			return comment;
		}
		return "";
	}

	// 提取列定义
	static ColumnDefinition parseColumnDefinition(const QString& columnDef)
	{
		ColumnDefinition column;

		// 如果字符串为空，直接返回
		if (columnDef.trimmed().isEmpty()) {
			return column;
		}

		// 提取注释
		column.comment = extractComment(columnDef);

		// 移除注释部分，只保留列定义部分
		QString columnDefWithoutComment = columnDef;
		int commentIndex = columnDef.indexOf("--");
		if (commentIndex != -1) {
			columnDefWithoutComment = columnDef.left(commentIndex);
		}

		// 清理字符串
		columnDefWithoutComment = columnDefWithoutComment.trimmed();
		if (columnDefWithoutComment.endsWith(',')) {
			columnDefWithoutComment = columnDefWithoutComment.left(columnDefWithoutComment.length() - 1).trimmed();
		}

		// 使用正则表达式解析列定义
		// 格式：column_name data_type [constraints]
		QRegularExpression regex(
			R"(\s*([\w_]+)\s+([\w\.]+(?:\([^)]+\))?(?:\[\])?)\s*(.*))",
			QRegularExpression::CaseInsensitiveOption
		);

		QRegularExpressionMatch match = regex.match(columnDefWithoutComment);
		if (match.hasMatch()) {
			column.name = match.captured(1);
			column.sqlType = match.captured(2);
			column.cppType = sqlTypeToCppType(column.sqlType);

			QString constraints = match.captured(3).toLower();
			column.isPrimaryKey = constraints.contains("primary key");
			column.isNotNull = constraints.contains("not null");

			// 提取默认值
			QRegularExpression defaultValueRegex(R"(default\s+([^\s,]+(?:\([^)]+\))?))",
				QRegularExpression::CaseInsensitiveOption);
			QRegularExpressionMatch defaultValueMatch = defaultValueRegex.match(constraints);
			if (defaultValueMatch.hasMatch()) {
				column.defaultValue = defaultValueMatch.captured(1);
			}
		}

		return column;
	}

	// 解析CREATE TABLE语句
	static QList<TableDefinition> parseCreateTableStatements(const QString& sqlContent)
	{
		QList<TableDefinition> tableDefs;

		// 正则表达式匹配CREATE TABLE语句
		QRegularExpression createTableRegex(
			R"(CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?([\w_]+)\s*\(\s*([^;]+)\s*\)\s*;)",
			QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
		);

		QRegularExpressionMatchIterator matches = createTableRegex.globalMatch(sqlContent);

		while (matches.hasNext()) {
			QRegularExpressionMatch match = matches.next();
			TableDefinition tableDef;
			tableDef.tableName = match.captured(1);
			tableDef.className = toCamelCase(tableDef.tableName, true);

			QString columnsStr = match.captured(2);

			// 按行分割列定义
			QStringList lines = columnsStr.split('\n', Qt::SkipEmptyParts);

			// 解析每个列定义
			for (QString line : lines) {
				line = line.trimmed();

				// 跳过空行
				if (line.isEmpty()) {
					continue;
				}

				// 跳过约束定义（如PRIMARY KEY, FOREIGN KEY, CHECK等）
				QString lowerLine = line.toLower();
				if (lowerLine.startsWith("primary key") ||
					lowerLine.startsWith("foreign key") ||
					lowerLine.startsWith("check") ||
					lowerLine.startsWith("unique") ||
					lowerLine.startsWith("constraint")) {
					continue;
				}

				// 如果以逗号结尾，去除逗号
				if (line.endsWith(',')) {
					line = line.left(line.length() - 1).trimmed();
				}

				// 跳过空行
				if (line.isEmpty()) {
					continue;
				}

				ColumnDefinition column = parseColumnDefinition(line);
				if (!column.name.isEmpty()) {
					tableDef.columns << column;
				}
			}

			if (!tableDef.columns.isEmpty()) {
				tableDefs << tableDef;
			}
		}

		return tableDefs;
	}

	// 生成C++实体类
	static bool generateCppClass(const TableDefinition& tableDef, const QString& outputDir, bool overwrite = false)
	{
		QDir dir(outputDir);
		QString headerFileName = tableDef.className + ".h";
		QString headerPath = dir.filePath(headerFileName);

		// 检查文件是否存在，根据overwrite参数决定是否继续
		if (QFile::exists(headerPath) && !overwrite) {
			qWarning() << "Class file already exists:" << headerPath << "(use overwrite=true to override)";
			return false;
		}

		QFile headerFile(headerPath);
		if (!headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			qCritical() << "Failed to create header file:" << headerPath;
			return false;
		}

		QTextStream out(&headerFile);
		out.setEncoding(QStringConverter::Utf8);

		// 生成头文件内容
		out << "#pragma once\n\n";

		// 包含必要的头文件
		QSet<QString> includes;
		includes << "#include <QString>";

		for (const ColumnDefinition& column : tableDef.columns) {
			if (column.cppType == "QDateTime") {
				includes << "#include <QDateTime>";
			}
			else if (column.cppType == "QDate") {
				includes << "#include <QDate>";
			}
			else if (column.cppType == "QTime") {
				includes << "#include <QTime>";
			}
			else if (column.cppType == "QByteArray") {
				includes << "#include <QByteArray>";
			}
			else if (column.cppType == "qlonglong" || column.cppType == "qint64") {
				includes << "#include <QtGlobal>";
			}
		}

		QList<QString> sortedIncludes = includes.values();
		std::sort(sortedIncludes.begin(), sortedIncludes.end());
		for (const QString& include : sortedIncludes) {
			out << include << "\n";
		}
		out << "\n";

		// 类声明
		out << "// 表名: " << tableDef.tableName << "\n";
		out << "class " << tableDef.className << "\n";
		out << "{\n";
		out << "public:\n";
		out << "    " << tableDef.className << "() = default;\n\n";

		// 成员变量
		for (const ColumnDefinition& column : tableDef.columns) {
			QString memberName = toCamelCase(column.name, false);
			out << "    " << column.cppType << " " << memberName << ";";

			// 如果存在注释，则添加注释
			if (!column.comment.isEmpty()) {
				out << " // " << column.comment;
			}
			out << "\n";
		}
		out << "\n";

		// isValid函数（如果主键不为空）
		for (const ColumnDefinition& column : tableDef.columns) {
			if (column.isPrimaryKey) {
				QString memberName = toCamelCase(column.name, false);
				out << "    bool isValid() const { ";
				if (column.cppType == "QString" || column.cppType == "QByteArray") {
					out << "return !" << memberName << ".isEmpty(); }\n";
				}
				else if (column.cppType == "QDateTime" || column.cppType == "QDate" || column.cppType == "QTime") {
					out << "return " << memberName << ".isValid(); }\n";
				}
				else if (column.cppType == "int" || column.cppType == "short" || column.cppType == "qlonglong") {
					out << "return " << memberName << " != 0; }\n";
				}
				else {
					out << "return true; }\n";
				}
				break;
			}
		}

		// 如果没有主键，添加默认的isValid函数
		bool hasPrimaryKey = false;
		for (const ColumnDefinition& column : tableDef.columns) {
			if (column.isPrimaryKey) {
				hasPrimaryKey = true;
				break;
			}
		}

		if (!hasPrimaryKey) {
			out << "    bool isValid() const { return false; }\n";
		}

		out << "};\n";

		headerFile.close();

		qDebug() << "Generated class:" << headerPath << (overwrite ? "(overwritten)" : "");
		return true;
	}

	// 生成DAO头文件（按DownloadRecordDAO风格）
	static bool generateDaoHeader(const TableDefinition& tableDef, const QString& outputDir, bool overwrite = false)
	{
		QDir dir(outputDir);
		QString daoFileName = tableDef.className + "DAO.h";
		QString daoPath = dir.filePath(daoFileName);

		// 检查文件是否存在，根据overwrite参数决定是否继续
		if (QFile::exists(daoPath) && !overwrite) {
			qWarning() << "DAO file already exists:" << daoPath << "(use overwrite=true to override)";
			return false;
		}

		QFile daoFile(daoPath);
		if (!daoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			qCritical() << "Failed to create DAO file:" << daoPath;
			return false;
		}

		QTextStream out(&daoFile);
		out.setEncoding(QStringConverter::Utf8);

		// 查找主键列
		QString primaryKeyName;
		QString primaryKeyCppType;
		for (const ColumnDefinition& column : tableDef.columns) {
			if (column.isPrimaryKey) {
				primaryKeyName = column.name;
				primaryKeyCppType = column.cppType;
				break;
			}
		}

		// 生成变量名：类名首字母小写
		QString classNameLower = toCamelCase(tableDef.className, false);

		out << "#pragma once\n\n";
		out << "#include \"" << tableDef.className << ".h\"\n\n";

		out << "#include <QVariantMap>\n";
		out << "#include <functional>\n\n";

		out << "class QSqlQuery;\n\n";
		out << "class QueryWrapper;\n";
		out << "class DatabaseManager;\n\n";

		out << "class " << tableDef.className << "DAO\n";
		out << "{\n";
		out << "public:\n";
		out << "    explicit " << tableDef.className << "DAO(QSharedPointer<DatabaseManager> dbManager);\n";
		out << "    ~" << tableDef.className << "DAO();\n\n";

		out << "    // 表操作\n";
		out << "    bool createTable();\n";
		out << "    bool dropTable();\n\n";

		out << "    // CRUD操作\n";
		out << "    bool insert(const " << tableDef.className << "& " << classNameLower << ");\n";
		out << "    bool update(const " << tableDef.className << "& " << classNameLower << ");\n";

		// 根据主键生成删除方法
		if (!primaryKeyName.isEmpty()) {
			QString paramName = toCamelCase(primaryKeyName, false);
			out << "    bool remove(const " << primaryKeyCppType << "& " << paramName << ");\n";
			out << "    QList<" << tableDef.className << "> getById(const " << primaryKeyCppType << "& " << paramName << ");\n";
		}
		else {
			out << "    bool remove(const QString& id);\n";
			out << "    QList<" << tableDef.className << "> getById(const QString& id);\n";
		}

		out << "    bool insertBatch(const QList<" << tableDef.className << ">& " << classNameLower << "s);\n";
		out << "    int count();\n\n";

		out << "    QList<" << tableDef.className << "> selectList(const QueryWrapper& wrapper);\n";
		out << "    int selectCount(const QueryWrapper& wrapper);\n";
		out << "    bool deleteByWrapper(const QueryWrapper& wrapper);\n";
		out << "    bool updateByWrapper(const QueryWrapper& wrapper, const QVariantMap& updateFields);\n\n";

		out << "    // 分页查询\n";
		out << "    QList<" << tableDef.className << "> selectPage(const QueryWrapper& wrapper, int pageNum, int pageSize);\n\n";

		out << "    // 查询操作\n";
		out << "    bool executeQuery(const QString& queryStr, const QVariantMap& params);\n";
		out << "    bool executeQuery(const QString& queryStr, const QVariantList& params = QVariantList());\n";
		out << "    QList<" << tableDef.className << "> executeSelect(const QString& queryStr, const QVariantMap& params);\n";
		out << "    QList<" << tableDef.className << "> executeSelect(const QString& queryStr, const QVariantList& params = QVariantList());\n\n";

		out << "private:\n";
		out << "    QVariantMap toMap(const " << tableDef.className << "& " << classNameLower << ");\n";
		out << "    void fillFromQueryResult(const QVariantMap& result, " << tableDef.className << "& " << classNameLower << ");\n\n";

		out << "private:\n";
		out << "    QSharedPointer<DatabaseManager> m_dbManager;\n";
		out << "};";

		daoFile.close();

		qDebug() << "Generated DAO header:" << daoPath << (overwrite ? "(overwritten)" : "");
		return true;
	}

	// 生成DAO实现文件（按DownloadRecordDAO风格）
	static bool generateDaoImplementation(const TableDefinition& tableDef, const QString& outputDir, bool overwrite = false)
	{
		QDir dir(outputDir);
		QString daoFileName = tableDef.className + "DAO.cpp";
		QString daoPath = dir.filePath(daoFileName);

		// 检查文件是否存在，根据overwrite参数决定是否继续
		if (QFile::exists(daoPath) && !overwrite) {
			qWarning() << "DAO implementation file already exists:" << daoPath << "(use overwrite=true to override)";
			return false;
		}

		QFile daoFile(daoPath);
		if (!daoFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			qCritical() << "Failed to create DAO implementation file:" << daoPath;
			return false;
		}

		QTextStream out(&daoFile);
		out.setEncoding(QStringConverter::Utf8);

		// 查找主键列
		QString primaryKeyName;
		QString primaryKeyCppType;
		for (const ColumnDefinition& column : tableDef.columns) {
			if (column.isPrimaryKey) {
				primaryKeyName = column.name;
				primaryKeyCppType = column.cppType;
				break;
			}
		}

		// 生成变量名：类名首字母小写
		QString classNameLower = toCamelCase(tableDef.className, false);

		// 生成实现文件内容
		out << "#include \"" << tableDef.className << "DAO.h\"\n\n";
		out << "#include \"QueryWrapper.h\"\n";
		out << "#include \"DatabaseManager.h\"\n\n";

		// 命名空间定义
		out << "// 编译时常量定义\n";
		out << "namespace\n{\n";
		out << "    const QString TABLE_NAME = \"" << tableDef.tableName << "\";\n\n";

		// 生成CREATE TABLE SQL语句
		out << "    // SQL语句模板\n";
		out << "    const QString CREATE_TABLE_SQL =\n";
		out << "        \"CREATE TABLE IF NOT EXISTS " << tableDef.tableName << " (\"\n";

		// 生成列定义部分
		for (int i = 0; i < tableDef.columns.size(); ++i) {
			const ColumnDefinition& column = tableDef.columns[i];
			out << "        \"    " << column.name << " " << column.sqlType;

			if (column.isNotNull) {
				out << " NOT NULL";
			}
			if (column.isPrimaryKey) {
				out << " PRIMARY KEY";
			}
			if (!column.defaultValue.isEmpty()) {
				out << " DEFAULT " << column.defaultValue;
			}

			if (i < tableDef.columns.size() - 1) {
				out << ",\"\n";
			}
			else {
				out << "\"\n";
			}
		}
		out << "        \");\";\n\n";

		out << "    const QString DROP_TABLE_SQL = \"DROP TABLE IF EXISTS " << tableDef.tableName << "\";\n\n";

		// INSERT SQL - 使用INSERT OR REPLACE
		out << "    const QString INSERT_SQL =\n";
		out << "        \"INSERT OR REPLACE INTO " << tableDef.tableName << " \"\n";
		out << "        \"(";
		for (int i = 0; i < tableDef.columns.size(); ++i) {
			out << tableDef.columns[i].name;
			if (i < tableDef.columns.size() - 1) out << ", ";
		}
		out << ") \"\n";
		out << "        \"VALUES \"\n";
		out << "        \"(";
		for (int i = 0; i < tableDef.columns.size(); ++i) {
			out << ":" << tableDef.columns[i].name;
			if (i < tableDef.columns.size() - 1) out << ", ";
		}
		out << ")\";\n\n";

		// DELETE SQL
		if (!primaryKeyName.isEmpty()) {
			out << "    const QString DELETE_SQL = \"DELETE FROM " << tableDef.tableName << " WHERE "
				<< primaryKeyName << " = ?\";\n\n";
		}
		else {
			out << "    const QString DELETE_SQL = \"DELETE FROM " << tableDef.tableName << " WHERE id = ?\";\n\n";
		}

		// SELECT BY ID SQL
		if (!primaryKeyName.isEmpty()) {
			out << "    const QString SELECT_BY_ID_SQL = \"SELECT * FROM " << tableDef.tableName
				<< " WHERE " << primaryKeyName << " = ?\";\n\n";
		}
		else {
			out << "    const QString SELECT_BY_ID_SQL = \"SELECT * FROM " << tableDef.tableName
				<< " WHERE id = ?\";\n\n";
		}

		out << "    const QString COUNT_SQL = \"SELECT COUNT(*) FROM " << tableDef.tableName << "\";\n";
		out << "}\n\n";

		// 构造函数
		out << tableDef.className << "DAO::" << tableDef.className << "DAO(QSharedPointer<DatabaseManager> dbManager)\n";
		out << "    : m_dbManager(dbManager)\n";
		out << "{\n";
		out << "    createTable();\n";
		out << "}\n\n";

		out << tableDef.className << "DAO::~" << tableDef.className << "DAO()\n";
		out << "{\n";
		out << "}\n\n";

		// createTable
		out << "bool " << tableDef.className << "DAO::createTable()\n";
		out << "{\n";
		out << "    return m_dbManager->executeQuery(CREATE_TABLE_SQL);\n";
		out << "}\n\n";

		// dropTable
		out << "bool " << tableDef.className << "DAO::dropTable()\n";
		out << "{\n";
		out << "    return m_dbManager->executeQuery(DROP_TABLE_SQL);\n";
		out << "}\n\n";

		// toMap
		out << "QVariantMap " << tableDef.className << "DAO::toMap(const " << tableDef.className << "& " << classNameLower << ")\n";
		out << "{\n";
		out << "    QVariantMap map;\n";
		for (const ColumnDefinition& column : tableDef.columns) {
			QString memberName = toCamelCase(column.name, false);
			out << "    map[\":" << column.name << "\"] = " << classNameLower << "." << memberName << ";\n";
		}
		out << "\n    return map;\n";
		out << "}\n\n";

		// fillFromQueryResult
		out << "void " << tableDef.className << "DAO::fillFromQueryResult(const QVariantMap& result, "
			<< tableDef.className << "& " << classNameLower << ")\n";
		out << "{\n";
		for (const ColumnDefinition& column : tableDef.columns) {
			QString memberName = toCamelCase(column.name, false);
			out << "    " << classNameLower << "." << memberName << " = result.value(\"" << column.name << "\")";

			// 根据类型添加对应的转换函数
			if (column.cppType == "QDateTime") {
				out << ".toDateTime()";
			}
			else if (column.cppType == "QDate") {
				out << ".toDate()";
			}
			else if (column.cppType == "QTime") {
				out << ".toTime()";
			}
			else if (column.cppType == "int" || column.cppType == "short" || column.cppType == "quint8") {
				out << ".toInt()";
			}
			else if (column.cppType == "bool") {
				out << ".toBool()";
			}
			else if (column.cppType == "float" || column.cppType == "double") {
				out << ".toDouble()";
			}
			else if (column.cppType == "qlonglong" || column.cppType == "qint64") {
				out << ".toLongLong()";
			}
			else if (column.cppType == "QByteArray") {
				out << ".toByteArray()";
			}
			else if (column.cppType == "QString") {
				out << ".toString()";
			}

			out << ";\n";
		}
		out << "}\n\n";

		// insert
		out << "bool " << tableDef.className << "DAO::insert(const " << tableDef.className << "& " << classNameLower << ")\n";
		out << "{\n";
		out << "    return m_dbManager->executeQuery(INSERT_SQL, toMap(" << classNameLower << "));\n";
		out << "}\n\n";

		// update
		out << "bool " << tableDef.className << "DAO::update(const " << tableDef.className << "& " << classNameLower << ")\n";
		out << "{\n";
		out << "    return insert(" << classNameLower << ");\n";
		out << "}\n\n";

		// remove
		if (!primaryKeyName.isEmpty()) {
			QString paramName = toCamelCase(primaryKeyName, false);
			out << "bool " << tableDef.className << "DAO::remove(const " << primaryKeyCppType << "& "
				<< paramName << ")\n";
			out << "{\n";
			out << "    QVariantList params;\n";
			out << "    params << " << paramName << ";\n\n";
			out << "    return m_dbManager->executeQuery(DELETE_SQL, params);\n";
			out << "}\n\n";

			// getById
			out << "QList<" << tableDef.className << "> " << tableDef.className << "DAO::getById(const "
				<< primaryKeyCppType << "& " << paramName << ")\n";
			out << "{\n";
			out << "    QVariantList params;\n";
			out << "    params << " << paramName << ";\n\n";
			out << "    return executeSelect(SELECT_BY_ID_SQL, params);\n";
			out << "}\n\n";
		}
		else {
			out << "bool " << tableDef.className << "DAO::remove(const QString& id)\n";
			out << "{\n";
			out << "    QVariantList params;\n";
			out << "    params << id;\n\n";
			out << "    return m_dbManager->executeQuery(DELETE_SQL, params);\n";
			out << "}\n\n";

			out << "QList<" << tableDef.className << "> " << tableDef.className << "DAO::getById(const QString& id)\n";
			out << "{\n";
			out << "    QVariantList params;\n";
			out << "    params << id;\n\n";
			out << "    return executeSelect(SELECT_BY_ID_SQL, params);\n";
			out << "}\n\n";
		}

		// insertBatch
		out << "bool " << tableDef.className << "DAO::insertBatch(const QList<" << tableDef.className << ">& " << classNameLower << "s)\n";
		out << "{\n";
		out << "    if (!m_dbManager->beginTransaction())\n";
		out << "    {\n";
		out << "        return false;\n";
		out << "    }\n\n";
		out << "    for (const auto& " << classNameLower << " : " << classNameLower << "s)\n";
		out << "    {\n";
		out << "        if (!insert(" << classNameLower << "))\n";
		out << "        {\n";
		out << "            m_dbManager->rollbackTransaction();\n";
		out << "            return false;\n";
		out << "        }\n";
		out << "    }\n\n";
		out << "    return m_dbManager->commitTransaction();\n";
		out << "}\n\n";

		// count
		out << "int " << tableDef.className << "DAO::count()\n";
		out << "{\n";
		out << "    int result = 0;\n";
		out << "    auto results = m_dbManager->executeQueryToMap(COUNT_SQL);\n";
		out << "    if (!results.isEmpty())\n";
		out << "    {\n";
		out << "        result = results.first().value(0).toInt();\n";
		out << "    }\n";
		out << "    return result;\n";
		out << "}\n\n";

		// selectList
		out << "QList<" << tableDef.className << "> " << tableDef.className << "DAO::selectList(const QueryWrapper& wrapper)\n";
		out << "{\n";
		out << "    QString sql = wrapper.buildSelectSql();\n";
		out << "    QVariantList params = wrapper.getBindValues();\n\n";
		out << "    if (sql.isEmpty())\n";
		out << "    {\n";
		out << "        return QList<" << tableDef.className << ">();\n";
		out << "    }\n\n";
		out << "    return executeSelect(sql, params);\n";
		out << "}\n\n";

		// selectCount
		out << "int " << tableDef.className << "DAO::selectCount(const QueryWrapper& wrapper)\n";
		out << "{\n";
		out << "    QString sql = wrapper.buildCountSql();\n";
		out << "    QVariantList params = wrapper.getBindValues();\n\n";
		out << "    if (sql.isEmpty())\n";
		out << "    {\n";
		out << "        return 0;\n";
		out << "    }\n\n";
		out << "    auto results = m_dbManager->executeQueryToMap(sql, params);\n";
		out << "    if (!results.isEmpty())\n";
		out << "    {\n";
		out << "        return results.first().value(0).toInt();\n";
		out << "    }\n";
		out << "    return 0;\n";
		out << "}\n\n";

		// deleteByWrapper
		out << "bool " << tableDef.className << "DAO::deleteByWrapper(const QueryWrapper& wrapper)\n";
		out << "{\n";
		out << "    QString sql = wrapper.buildDeleteSql();\n";
		out << "    QVariantList params = wrapper.getBindValues();\n\n";
		out << "    if (sql.isEmpty())\n";
		out << "    {\n";
		out << "        return false;\n";
		out << "    }\n\n";
		out << "    return m_dbManager->executeQuery(sql, params);\n";
		out << "}\n\n";

		// updateByWrapper
		out << "bool " << tableDef.className << "DAO::updateByWrapper(const QueryWrapper& wrapper, const QVariantMap& updateFields)\n";
		out << "{\n";
		out << "    QString sql = wrapper.buildUpdateSql(updateFields);\n";
		out << "    QVariantList params = wrapper.getBindValues();\n\n";
		out << "    if (sql.isEmpty())\n";
		out << "    {\n";
		out << "        return false;\n";
		out << "    }\n\n";
		out << "    return m_dbManager->executeQuery(sql, params);\n";
		out << "}\n\n";

		// selectPage
		out << "QList<" << tableDef.className << "> " << tableDef.className << "DAO::selectPage(const QueryWrapper& wrapper, int pageNum, int pageSize)\n";
		out << "{\n";
		out << "    QueryWrapper pageWrapper = wrapper;\n";
		out << "    pageWrapper.limit((pageNum - 1) * pageSize, pageSize);\n\n";
		out << "    return selectList(pageWrapper);\n";
		out << "}\n\n";

		// executeQuery (QVariantList)
		out << "bool " << tableDef.className << "DAO::executeQuery(const QString& queryStr, const QVariantList& params)\n";
		out << "{\n";
		out << "    return m_dbManager->executeQuery(queryStr, params);\n";
		out << "}\n\n";

		// executeQuery (QVariantMap)
		out << "bool " << tableDef.className << "DAO::executeQuery(const QString& queryStr, const QVariantMap& params)\n";
		out << "{\n";
		out << "    return m_dbManager->executeQuery(queryStr, params);\n";
		out << "}\n\n";

		// executeSelect (QVariantList)
		out << "QList<" << tableDef.className << "> " << tableDef.className << "DAO::executeSelect(const QString& queryStr,\n";
		out << "    const QVariantList& params)\n";
		out << "{\n";
		out << "    auto results = m_dbManager->executeQueryToMap(queryStr, params);\n";
		out << "    QList<" << tableDef.className << "> " << classNameLower << "s;\n";
		out << "    for (const auto& result : results)\n";
		out << "    {\n";
		out << "        " << tableDef.className << " " << classNameLower << ";\n";
		out << "        fillFromQueryResult(result, " << classNameLower << ");\n";
		out << "        " << classNameLower << "s.append(" << classNameLower << ");\n";
		out << "    }\n";
		out << "    return " << classNameLower << "s;\n";
		out << "}\n\n";

		// executeSelect (QVariantMap)
		out << "QList<" << tableDef.className << "> " << tableDef.className << "DAO::executeSelect(const QString& queryStr,\n";
		out << "    const QVariantMap& params)\n";
		out << "{\n";
		out << "    auto results = m_dbManager->executeQueryToMap(queryStr, params);\n";
		out << "    QList<" << tableDef.className << "> " << classNameLower << "s;\n";
		out << "    for (const auto& result : results)\n";
		out << "    {\n";
		out << "        " << tableDef.className << " " << classNameLower << ";\n";
		out << "        fillFromQueryResult(result, " << classNameLower << ");\n";
		out << "        " << classNameLower << "s.append(" << classNameLower << ");\n";
		out << "    }\n";
		out << "    return " << classNameLower << "s;\n";
		out << "}";

		daoFile.close();

		qDebug() << "Generated DAO implementation:" << daoPath << (overwrite ? "(overwritten)" : "");
		return true;
	}
};