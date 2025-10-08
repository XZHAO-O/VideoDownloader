#pragma once
#include <QSpinBox>
#include <QToolButton>
#include <QPropertyAnimation>
#include "DesignSystem.h"

class AntBaseSpinBox : public QSpinBox
{
	Q_OBJECT
		Q_PROPERTY(int buttonX READ buttonX WRITE setButtonX)

public:
	explicit AntBaseSpinBox(QWidget* parent = nullptr);

	int buttonX() const { return m_buttonX; }
	void setButtonX(int x)
	{
		m_buttonX = x;
		updateButtonsPosition();
	}
protected:
	void resizeEvent(QResizeEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void stepBy(int steps);
	void focusInEvent(QFocusEvent* event);
	void focusOutEvent(QFocusEvent* event);
private:
	void updateButtonsPosition();
private:
	QToolButton* m_plusBtn;
	QToolButton* m_minusBtn;
	QPropertyAnimation* m_animation;
	Theme theme = DesignSystem::instance()->currentTheme();
	QColor borderColor;
	QColor primaryColor;
	int m_buttonX;
	
};
