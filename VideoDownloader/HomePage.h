#pragma once

#include <QWidget>
#include <QUrl>
#include <QDateTime>

#include "VideoInfo.h"

class AntInput;
class ApplicationController;
class ConfigModManager;
class SearchResultsWidget;

class HomePage : public QWidget
{
	Q_OBJECT

public:
	HomePage(QSharedPointer<ApplicationController> appController, QWidget* parent = nullptr);
	~HomePage();

protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void onSearchTextChanged(const QString& text);
	void onSearchClicked();
	void onNextButtonClicked();

private:
	void setupUI();
	void setupConnections();
	void updateSearchResultsPosition();
	void getVideoList(const QString& searchText);
	void loadMockSearchData();

	AntInput* antInput = nullptr;
	SearchResultsWidget* m_searchResultsWidget = nullptr;

	QSharedPointer<ApplicationController> m_appController;
	QSharedPointer<ConfigModManager> m_configModManager;

	QList<VideoInfo> videoInfoList;
	bool searchChanged = false;

signals:
	void navigateToDownloadRequested(QList<VideoInfo> selectedVideoInfoList);
};