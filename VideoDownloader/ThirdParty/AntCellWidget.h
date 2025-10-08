#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>

class AntCellWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AntCellWidget(const QString& btnName, QWidget* parent = nullptr);
	~AntCellWidget();

	QPushButton* getBtn() { return button; }
protected:
	bool eventFilter(QObject* watched, QEvent* event);
signals:
	void nameClicked();
	void actionClicked();
private:
	QPushButton* button;
};
