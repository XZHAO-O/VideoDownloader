#pragma once

#include <QWidget>
#include <QPixmap>
#include <QVariantAnimation>

class CardWidget : public QWidget
{
	Q_OBJECT

public:
	CardWidget(QString lastLogin, QString totalTime, QWidget* parent = nullptr);
	void setImageFile(const QString& path);

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	QPixmap m_pixmap;
	QVariantAnimation* m_scaleAnimation;
	qreal m_scaleFactor = 1.0;
	QString m_lastLogin;
	QString m_totalTime;
};
