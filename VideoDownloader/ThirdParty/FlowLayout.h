#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QWidget>
#include <QPropertyAnimation>
#include <QMargins>

class FlowLayout : public QLayout
{
	Q_OBJECT

public:
	explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int spacing = -1);
	~FlowLayout();

	void addItem(QLayoutItem* item) override;

	int horizontalSpacing() const;
	int verticalSpacing() const;

	void setGeometry(const QRect& rect) override;

	QSize sizeHint() const override;
	QSize minimumSize() const override;

	int count() const override;
	QLayoutItem* itemAt(int index) const override;
	QLayoutItem* takeAt(int index) override;

protected:
	void doLayout(const QRect& rect);

private:
	QList<QLayoutItem*> itemList;
	int m_spacing;
};

#endif // FLOWLAYOUT_H
