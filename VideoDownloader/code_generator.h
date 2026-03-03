// Copyright 2026 XZHAO_O. All rights reserved.
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMap>
#include <QDebug>
#include <QDir>
#include <QSet>

#include "database_id.h"

namespace nexusdl::database {

	class CodeGenerator
	{
	public:
		enum GenerateOption {
			GenerateClassOnly = 0,
			GenerateDaoOnly,
			GenerateServiceOnly,
			GenerateClassAndDao,
			GenerateClassAndService,
			GenerateDaoAndService,
			GenerateAll
		};

		// 对外接口，增加 databaseId 参数指定表所属的数据库
		static bool generateFromSql(const QString& sqlFilePath,
			GenerateOption option = GenerateAll,
			const QString& outputDir = "",
			bool overwrite = false,
			DatabaseId dbId = DatabaseId::Download)
		{
			if (!QFile::exists(sqlFilePath)) {
				qCritical() << "SQL file does not exist:" << sqlFilePath;
				return false;
			}

			QFile sqlFile(sqlFilePath);
			if (!sqlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
				qCritical() << "Failed to open SQL file:" << sqlFilePath;
				return false;
			}

			QTextStream in(&sqlFile);
			QString sqlContent = in.readAll();
			sqlFile.close();

			QList<TableDefinition> tableDefs = parseCreateTableStatements(sqlContent);
			if (tableDefs.isEmpty()) {
				qWarning() << "No CREATE TABLE statements found in" << sqlFilePath;
				return false;
			}

			QString outputPath = outputDir.isEmpty() ? QDir::currentPath() : outputDir;
			QDir dir(outputPath);
			if (!dir.exists() && !dir.mkpath(".")) {
				qCritical() << "Failed to create output directory:" << outputPath;
				return false;
			}

			bool success = true;
			for (const TableDefinition& tableDef : tableDefs) {
				const bool generateClass = (option == GenerateClassOnly || option == GenerateClassAndDao ||
					option == GenerateClassAndService || option == GenerateAll);
				const bool generateDao = (option == GenerateDaoOnly || option == GenerateClassAndDao ||
					option == GenerateDaoAndService || option == GenerateAll);
				const bool generateService = (option == GenerateServiceOnly || option == GenerateClassAndService ||
					option == GenerateDaoAndService || option == GenerateAll);

				if (generateClass) {
					if (!generateCppClass(tableDef, outputPath, dbId, overwrite))
						success = false;
				}
				if (generateDao) {
					if (!generateDaoHeader(tableDef, outputPath, overwrite) ||
						!generateDaoImplementation(tableDef, outputPath, overwrite))
						success = false;
				}
				if (generateService) {
					if (!generateServiceHeader(tableDef, outputPath, overwrite) ||
						!generateServiceImplementation(tableDef, outputPath, overwrite))
						success = false;
				}
			}

			qInfo() << "Code generation completed. Tables processed:" << tableDefs.size();
			return success;
		}

	private:
		struct ColumnDefinition
		{
			QString name;
			QString sqlType;
			QString cppType;
			bool isPrimaryKey = false;
			bool isNotNull = false;
			QString defaultValue;
			QString comment;
		};

		struct TableDefinition
		{
			QString tableName;
			QString className;
			QList<ColumnDefinition> columns;
		};

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

		static QString toCamelCase(const QString& str, bool firstUpper = true)
		{
			QStringList parts = str.split('_', Qt::SkipEmptyParts);
			QString result;
			for (int i = 0; i < parts.size(); ++i) {
				QString part = parts[i];
				if (part.isEmpty()) continue;
				if (i == 0 && !firstUpper)
					result += part[0].toLower() + part.mid(1);
				else
					result += part[0].toUpper() + part.mid(1);
			}
			return result.isEmpty() ? str : result;
		}

		static QString sqlTypeToCppType(const QString& sqlType)
		{
			QString lowerType = sqlType.toLower();
			const int parenIndex = lowerType.indexOf('(');
			if (parenIndex != -1)
				lowerType = lowerType.left(parenIndex);
			if (lowerType.endsWith("[]"))
				lowerType = lowerType.left(lowerType.length() - 2);

			auto typeMap = getTypeMap();
			if (typeMap.contains(lowerType))
				return typeMap[lowerType];

			qWarning() << "Unknown SQL type:" << sqlType << ", using QString as default";
			return "QString";
		}

