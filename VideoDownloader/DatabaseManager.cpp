#include "DatabaseManager.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDateTime>
#include <QDebug>

DatabaseManager::DatabaseManager(QObject* parent)
	: QObject(parent)
	, m_isInitialized(false)
{
}

DatabaseManager::~DatabaseManager()
{
	if (m_database.isOpen()) {
		m_database.close();
	}
}

bool DatabaseManager::initialize(const QString& databasePath)
{
	QMutexLocker locker(&m_mutex);

	if (m_isInitialized) {
		return true;
	}

	// 检查并创建数据库目录
	QFileInfo fileInfo(databasePath);
	QString dirPath = fileInfo.absolutePath();
	QDir dir(dirPath);

	if (!dir.exists()) {
		if (!dir.mkpath(".")) {
			qCritical() << "Failed to create database directory:" << dirPath;
			return false;
		}
	}

	// 设置数据库路径
	m_databasePath = databasePath;

	// 初始化数据库连接
	if (!initConnection(databasePath)) {
		return false;
	}

	m_isInitialized = true;
	qDebug() << "Database initialized successfully at:" << m_databasePath;
	return true;
}

bool DatabaseManager::initConnection(const QString& databasePath)
{
	// 检查是否需要创建数据库文件
	bool dbExists = QFile::exists(databasePath);

	m_database = QSqlDatabase::addDatabase("QSQLITE", "VideoDownloaderConnection");
	m_database.setDatabaseName(databasePath);

	if (!m_database.open()) {
		qCritical() << "Failed to open database:" << m_database.lastError().text();
		return false;
	}

	// 如果是新创建的数据库，设置SQLite参数
	if (!dbExists) {
		QSqlQuery query(m_database);
		query.exec("PRAGMA foreign_keys = ON");
		query.exec("PRAGMA journal_mode = WAL");
		query.exec("PRAGMA synchronous = NORMAL");
		query.exec("PRAGMA cache_size = -64000");
		query.exec("PRAGMA busy_timeout = 5000");
	}

	return true;
}

bool DatabaseManager::createTable(const QString& tableName, const QString& tableDefinition)
{
	QMutexLocker locker(&m_mutex);

	if (!m_database.isOpen()) {
		qCritical() << "Database is not open";
		return false;
	}

	QString createTableSQL = QString("CREATE TABLE IF NOT EXISTS %1 (%2)")
		.arg(tableName)
		.arg(tableDefinition);

	QSqlQuery query(m_database);
	if (!query.exec(createTableSQL)) {
		qCritical() << "Failed to create table:" << query.lastError().text();
		return false;
	}

	return true;
}

bool DatabaseManager::executeQuery(const QString& queryStr, const QVariantMap& params)
{
	QMutexLocker locker(&m_mutex);

	if (!m_database.isOpen()) {
		qCritical() << "Database is not open";
		return false;
	}

	QSqlQuery query(m_database);
	query.prepare(queryStr);

	for (auto it = params.constBegin(); it != params.constEnd(); ++it)
	{
		query.bindValue(it.key(), it.value());
	}

	if (!query.exec()) {
		qCritical() << "Failed to execute query:" << query.lastError().text();
		return false;
	}

	return true;
}

bool DatabaseManager::executeQuery(const QString& queryStr, const QVariantList& params)
{
	QMutexLocker locker(&m_mutex);

	if (!m_database.isOpen()) {
		qCritical() << "Database is not open";
		return false;
	}

	QSqlQuery query(m_database);
	query.prepare(queryStr);

	for (int i = 0; i < params.size(); ++i) {
		query.bindValue(i, params[i]);
	}

	if (!query.exec()) {
		qCritical() << "Failed to execute query:" << query.lastError().text();
		return false;
	}

	return true;
}

