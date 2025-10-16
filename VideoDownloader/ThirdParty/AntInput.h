#pragma once
#include "AntBaseInput.h"
#include <QToolButton>

class AntInput : public AntBaseInput
{
	Q_OBJECT
public:
	explicit AntInput(int popupHeight, QStringList itemTextList, QWidget* parent = nullptr);

signals:
	void searchClicked();

protected:
	void resizeEvent(QResizeEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;

private:
	QToolButton* m_searchButton = nullptr;

	void updateSearchButtonPosition();
	void setCurrentText(QString text);
	void handleSearch();
};