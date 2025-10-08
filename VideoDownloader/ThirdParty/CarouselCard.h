#pragma once

#include <QWidget>
#include <QUrl>
#include <QToolButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QResizeEvent>

class CarouselCard : public QWidget
{
	Q_OBJECT
public:
	CarouselCard(const QString& imagePath, QWidget* parent = nullptr);
	~CarouselCard();

	void addTab(QString text, QColor bgColor);
	void enableUrlBtn(QUrl url);
	void setTextContent(QString text);
protected:
	void paintEvent(QPaintEvent*) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	int w = 200;
	int h = 80;
	QString m_imagePath;
	QPixmap m_pixmap;
	qreal m_fontSize;
	QToolButton* btn = nullptr;
	QLabel* textLabel = nullptr;
	QPropertyAnimation* fontAnim = nullptr;
};