QList<QVariantMap> DatabaseManager::executeQueryToMap(const QString& queryStr, const QVariantMap& params)
{
	QMutexLocker locker(&m_mutex);

	QList<QVariantMap> result;

	if (!m_database.isOpen())
	{
		qCritical() << "Database is not open";
		return result;
	}

	QSqlQuery query(m_database);
	query.prepare(queryStr);

	for (auto it = params.constBegin(); it != params.constEnd(); ++it)
	{
		query.bindValue(it.key(), it.value());
	}

	if (!query.exec())
	{
		qCritical() << "Failed to execute select:" << query.lastError().text();
		return result;
	}

	while (query.next())
	{
		QVariantMap row;
		QSqlRecord record = query.record();

		for (int i = 0; i < record.count(); ++i)
		{
			row[record.fieldName(i)] = query.value(i);
		}

		result.append(row);
	}

	return result;
}

QList<QVariantMap> DatabaseManager::executeQueryToMap(const QString& queryStr,
	const QVariantList& params)
{
	QMutexLocker locker(&m_mutex);

	QList<QVariantMap> result;

	if (!m_database.isOpen())
	{
		qCritical() << "Database is not open";
		return result;
	}

	QSqlQuery query(m_database);
	query.prepare(queryStr);

	for (int i = 0; i < params.size(); ++i)
	{
		query.bindValue(i, params[i]);
	}

	if (!query.exec())
	{
		qCritical() << "Failed to execute select:" << query.lastError().text();
		return result;
	}

	while (query.next())
	{
		QVariantMap row;
		QSqlRecord record = query.record();

		for (int i = 0; i < record.count(); ++i)
		{
			row[record.fieldName(i)] = query.value(i);
		}

		result.append(row);
	}

	return result;
}

QString DatabaseManager::lastError() const
{
	QMutexLocker locker(&m_mutex);
	return m_database.lastError().text();
}

QString DatabaseManager::databasePath() const
{
	return m_databasePath;
}

qint64 DatabaseManager::databaseSize() const
{
	QFileInfo fileInfo(m_databasePath);
	return fileInfo.exists() ? fileInfo.size() : -1;
}

bool DatabaseManager::beginTransaction()
{
	QMutexLocker locker(&m_mutex);
	return m_database.transaction();
}

bool DatabaseManager::commitTransaction()
{
	QMutexLocker locker(&m_mutex);
	return m_database.commit();
}

bool DatabaseManager::rollbackTransaction()
{
	QMutexLocker locker(&m_mutex);
	return m_database.rollback();
}

bool DatabaseManager::backupDatabase(const QString& backupPath)
{
	QMutexLocker locker(&m_mutex);

	if (!m_database.isOpen()) {
		qCritical() << "Database is not open";
		return false;
	}

	// 使用SQLite的VACUUM INTO进行备份
	QSqlQuery query(m_database);
	QString backupQuery = QString("VACUUM INTO '%1'").arg(backupPath);

	if (!query.exec(backupQuery)) {
		qCritical() << "Failed to backup database:" << query.lastError().text();
		return false;
	}

	return true;
}

bool DatabaseManager::restoreDatabase(const QString& backupPath)
{
	QMutexLocker locker(&m_mutex);

	// 关闭当前数据库连接
	if (m_database.isOpen()) {
		m_database.close();
	}

	// 检查备份文件是否存在
	if (!QFile::exists(backupPath)) {
		qCritical() << "Backup file does not exist:" << backupPath;
		return false;
	}

	// 备份当前数据库（如果存在）
	if (QFile::exists(m_databasePath)) {
		QString backupName = QString("%1.backup_%2")
			.arg(m_databasePath)
			.arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
		if (!QFile::copy(m_databasePath, backupName)) {
			qWarning() << "Failed to backup current database before restore";
		}

		// 删除当前数据库
		if (!QFile::remove(m_databasePath)) {
			qCritical() << "Failed to remove current database";
			return false;
		}
	}

	// 复制备份文件
	if (!QFile::copy(backupPath, m_databasePath)) {
		qCritical() << "Failed to copy backup file";
		return false;
	}

	// 重新打开数据库
	return initConnection(m_databasePath);
}