#include "AntInput.h"
#include <QStyle>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QFocusEvent>
#include <QKeyEvent>

AntInput::AntInput(int popupHeight, QStringList itemTextList, QWidget* parent)
	: AntBaseInput(parent)
{
	m_searchButton = new QToolButton(this);
	m_searchButton->setCursor(Qt::PointingHandCursor);
	m_searchButton->setIcon(QIcon(":/Imgs/search.svg"));
	m_searchButton->setFixedSize(17, 17);
	m_searchButton->setIconSize(QSize(17, 17));
	m_searchButton->setStyleSheet(R"(
    QToolButton {
        border: none;
        padding: 0;
        background: transparent;
    })");

	connect(m_searchButton, &QToolButton::clicked, this, [this]() {
		handleSearch();
		});

	QFont font = this->font();
	font.setPointSizeF(10.8);
	setFont(font);
}

void AntInput::resizeEvent(QResizeEvent* event)
{
	AntBaseInput::resizeEvent(event);
	updateSearchButtonPosition();
}

void AntInput::focusOutEvent(QFocusEvent* event)
{
	AntBaseInput::focusOutEvent(event);
	// 焦点移出时不需要特殊处理
}

void AntInput::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
		handleSearch();
		event->accept();
		return;
	}
	AntBaseInput::keyPressEvent(event);
}

void AntInput::updateSearchButtonPosition()
{
	int frameWidth = 3;
	int btnSize = m_searchButton->size().width();
	int padding = 12;

	int buttonX = width() - btnSize - frameWidth - padding;
	m_searchButton->move(buttonX, (height() - btnSize) / 2);

	setTextMargins(frameWidth + padding, 0, btnSize + padding, 0);
}

void AntInput::setCurrentText(QString text)
{
	setText(text);
}

void AntInput::handleSearch()
{
	// 清除焦点，这样用户就可以立即进行其他操作
	clearFocus();
	emit searchClicked();
}