		static QString extractComment(const QString& line)
		{
			const int commentIndex = line.indexOf("--");
			if (commentIndex != -1)
				return line.mid(commentIndex + 2).trimmed();
			return QString();
		}

		static ColumnDefinition parseColumnDefinition(const QString& columnDef)
		{
			ColumnDefinition column;
			if (columnDef.trimmed().isEmpty())
				return column;

			column.comment = extractComment(columnDef);
			QString defWithoutComment = columnDef;
			const int commentIndex = columnDef.indexOf("--");
			if (commentIndex != -1)
				defWithoutComment = columnDef.left(commentIndex);
			defWithoutComment = defWithoutComment.trimmed();
			if (defWithoutComment.endsWith(','))
				defWithoutComment.chop(1);

			QRegularExpression regex(
				R"(\s*([\w_]+)\s+([\w\.]+(?:\([^)]+\))?(?:\[\])?)\s*(.*))",
				QRegularExpression::CaseInsensitiveOption
			);
			QRegularExpressionMatch match = regex.match(defWithoutComment);
			if (match.hasMatch()) {
				column.name = match.captured(1);
				column.sqlType = match.captured(2);
				column.cppType = sqlTypeToCppType(column.sqlType);
				QString constraints = match.captured(3).toLower();
				column.isPrimaryKey = constraints.contains("primary key");
				column.isNotNull = constraints.contains("not null");

				QRegularExpression defaultRegex(R"(default\s+([^\s,]+(?:\([^)]+\))?))",
					QRegularExpression::CaseInsensitiveOption);
				QRegularExpressionMatch defaultMatch = defaultRegex.match(constraints);
				if (defaultMatch.hasMatch())
					column.defaultValue = defaultMatch.captured(1);
			}
			return column;
		}

		static QList<TableDefinition> parseCreateTableStatements(const QString& sqlContent)
		{
			QList<TableDefinition> tableDefs;
			QRegularExpression createTableRegex(
				R"(CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?([\w_]+)\s*\(\s*([^;]+)\s*\)\s*;)",
				QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
			);

			auto matches = createTableRegex.globalMatch(sqlContent);
			while (matches.hasNext()) {
				auto match = matches.next();
				TableDefinition tableDef;
				tableDef.tableName = match.captured(1);
				tableDef.className = toCamelCase(tableDef.tableName, true);

				QString columnsStr = match.captured(2);
				QStringList lines = columnsStr.split('\n', Qt::SkipEmptyParts);

				for (QString line : lines) {
					line = line.trimmed();
					if (line.isEmpty()) continue;

					QString lowerLine = line.toLower();
					if (lowerLine.startsWith("primary key") ||
						lowerLine.startsWith("foreign key") ||
						lowerLine.startsWith("check") ||
						lowerLine.startsWith("unique") ||
						lowerLine.startsWith("constraint")) {
						continue;
					}

					if (line.endsWith(','))
						line.chop(1);
					line = line.trimmed();
					if (line.isEmpty()) continue;

					ColumnDefinition column = parseColumnDefinition(line);
					if (!column.name.isEmpty())
						tableDef.columns << column;
				}

				if (!tableDef.columns.isEmpty())
					tableDefs << tableDef;
			}
			return tableDefs;
		}

