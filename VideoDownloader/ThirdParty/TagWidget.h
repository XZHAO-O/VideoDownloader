#pragma once

#include <QWidget>
#include <QSvgRenderer>
#include "DesignSystem.h"

class TagWidget : public QWidget
{
	Q_OBJECT

public:
	// 定义结构体用于存储每个标签的配置信息
	struct TagInfo
	{
		QString name;
		QString svgPath;
		QColor color;
	};

	TagWidget(const QString& text, qreal fontSize = 11.5, QColor bgColor = DesignSystem::instance()->primaryColor(),
		QWidget* parent = nullptr, bool hasIcon = false, const QString& svgPath = "", int iconSize = 18);

	QSize sizeHint() const override;
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	QString m_text;
	QSvgRenderer* m_svgRenderer;
	QColor m_bgColor;
	QColor m_textColor;

	int m_iconSize;
	qreal m_fontSize;
	bool m_hasIcon;
};
