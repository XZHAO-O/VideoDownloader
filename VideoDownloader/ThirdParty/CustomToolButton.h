#pragma once

#include <QToolButton>
#include <QSvgRenderer>
#include <QEvent>
#include <QMouseEvent>

class CustomToolButton : public QToolButton
{
	Q_OBJECT

public:
	explicit CustomToolButton(QSize iconSize, QWidget* parent);
	~CustomToolButton();

	void setSvgIcons(const QString& normalPath, const QString& activePath); // 设置图标

	bool isBtnChecked() const { return m_checked; };
	void setBtnChecked(bool checked)
	{
		m_checked = checked;
		update();
	}

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event);
private:
	bool hovered;
	bool m_checked = false;
	QSize m_iconSize;
	QSvgRenderer* m_normalRenderer = nullptr;
	QSvgRenderer* m_activeRenderer = nullptr;
};
