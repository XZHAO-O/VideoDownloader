#include "LanguageManager.h"

LanguageManager* LanguageManager::s_instance = nullptr;

LanguageManager::LanguageManager(QObject* parent)
	: QObject(parent)
	, m_appTranslator(nullptr)
	, m_currentLanguage(SystemDefault)
	, m_settings(nullptr)
{
	m_appTranslator = new QTranslator(this);
	m_settings = new QSettings("MyCompany", "MultiLanguageApp", this);
}

LanguageManager::~LanguageManager()
{
}

LanguageManager* LanguageManager::instance()
{
	if (!s_instance) {
		s_instance = new LanguageManager();
	}
	return s_instance;
}

void LanguageManager::destroyInstance()
{
	if (s_instance) {
		delete s_instance;
		s_instance = nullptr;
	}
}

void LanguageManager::initialize()
{
	loadSettings();
	switchLanguage(m_currentLanguage);
}

bool LanguageManager::switchLanguage(Language language)
{
	// 移除现有的翻译器
	QApplication::removeTranslator(m_appTranslator);

	if (language == SystemDefault) {
		m_currentLanguage = SystemDefault;
	}
	else {
		QString filePath = languageFilePath(language);

		if (m_appTranslator->load(filePath)) {
			QApplication::installTranslator(m_appTranslator);
			m_currentLanguage = language;
			qDebug() << "Switched to language:" << languageName(language);
		}
		else {
			qWarning() << "Failed to load translation file:" << filePath;
			return false;
		}
	}

	saveSettings();
	emit languageChanged();
	return true;
}

LanguageManager::Language LanguageManager::currentLanguage() const
{
	return m_currentLanguage;
}

QString LanguageManager::languageName(Language language) const
{
	switch (language) {
	case English: return "English";
	case ChineseSimplified: return "简体中文";
	case Japanese: return "日本語";
	case SystemDefault: return "System Default";
	default: return "Unknown";
	}
}

QList<LanguageManager::Language> LanguageManager::availableLanguages() const
{
	return { SystemDefault, English, ChineseSimplified, Japanese };
}

void LanguageManager::loadSettings()
{
	int lang = m_settings->value("Language", SystemDefault).toInt();
	m_currentLanguage = static_cast<Language>(lang);
}

void LanguageManager::saveSettings()
{
	m_settings->setValue("Language", static_cast<int>(m_currentLanguage));
}

QString LanguageManager::languageCode(Language language) const
{
	switch (language) {
	case English: return "en_US";
	case ChineseSimplified: return "zh_CN";
	case Japanese: return "ja_JP";
	default: return "";
	}
}

QString LanguageManager::languageFilePath(Language language) const
{
	QString code = languageCode(language);
	if (code.isEmpty()) return "";

	// 尝试多种路径
	QStringList possiblePaths;

	// 1. 当前工作目录的translations文件夹
	possiblePaths << QDir::currentPath() + "/translations/app_" + code + ".qm";

	// 2. 应用程序目录的translations文件夹
	possiblePaths << QApplication::applicationDirPath() + "/translations/app_" + code + ".qm";

	// 3. 资源文件
	possiblePaths << ":/translations/app_" + code + ".qm";

	// 4. VS调试输出目录
	#ifdef _DEBUG
	possiblePaths << QApplication::applicationDirPath() + "/../../translations/app_" + code + ".qm";
	#else
	possiblePaths << QApplication::applicationDirPath() + "/../translations/app_" + code + ".qm";
	#endif

	for (const QString& path : possiblePaths) {
		if (QFile::exists(path)) {
			qDebug() << "Found translation file:" << path;
			return path;
		}
	}

	qWarning() << "No translation file found for language:" << code;
	return "";
}