		// ---------- 实体类生成 ----------
		static bool generateCppClass(const TableDefinition& tableDef,
			const QString& outputDir,
			DatabaseId dbId,
			bool overwrite)
		{
			QDir dir(outputDir);
			QString headerPath = dir.filePath(tableDef.className + ".h");
			if (QFile::exists(headerPath) && !overwrite) {
				qWarning() << "Class file already exists:" << headerPath;
				return false;
			}

			QFile headerFile(headerPath);
			if (!headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				qCritical() << "Failed to create header file:" << headerPath;
				return false;
			}

			QTextStream out(&headerFile);
			out.setEncoding(QStringConverter::Utf8);

			// 版权声明
			out << "// Copyright 2026 XZHAO_O. All rights reserved.\n";
			out << "// SPDX-License-Identifier: MIT\n\n";

			// 头文件包含
			out << "#pragma once\n\n";
			out << "#include <QSqlRecord>\n";
			out << "#include <QVariantMap>\n";
			out << "#include \"base_entity.h\"\n";
			out << "#include \"database_id.h\"\n";
			out << "#include \"field.h\"\n";

			QSet<QString> extraIncludes;
			for (const auto& col : tableDef.columns) {
				if (col.cppType == "QDateTime") extraIncludes << "#include <QDateTime>";
				else if (col.cppType == "QDate") extraIncludes << "#include <QDate>";
				else if (col.cppType == "QTime") extraIncludes << "#include <QTime>";
				else if (col.cppType == "QByteArray") extraIncludes << "#include <QByteArray>";
				else if (col.cppType == "qlonglong" || col.cppType == "qint64") extraIncludes << "#include <QtGlobal>";
			}
			for (const QString& inc : extraIncludes)
				out << inc << "\n";
			out << "\n";

			out << "namespace nexusdl::database {\n\n";

			// 类定义
			out << "class " << tableDef.className
				<< " : public BaseEntity<" << tableDef.className << ">\n";
			out << "{\n";
			out << "public:\n";
			out << "    " << tableDef.className << "() = default;\n\n";

			// 成员变量
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				out << "    " << col.cppType << " " << member << ";";
				if (!col.comment.isEmpty())
					out << " // " << col.comment;
				out << "\n";
			}
			out << "\n";

			// ---------- 为每个字段生成静态 Field 对象 ----------
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				QString fieldName = member + "_field";   // 例如 id_field, name_field
				out << "    inline static constexpr Field<" << tableDef.className << ", " << col.cppType
					<< "> " << fieldName << "{ &" << tableDef.className << "::" << member
					<< ", \"" << col.name << "\" };\n";
			}
			out << "\n";

			// 静态方法
			out << "    static QString tableName() { return \"" << tableDef.tableName << "\"; }\n";

			QString pkName;
			for (const auto& col : tableDef.columns) {
				if (col.isPrimaryKey) {
					pkName = col.name;
					break;
				}
			}
			if (pkName.isEmpty()) pkName = "id"; // fallback
			out << "    static QString primaryKey() { return \"" << pkName << "\"; }\n";

			QString dbIdStr = (dbId == DatabaseId::User)
				? "DatabaseId::User" : "DatabaseId::Download";
			out << "    static DatabaseId databaseId() { return " << dbIdStr << "; }\n\n";

