// AntInput.h
#pragma once
#include "AntBaseInput.h"
#include <QToolButton>
#include <QPropertyAnimation>
#include "PopupViewController.h"

class AntInput : public AntBaseInput
{
	Q_OBJECT
public:
	explicit AntInput(int popupHeight, QStringList itemTextList, QWidget* parent = nullptr);

	//PopupViewController* PopupView() { return popupView; };

signals:
	void searchClicked();

protected:
	void resizeEvent(QResizeEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;

private:
	QToolButton* m_searchButton = nullptr;
	//PopupViewController* popupView = nullptr;

	void updateSearchButtonPosition();
	void setCurrentText(QString text);
};
