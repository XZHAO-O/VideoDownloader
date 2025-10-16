#pragma once

#include <QWidget>
#include <QSharedPointer>
#include "FramelessVideoWindow.h"
#include "AntInput.h"
#include "ApplicationController.h"
#include "DownloadManager.h"
#include "SearchResultsWidget.h"
#include "ConfigModManager.h"

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

	FramelessVideoWindow* videoWindow = nullptr;
	AntInput* antInput = nullptr;
	SearchResultsWidget* m_searchResultsWidget = nullptr;

	QSharedPointer<ApplicationController> m_appController;
	QSharedPointer<ConfigModManager> m_configModManager;

	bool searchChanged = false;

signals:
	void navigateToDownloadRequested();
};