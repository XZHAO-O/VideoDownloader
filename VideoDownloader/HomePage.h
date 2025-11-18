#pragma once

#include <QWidget>
#include <QDateTime>
#include <QUrl>

#include "VideoInfo.h"

class AntInput;
class SearchResultsWidget;
class PlatformAggregatorService;
class ConfigModManager;

class HomePage : public QWidget
{
	Q_OBJECT

public:
	HomePage(QSharedPointer<PlatformAggregatorService> platformService, QSharedPointer<ConfigModManager> configModManager, QWidget* parent = nullptr);
	~HomePage();

signals:
	void navigateToDownloadRequested(QList<VideoInfo> selectedVideoInfoList);

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
	void availablePlatformsChanged();
	void updateSearchResultsPosition();
	void getVideoList(const QString& searchText);
	void processVideoList(const QList<VideoInfo>& videos);
	void clearSearchData();

	AntInput* antInput = nullptr;
	SearchResultsWidget* m_searchResultsWidget = nullptr;

	QSharedPointer<PlatformAggregatorService> m_platformService;
	QSharedPointer<ConfigVideoPlatform> m_selectedPlatform;
	QSharedPointer<ConfigModManager> m_configModManager;
	QStringList m_availablePlatforms;

	QList<VideoInfo> videoInfoList;

	bool searchChanged = false;
};