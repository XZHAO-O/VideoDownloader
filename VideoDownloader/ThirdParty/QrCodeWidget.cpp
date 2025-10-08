#include "QrCodeWidget.h"
#include <QPainter>
#include "DesignSystem.h"

using namespace qrcodegen;

QrCodeWidget::QrCodeWidget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName("QrCodeWidget");
}

void QrCodeWidget::setData(const QString& data)
{
	if (m_text == data)
		return;

	m_text = data;
	generateQrCodeImage();
	update();
}

QString QrCodeWidget::dataString() const
{
	return m_text;
}

void QrCodeWidget::setScale(int scale)
{
	if (scale <= 0)
		return;
	if (m_scale == scale)
		return;
	m_scale = scale;
	generateQrCodeImage();
	update();
}

int QrCodeWidget::scale() const
{
	return m_scale;
}

void QrCodeWidget::setBorder(int border)
{
	if (border < 0)
		return;
	if (m_border == border)
		return;
	m_border = border;
	generateQrCodeImage();
	update();
}

int QrCodeWidget::border() const
{
	return m_border;
}

void QrCodeWidget::generateQrCodeImage()
{
	if (m_text.isEmpty()) {
		m_qrImage = QImage();
		return;
	}

	QrCode qr = QrCode::encodeText(m_text.toUtf8().constData(), QrCode::Ecc::LOW);
	int qrSize = qr.getSize();

	// 动态计算缩放因子，确保不超出控件大小
	int maxImageSize = qMin(width(), height());
	m_scale = qMax(1, maxImageSize / (qrSize + m_border * 2));  // 防止为 0

	int imgSize = (qrSize + m_border * 2) * m_scale;
	m_qrImage = QImage(imgSize, imgSize, QImage::Format_RGB32);
	m_qrImage.fill(Qt::white);

	QPainter painter(&m_qrImage);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::black);

	for (int y = 0; y < qrSize; ++y) {
		for (int x = 0; x < qrSize; ++x) {
			if (qr.getModule(x, y)) {
				painter.drawRect((x + m_border) * m_scale, (y + m_border) * m_scale, m_scale, m_scale);
			}
		}
	}
}

void QrCodeWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);             // 抗锯齿
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);    // 缩放时平滑处理

	if (!m_qrImage.isNull()) {
		QPoint topLeft((width() - m_qrImage.width()) / 2, (height() - m_qrImage.height()) / 2);
		painter.drawImage(topLeft, m_qrImage);
	}
}

void QrCodeWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	generateQrCodeImage();		// 根据新的控件大小重新生成二维码
}