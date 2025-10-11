#pragma once

#include <QWidget>
#include <QSharedPointer>
#include "FramelessVideoWindow.h"
#include "AntInput.h"
#include "ApplicationController.h"
#include "DownloadManager.h"
#include "SearchResultsWidget.h"  // 新增

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
	void onNextButtonClicked();  // 新增

private:
	void setupUI();
	void setupConnections();
	void updateSearchResultsPosition();
	void loadMockSearchData();

	FramelessVideoWindow* videoWindow = nullptr;
	AntInput* antInput = nullptr;
	SearchResultsWidget* m_searchResultsWidget = nullptr;  // 替换为组件

	QSharedPointer<ApplicationController> m_appController;

signals:
	void navigateToDownloadRequested();
};