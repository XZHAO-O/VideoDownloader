#pragma once

#include <QWidget>
#include "AntBaseDoubleSpinBox.h"

class AntDoubleNumberInput : public QWidget
{
	Q_OBJECT

public:
	AntDoubleNumberInput(QWidget* parent);
	~AntDoubleNumberInput();

	AntBaseDoubleSpinBox* input() const { return m_input; }

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	AntBaseDoubleSpinBox* m_input;
	QColor m_shadowColor;
};
