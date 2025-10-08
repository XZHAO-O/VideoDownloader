#pragma once

#include <QWidget>
#include <QImage>
#include <QUrl>
#include <QEvent>
#include <QPushButton>
#include "GaussianBlur.h"

class GlassWidget : public QWidget
{
	Q_OBJECT
public:
	GlassWidget(float blurVlue = 10.0f, int passes = 3, QWidget* parent = nullptr);

	void setBackgroundImage(const QImage& bg);
	void clearImage() { m_blurredBackground = QImage(); update(); }
	void setPixmapAndText(QString pixmap, QString title, QString text);
	void setUrl(QUrl url);
protected:
	void resizeEvent(QResizeEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
private:
	QImage m_blurredBackground;
	GaussianBlur m_blur;
	float m_opacity = 1.0f;
	float m_blurRadius = 10.f;
	QPixmap m_pixmap;
	QFont m_titleFont;
	QFont m_textFont;
	QString m_titleText;
	QString m_text;
	QPushButton* m_btn = nullptr;
};
