#pragma once

#include <QWidget>
#include <QDateTime>
#include <QUrl>

#include "VideoInfo.h"

class QCheckBox;
class QStandardItemModel;
class QLabel;
class AntButton;
class AntChatListView;

class SearchResultsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SearchResultsWidget(QWidget* parent = nullptr);
	~SearchResultsWidget();

	void addSearchResultItem(const QString& title, const QString& duration, const QString& author);
	void addSearchResultItems(const QList<VideoInfo>& videoInfos); // 新增批量添加函数
	void clearAll();
	const QList<int>& getSelectedIndexes() const { return m_selectedIndexes; }
	int getTotalItems() const;
	int getSelectedCount() const;

	void updateStyle();

public slots:
	void setVisible(bool visible) override;

signals:
	void nextButtonClicked();
	void selectionChanged();

private slots:
	void onSelectAllStateChanged(int state);
	void onNextButtonClicked();
	void handleSelectAllClick();
	void onThemeChanged();

private:
	void setupUI();
	void setupConnections();
	void updateSelectedCount();
	void updateSelectAllCheckboxState();
	bool eventFilter(QObject* obj, QEvent* event) override;

	QLabel* m_selectedCountLabel = nullptr;
	AntChatListView* m_searchResultsList = nullptr;
	QStandardItemModel* m_listModel = nullptr;
	QCheckBox* m_selectAllCheckBox = nullptr;
	AntButton* m_nextButton = nullptr;

	QList<int> m_selectedIndexes;
	int m_totalItems = 0;
};