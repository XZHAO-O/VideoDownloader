#pragma once

#include <QTableView>
#include <QPainter>
#include "DesignSystem.h"

class AntBaseTableView : public QTableView
{
	Q_OBJECT
public:
	explicit AntBaseTableView(QWidget* parent = nullptr);
	virtual ~AntBaseTableView() = default;

protected:
	void initStyle();  // 初始化样式
private:
};
