#pragma once

#include <QObject>
#include <QTranslator>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QDebug>

class LanguageManager : public QObject
{
	Q_OBJECT

public:
	enum Language {
		English,
		ChineseSimplified,
		Japanese,
		SystemDefault
	};
	Q_ENUM(Language);

	static LanguageManager* instance();
	static void destroyInstance();

	void initialize();
	bool switchLanguage(Language language);
	Language currentLanguage() const;
	QString languageName(Language language) const;
	QList<Language> availableLanguages() const;

signals:
	void languageChanged();

private:
	explicit LanguageManager(QObject* parent = nullptr);
	~LanguageManager();

	void loadSettings();
	void saveSettings();
	QString languageCode(Language language) const;
	QString languageFilePath(Language language) const;

	QTranslator* m_appTranslator;
	Language m_currentLanguage;
	QSettings* m_settings;

	static LanguageManager* s_instance;
};