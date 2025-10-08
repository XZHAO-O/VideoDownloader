#pragma once

#include <QWidget>
#include "AntBaseSpinBox.h"

class AntNumberInput : public QWidget
{
	Q_OBJECT
public:
	explicit AntNumberInput(QWidget* parent = nullptr);

	AntBaseSpinBox* input() const { return m_input; }

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	AntBaseSpinBox* m_input;
	QColor m_shadowColor;
};
