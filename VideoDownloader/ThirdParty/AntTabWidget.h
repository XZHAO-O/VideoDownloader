#pragma once

#include <QWidget>
#include <QString>
#include <QSvgRenderer>
#include <QPushButton>

class AntTabWidget : public QWidget
{
	Q_OBJECT
public:
	explicit AntTabWidget(const QString& title, const QString& icon, int tabwidth, int tabHeight, QWidget* parent = nullptr);

	QString title() const;
	void setSelected(bool selected);
	bool isSelected() const;

	// Getter 和 Setter 方法
	int tabWidth() const;
	void setTabWidth(int width);

	int tabHeight() const;
	void setTabHeight(int height);
signals:
	void requestClose(AntTabWidget* tab);
protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	bool m_selected = false;
	bool m_hovered = false;
	QString m_title;
	QString m_icon;
	QColor hoverBgColor;
	QColor textColor;
	QSvgRenderer* m_svgRenderer;
	QPushButton* m_closeButton = nullptr;
	int m_iconSize = 24;
	int m_iconTopOffset = 5;	// 自己调整合适即可
	int m_tabWidth;
	int m_tabHeight;
};
