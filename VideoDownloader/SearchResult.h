#pragma once

#include <QString>
#include <QList>
#include <QUrl>
#include "VideoInfo.h"

// 搜索条目
struct SearchItem {
	VideoInfo videoInfo;
	QString snippet;
	QString channelTitle;
	QDateTime publishDate;

	bool isValid() const {
		return videoInfo.isValid();
	}
};

// 搜索结果
struct SearchResult {
	QList<SearchItem> items;
	int totalResults;
	int resultsPerPage;
	QString nextPageToken;
	QString prevPageToken;
	QString searchQuery;
	QString platformId;

	bool isEmpty() const {
		return items.isEmpty();
	}

	int count() const {
		return items.size();
	}

	SearchItem at(int index) const {
		return items.value(index);
	}
};

Q_DECLARE_METATYPE(SearchItem)
Q_DECLARE_METATYPE(SearchResult)