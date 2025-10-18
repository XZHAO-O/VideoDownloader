#pragma once

#include <QWidget>

class QLabel;
class QVBoxLayout;

class QRCodeLoginWidget : public QWidget
{
	Q_OBJECT

public:
	explicit QRCodeLoginWidget(QWidget* parent = nullptr);
	~QRCodeLoginWidget();

	void setContent(const QString& title, QWidget* customWidget);
	void updateTheme();

	int pageWidth() { return w; }
	int pageHeight() { return h; }

private:
	int w;
	int h;
	QLabel* m_titleLabel = nullptr;
	QWidget* m_contentWidget = nullptr;
	QVBoxLayout* m_mainLayout = nullptr;
};