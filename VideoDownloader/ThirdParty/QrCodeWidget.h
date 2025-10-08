#pragma once

#include <QWidget>
#include <QImage>
#include "qrcodegen.hpp"

class QrCodeWidget : public QWidget
{
	Q_OBJECT
public:
	explicit QrCodeWidget(QWidget* parent = nullptr);

	void setData(const QString& text);
	QString dataString() const;

	void setScale(int scale);
	int scale() const;

	void setBorder(int border);
	int border() const;

protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
private:
	void generateQrCodeImage();

	QString m_text;
	int m_scale = 10;
	int m_border = 4;
	QImage m_qrImage;
};
