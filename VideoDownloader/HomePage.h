#pragma once

#include <QWidget>
#include <QListWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSharedPointer>
#include "FramelessVideoWindow.h"
#include "AntInput.h"
#include "AntButton.h"
#include "ApplicationController.h"
#include "DownloadManager.h"

class HomePage : public QWidget
{
	Q_OBJECT

public:
	HomePage(QSharedPointer<ApplicationController> appController, QWidget* parent = nullptr);
	~HomePage();

protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override; // 添加事件过滤器

private slots:
	void onSearchTextChanged(const QString& text);
	void onSearchClicked();
	void onSelectAllStateChanged(int state);
	void onNextButtonClicked();
	void onItemCheckboxStateChanged(int state);

private:
	void setupUI();
	void setupConnections();
	void setupSearchResultsContainer();
	void updateSelectedCount();
	void addSearchResultItem(const QString& title, const QString& duration, const QString& author);
	void navigateToDownloadQueue();
	void updateSearchResultsPosition();
	void clearAllSelections();

	// 新增的方法声明
	void updateSearchResultsStyle();
	void updateItemStyle(QWidget* itemWidget);
	void blockItemSignals(bool block);
	void updateSelectAllCheckboxState();
	void handleSelectAllClick(); // 添加这个声明

	// 模拟搜索数据
	void loadMockSearchData();

	FramelessVideoWindow* videoWindow = nullptr;
	AntInput* antInput = nullptr;

	QSharedPointer<ApplicationController> m_appController;

	// 搜索结果容器
	QWidget* m_searchResultsContainer = nullptr;
	QLabel* m_selectedCountLabel = nullptr;
	QListWidget* m_searchResultsList = nullptr;
	QCheckBox* m_selectAllCheckBox = nullptr;
	AntButton* m_nextButton = nullptr;

	// 存储选中的视频信息
	QList<int> m_selectedIndexes;
	int m_totalItems = 0;

signals:
	void navigateToDownloadRequested();
};