			// fromRecord
			out << "    static " << tableDef.className << " fromRecord(const QSqlRecord& record)\n";
			out << "    {\n";
			out << "        " << tableDef.className << " entity;\n";
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				out << "        entity." << member << " = record.value(\"" << col.name << "\")";
				if (col.cppType == "QDateTime")      out << ".toDateTime()";
				else if (col.cppType == "QDate")     out << ".toDate()";
				else if (col.cppType == "QTime")     out << ".toTime()";
				else if (col.cppType == "int" ||
					col.cppType == "short" ||
					col.cppType == "quint8")    out << ".toInt()";
				else if (col.cppType == "bool")      out << ".toBool()";
				else if (col.cppType == "float" ||
					col.cppType == "double")    out << ".toDouble()";
				else if (col.cppType == "qlonglong" ||
					col.cppType == "qint64")    out << ".toLongLong()";
				else if (col.cppType == "QByteArray") out << ".toByteArray()";
				else                                   out << ".toString()";
				out << ";\n";
			}
			out << "        return entity;\n";
			out << "    }\n\n";

			// toMap override
			out << "    QVariantMap toMap() const override\n";
			out << "    {\n";
			out << "        QVariantMap map;\n";
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				out << "        map[\"" << col.name << "\"] = " << member << ";\n";
			}
			out << "        return map;\n";
			out << "    }\n\n";

			// isValid 辅助函数（基于主键）
			if (!pkName.isEmpty()) {
				QString pkMember = toCamelCase(pkName, false);
				QString pkType;
				for (const auto& col : tableDef.columns) {
					if (col.name == pkName) {
						pkType = col.cppType;
						break;
					}
				}
				out << "    bool isValid() const { ";
				if (pkType == "QString" || pkType == "QByteArray")
					out << "return !" << pkMember << ".isEmpty(); }\n";
				else if (pkType == "QDateTime" || pkType == "QDate" || pkType == "QTime")
					out << "return " << pkMember << ".isValid(); }\n";
				else if (pkType == "int" || pkType == "short" || pkType == "qlonglong")
					out << "return " << pkMember << " != 0; }\n";
				else
					out << "return true; }\n";
			}
			else {
				out << "    bool isValid() const { return false; }\n";
			}

			out << "};\n\n";
			out << "} // namespace nexusdl::database\n";
			headerFile.close();
			qDebug() << "Generated entity class:" << headerPath;
			return true;
		}

		// ---------- DAO 头文件 ----------
		static bool generateDaoHeader(const TableDefinition& tableDef,
			const QString& outputDir,
			bool overwrite)
		{
			QDir dir(outputDir);
			QString headerPath = dir.filePath(tableDef.className + "DAO.h");
			if (QFile::exists(headerPath) && !overwrite) {
				qWarning() << "DAO header already exists:" << headerPath;
				return false;
			}

			QFile headerFile(headerPath);
			if (!headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				qCritical() << "Failed to create DAO header:" << headerPath;
				return false;
			}

			QTextStream out(&headerFile);
			out.setEncoding(QStringConverter::Utf8);

			// 版权声明
			out << "// Copyright 2026 XZHAO_O. All rights reserved.\n";
			out << "// SPDX-License-Identifier: MIT\n\n";

			out << "#pragma once\n\n";
			out << "#include \"" << tableDef.className << ".h\"\n";
			out << "#include \"base_dao.h\"\n";
			out << "#include <QList>\n\n";
			out << "namespace nexusdl::database {\n\n";
			out << "class DatabaseManager;\n";
			out << "template<typename> class QueryWrapper;\n\n";

			out << "class " << tableDef.className << "DAO : public BaseDAO<"
				<< tableDef.className << "DAO, " << tableDef.className << ">\n";
			out << "{\n";
			out << "public:\n";
			out << "    static " << tableDef.className << "DAO& instance();\n\n";

			out << "    // 表结构管理\n";
			out << "    bool createTable();\n";
			out << "    bool dropTable();\n\n";

			out << "    // 批量操作\n";
			out << "    bool insertBatch(const QList<" << tableDef.className << ">& entities);\n\n";

			out << "    // 分页查询\n";
			out << "    QList<" << tableDef.className << "> page(const QueryWrapper<"
				<< tableDef.className << ">& wrapper, int pageNum, int pageSize);\n\n";

			out << "    // 条件删除（通过 QueryWrapper）\n";
			out << "    bool remove(const QueryWrapper<" << tableDef.className << ">& wrapper);\n\n";

			out << "private:\n";
			out << "    explicit " << tableDef.className << "DAO(SQLiteDatabase& db);\n";
			out << "    ~" << tableDef.className << "DAO() = default;\n";
			out << "    " << tableDef.className << "DAO(const " << tableDef.className << "DAO&) = delete;\n";
			out << "    " << tableDef.className << "DAO& operator=(const " << tableDef.className << "DAO&) = delete;\n";
			out << "};\n\n";
			out << "} // namespace nexusdl::database\n";
			headerFile.close();
			qDebug() << "Generated DAO header:" << headerPath;
			return true;
		}

		// ---------- DAO 实现文件 ----------
		static bool generateDaoImplementation(const TableDefinition& tableDef,
			const QString& outputDir,
			bool overwrite)
		{
			QDir dir(outputDir);
			QString cppPath = dir.filePath(tableDef.className + "DAO.cpp");
			if (QFile::exists(cppPath) && !overwrite) {
				qWarning() << "DAO implementation already exists:" << cppPath;
				return false;
			}

			QFile cppFile(cppPath);
			if (!cppFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				qCritical() << "Failed to create DAO implementation:" << cppPath;
				return false;
			}

			QTextStream out(&cppFile);
			out.setEncoding(QStringConverter::Utf8);

			// 版权声明
			out << "// Copyright 2026 XZHAO_O. All rights reserved.\n";
			out << "// SPDX-License-Identifier: MIT\n\n";

			out << "#include \"" << tableDef.className << "DAO.h\"\n";
			out << "#include \"database_manager.h\"\n";
			out << "#include \"query_wrapper.h\"\n\n";

			// 命名空间开始
			out << "namespace nexusdl::database {\n\n";

			// 匿名命名空间存放 SQL 常量
			out << "namespace {\n";
			out << "    const QString CREATE_TABLE_SQL = \n";
			out << "        \"CREATE TABLE IF NOT EXISTS " << tableDef.tableName << " (\\n\"\n";
			for (int i = 0; i < tableDef.columns.size(); ++i) {
				const auto& col = tableDef.columns.at(i);
				out << "        \"    " << col.name << " " << col.sqlType;
				if (col.isNotNull)      out << " NOT NULL";
				if (col.isPrimaryKey)   out << " PRIMARY KEY";
				if (!col.defaultValue.isEmpty())
					out << " DEFAULT " << col.defaultValue;
				if (i < tableDef.columns.size() - 1) {
					out << ",\\n\"\n";
				}
				else {
					out << "\\n)\";\n\n";
				}
			}
			out << "    const QString DROP_TABLE_SQL = \"DROP TABLE IF EXISTS "
				<< tableDef.tableName << "\";\n";
			out << "}\n\n";

			// instance()
			out << tableDef.className << "DAO& " << tableDef.className << "DAO::instance()\n";
			out << "{\n";
			out << "    static " << tableDef.className << "DAO instance(\n";
			out << "        DatabaseManager::instance().database(" << tableDef.className << "::databaseId()));\n";
			out << "    return instance;\n";
			out << "}\n\n";

			// 构造函数
			out << tableDef.className << "DAO::" << tableDef.className
				<< "DAO(SQLiteDatabase& db)\n";
			out << "    : BaseDAO(db)\n";
			out << "{\n";
			out << "}\n\n";

			// createTable
			out << "bool " << tableDef.className << "DAO::createTable()\n";
			out << "{\n";
			out << "    return m_db.executeWrite(CREATE_TABLE_SQL).has_value();\n";
			out << "}\n\n";

			// dropTable
			out << "bool " << tableDef.className << "DAO::dropTable()\n";
			out << "{\n";
			out << "    return m_db.executeWrite(DROP_TABLE_SQL).has_value();\n";
			out << "}\n\n";

			// insertBatch
			out << "bool " << tableDef.className << "DAO::insertBatch(const QList<"
				<< tableDef.className << ">& entities)\n";
			out << "{\n";
			out << "    if (entities.isEmpty()) return true;\n";
			out << "    if (!m_db.beginTransaction()) return false;\n";
			out << "    for (const auto& entity : entities) {\n";
			out << "        if (!insert(entity)) {\n";
			out << "            m_db.rollbackTransaction();\n";
			out << "            return false;\n";
			out << "        }\n";
			out << "    }\n";
			out << "    return m_db.commitTransaction().has_value();\n";
			out << "}\n\n";

			// page
			out << "QList<" << tableDef.className << "> " << tableDef.className
				<< "DAO::page(const QueryWrapper<" << tableDef.className << ">& wrapper, int pageNum, int pageSize)\n";
			out << "{\n";
			out << "    QueryWrapper<" << tableDef.className << "> pageWrapper = wrapper;\n";
			out << "    pageWrapper.limit((pageNum - 1) * pageSize, pageSize);\n";
			out << "    auto result = selectList(pageWrapper);\n";
			out << "    if (result) return *result;\n";
			out << "    return {};\n";
			out << "}\n\n";

			// remove (通过 QueryWrapper)
			out << "bool " << tableDef.className << "DAO::remove(const QueryWrapper<"
				<< tableDef.className << ">& wrapper)\n";
			out << "{\n";
			out << "    return deleteByWrapper(wrapper).has_value();\n";
			out << "}\n";

			// 命名空间结束
			out << "\n} // namespace nexusdl::database\n";

			cppFile.close();
			qDebug() << "Generated DAO implementation:" << cppPath;
			return true;
		}

		// ---------- Service 头文件 ----------
		static bool generateServiceHeader(const TableDefinition& tableDef,
			const QString& outputDir,
			bool overwrite)
		{
			QDir dir(outputDir);
			QString headerPath = dir.filePath(tableDef.className + "Service.h");
			if (QFile::exists(headerPath) && !overwrite) {
				qWarning() << "Service header already exists:" << headerPath;
				return false;
			}

			QFile headerFile(headerPath);
			if (!headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				qCritical() << "Failed to create Service header:" << headerPath;
				return false;
			}

			QTextStream out(&headerFile);
			out.setEncoding(QStringConverter::Utf8);

			// 版权声明
			out << "// Copyright 2026 XZHAO_O. All rights reserved.\n";
			out << "// SPDX-License-Identifier: MIT\n\n";

			out << "#pragma once\n\n";
			out << "#include \"" << tableDef.className << ".h\"\n";
			out << "#include \"base_service.h\"\n";
			out << "#include \"" << tableDef.className << "DAO.h\"\n";
			out << "#include <QList>\n\n";

			out << "namespace nexusdl::database {\n\n";
			out << "class " << tableDef.className << "Req;\n\n";

			out << "class " << tableDef.className << "Service : public BaseService<"
				<< tableDef.className << ", " << tableDef.className << "DAO>\n";
			out << "{\n";
			out << "public:\n";
			out << "    static " << tableDef.className << "Service& instance();\n\n";

			out << "    // 从请求对象转换\n";
			out << "    " << tableDef.className << " generateFromReq(const " << tableDef.className << "Req& req);\n\n";

			out << "    // 批量插入\n";
			out << "    bool insert(const QList<" << tableDef.className << ">& entities);\n";
			out << "    bool insert(const " << tableDef.className << "Req& req);\n";
			out << "    bool insert(const QList<" << tableDef.className << "Req>& reqs);\n\n";

			out << "    // 条件删除（基于请求对象）\n";
			out << "    bool remove(const " << tableDef.className << "Req& req);\n\n";

			out << "    // 条件查询\n";
			out << "    QList<" << tableDef.className << "> search(const " << tableDef.className << "Req& req);\n";
			out << "    int count(const " << tableDef.className << "Req& req);\n\n";

			out << "    // JSON 导入导出\n";
			out << "    int importFromJson(const QString& filePath);\n";
			out << "    bool exportToJson(const QString& filePath) const;\n\n";

			out << "private:\n";
			out << "    " << tableDef.className << "Service() = default;\n";
			out << "    ~" << tableDef.className << "Service() = default;\n";
			out << "    " << tableDef.className << "Service(const " << tableDef.className << "Service&) = delete;\n";
			out << "    " << tableDef.className << "Service& operator=(const " << tableDef.className << "Service&) = delete;\n";
			out << "};\n\n";
			out << "} // namespace nexusdl::database\n";

			headerFile.close();
			qDebug() << "Generated Service header:" << headerPath;
			return true;
		}

		// ---------- Service 实现文件 ----------
		static bool generateServiceImplementation(const TableDefinition& tableDef,
			const QString& outputDir,
			bool overwrite)
		{
			QDir dir(outputDir);
			QString cppPath = dir.filePath(tableDef.className + "Service.cpp");
			if (QFile::exists(cppPath) && !overwrite) {
				qWarning() << "Service implementation already exists:" << cppPath;
				return false;
			}

			QFile cppFile(cppPath);
			if (!cppFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
				qCritical() << "Failed to create Service implementation:" << cppPath;
				return false;
			}

			QTextStream out(&cppFile);
			out.setEncoding(QStringConverter::Utf8);

			// 版权声明
			out << "// Copyright 2026 XZHAO_O. All rights reserved.\n";
			out << "// SPDX-License-Identifier: MIT\n\n";

			out << "#include \"" << tableDef.className << "Service.h\"\n";
			out << "#include \"" << tableDef.className << "Req.h\"\n";
			out << "#include \"query_wrapper.h\"\n";
			out << "#include <QFile>\n";
			out << "#include <QJsonDocument>\n";
			out << "#include <QJsonArray>\n";
			out << "#include <QJsonObject>\n\n";

			// 命名空间开始
			out << "namespace nexusdl::database {\n\n";

			// instance()
			out << tableDef.className << "Service& " << tableDef.className << "Service::instance()\n";
			out << "{\n";
			out << "    static " << tableDef.className << "Service service;\n";
			out << "    return service;\n";
			out << "}\n\n";

			// generateFromReq
			out << tableDef.className << " " << tableDef.className
				<< "Service::generateFromReq(const " << tableDef.className << "Req& req)\n";
			out << "{\n";
			out << "    " << tableDef.className << " entity;\n";
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				out << "    entity." << member << " = req." << member << ";\n";
			}
			out << "    return entity;\n";
			out << "}\n\n";

			// insert (QList<Entity>)
			out << "bool " << tableDef.className << "Service::insert(const QList<"
				<< tableDef.className << ">& entities)\n";
			out << "{\n";
			out << "    return m_dao.insertBatch(entities);\n";
			out << "}\n\n";

			// insert (Req)
			out << "bool " << tableDef.className << "Service::insert(const "
				<< tableDef.className << "Req& req)\n";
			out << "{\n";
			out << "    return insert(generateFromReq(req));\n";
			out << "}\n\n";

			// insert (QList<Req>)
			out << "bool " << tableDef.className << "Service::insert(const QList<"
				<< tableDef.className << "Req>& reqs)\n";
			out << "{\n";
			out << "    QList<" << tableDef.className << "> entities;\n";
			out << "    entities.reserve(reqs.size());\n";
			out << "    for (const auto& req : reqs)\n";
			out << "        entities.append(generateFromReq(req));\n";
			out << "    return insert(entities);\n";
			out << "}\n\n";

			// remove (通过 Req) —— 示例：构造空 wrapper，用户需根据实际需求添加条件
			out << "bool " << tableDef.className << "Service::remove(const "
				<< tableDef.className << "Req& req)\n";
			out << "{\n";
			out << "    QueryWrapper<" << tableDef.className << "> wrapper;\n";
			out << "    // TODO: 根据 req 的字段添加条件，例如\n";
			out << "    // if (!req.id.isEmpty()) wrapper.eq(\"id\", req.id);\n";
			out << "    return m_dao.remove(wrapper);\n";
			out << "}\n\n";

			// search
			out << "QList<" << tableDef.className << "> " << tableDef.className
				<< "Service::search(const " << tableDef.className << "Req& req)\n";
			out << "{\n";
			out << "    QueryWrapper<" << tableDef.className << "> wrapper;\n";
			out << "    // TODO: 根据 req 的字段添加查询条件\n";
			out << "    auto result = m_dao.selectList(wrapper);\n";
			out << "    if (result) return *result;\n";
			out << "    return {};\n";
			out << "}\n\n";

			// count
			out << "int " << tableDef.className << "Service::count(const "
				<< tableDef.className << "Req& req)\n";
			out << "{\n";
			out << "    QueryWrapper<" << tableDef.className << "> wrapper;\n";
			out << "    // TODO: 根据 req 的字段添加条件\n";
			out << "    auto result = m_dao.selectCount(wrapper);\n";
			out << "    if (result) return *result;\n";
			out << "    return 0;\n";
			out << "}\n\n";

			// importFromJson
			out << "int " << tableDef.className << "Service::importFromJson(const QString& filePath)\n";
			out << "{\n";
			out << "    QFile file(filePath);\n";
			out << "    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {\n";
			out << "        qWarning() << \"Failed to open JSON file:\" << filePath;\n";
			out << "        return 0;\n";
			out << "    }\n\n";
			out << "    QByteArray jsonData = file.readAll();\n";
			out << "    file.close();\n\n";
			out << "    QJsonDocument doc = QJsonDocument::fromJson(jsonData);\n";
			out << "    if (doc.isNull() || !doc.isArray()) {\n";
			out << "        qWarning() << \"Invalid JSON format\";\n";
			out << "        return 0;\n";
			out << "    }\n\n";
			out << "    QJsonArray jsonArray = doc.array();\n";
			out << "    QList<" << tableDef.className << "> entities;\n";
			out << "    entities.reserve(jsonArray.size());\n\n";
			out << "    for (const QJsonValue& val : jsonArray) {\n";
			out << "        QJsonObject obj = val.toObject();\n";
			out << "        " << tableDef.className << " entity;\n";
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				QString key = col.name;
				if (col.cppType == "QDateTime") {
					out << "        entity." << member << " = QDateTime::fromString(obj[\""
						<< key << "\"].toString(), Qt::ISODate);\n";
				}
				else if (col.cppType == "QDate") {
					out << "        entity." << member << " = QDate::fromString(obj[\""
						<< key << "\"].toString(), Qt::ISODate);\n";
				}
				else if (col.cppType == "QTime") {
					out << "        entity." << member << " = QTime::fromString(obj[\""
						<< key << "\"].toString(), Qt::ISODate);\n";
				}
				else if (col.cppType == "QByteArray") {
					out << "        entity." << member << " = QByteArray::fromBase64(obj[\""
						<< key << "\"].toString().toLatin1());\n";
				}
				else if (col.cppType == "int" || col.cppType == "short" ||
					col.cppType == "quint8") {
					out << "        entity." << member << " = obj[\"" << key << "\"].toInt();\n";
				}
				else if (col.cppType == "bool") {
					out << "        entity." << member << " = obj[\"" << key << "\"].toBool();\n";
				}
				else if (col.cppType == "float" || col.cppType == "double") {
					out << "        entity." << member << " = obj[\"" << key << "\"].toDouble();\n";
				}
				else if (col.cppType == "qlonglong" || col.cppType == "qint64") {
					out << "        entity." << member << " = obj[\"" << key << "\"].toVariant().toLongLong();\n";
				}
				else {
					out << "        entity." << member << " = obj[\"" << key << "\"].toString();\n";
				}
			}
			out << "        entities.append(entity);\n";
			out << "    }\n\n";
			out << "    if (entities.isEmpty()) return 0;\n";
			out << "    return insert(entities) ? entities.size() : 0;\n";
			out << "}\n\n";

			// exportToJson
			out << "bool " << tableDef.className << "Service::exportToJson(const QString& filePath) const\n";
			out << "{\n";
			out << "    QList<" << tableDef.className << "> entities = search("
				<< tableDef.className << "Req());\n";
			out << "    if (entities.isEmpty()) {\n";
			out << "        qWarning() << \"No records to export\";\n";
			out << "        return false;\n";
			out << "    }\n\n";
			out << "    QJsonArray jsonArray;\n";
			out << "    for (const auto& entity : entities) {\n";
			out << "        QJsonObject obj;\n";
			for (const auto& col : tableDef.columns) {
				QString member = toCamelCase(col.name, false);
				QString key = col.name;
				if (col.cppType == "QDateTime") {
					out << "        obj[\"" << key << "\"] = entity." << member << ".toString(Qt::ISODate);\n";
				}
				else if (col.cppType == "QDate") {
					out << "        obj[\"" << key << "\"] = entity." << member << ".toString(Qt::ISODate);\n";
				}
				else if (col.cppType == "QTime") {
					out << "        obj[\"" << key << "\"] = entity." << member << ".toString(Qt::ISODate);\n";
				}
				else if (col.cppType == "QByteArray") {
					out << "        obj[\"" << key << "\"] = QString(entity." << member << ".toBase64());\n";
				}
				else {
					out << "        obj[\"" << key << "\"] = entity." << member << ";\n";
				}
			}
			out << "        jsonArray.append(obj);\n";
			out << "    }\n\n";
			out << "    QJsonDocument doc(jsonArray);\n";
			out << "    QFile file(filePath);\n";
			out << "    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {\n";
			out << "        qWarning() << \"Failed to open file for writing:\" << filePath;\n";
			out << "        return false;\n";
			out << "    }\n";
			out << "    file.write(doc.toJson());\n";
			out << "    file.close();\n";
			out << "    return true;\n";
			out << "}\n";

			// 命名空间结束
			out << "\n} // namespace nexusdl::database\n";

			cppFile.close();
			qDebug() << "Generated Service implementation:" << cppPath;
			return true;
		}
	};

}