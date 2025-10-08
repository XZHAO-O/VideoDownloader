#include "GlassWidget.h"
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QDesktopServices>

GlassWidget::GlassWidget(float blurVlue, int passes, QWidget* parent)
	: QWidget(parent), m_blur(blurVlue, passes)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_NoSystemBackground);
	setAutoFillBackground(false);
}

void GlassWidget::setBackgroundImage(const QImage& bg)
{
	if (bg.isNull())
		return;

	// 直接用传进来的图像做模糊处理
	m_blurredBackground = m_blur.blur(bg);
	update();
}

void GlassWidget::setPixmapAndText(QString pixmap, QString title, QString text)
{
	m_pixmap = QPixmap(pixmap);

	m_titleFont.setWeight(QFont::Black);
	m_titleFont.setPointSizeF(14.5);
	m_textFont.setPointSizeF(12.5);
	m_titleText = title;
	m_text = text;

	update();
}

void GlassWidget::setUrl(QUrl url)
{
	// 创建按钮
	m_btn = new QPushButton(this);
	m_btn->setStyleSheet("QPushButton { background-color: transparent; border: none; }");
	m_btn->setCursor(Qt::PointingHandCursor);
	m_btn->setIcon(QIcon(":/Imgs/goTo.svg"));
	m_btn->setFixedSize(18, 18);
	m_btn->setIconSize(m_btn->size());

	// 连接按钮点击信号到槽函数
	connect(m_btn, &QPushButton::clicked, this, [url]()
		{
			// 打开浏览器并跳转到指定的 URL
			QDesktopServices::openUrl(url);
		});
}

void GlassWidget::resizeEvent(QResizeEvent* event)
{
	if (m_btn)
	{
		// 获取父窗口的大小
		int parentWidth = this->width();
		int parentHeight = this->height();

		// 计算按钮的右下角位置
		int x = parentWidth - m_btn->width() - 10;
		int y = parentHeight - m_btn->height() - 10;

		// 设置按钮位置
		m_btn->move(x, y);
	}
}

void GlassWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	const int borderWidth = 2;
	const int cornerRadius = 8;
	QRect drawRect = rect().adjusted(2, 2, -2, -2);

	QPainterPath clipPath;
	clipPath.addRoundedRect(drawRect, cornerRadius, cornerRadius);

	// 1. 绘制模糊背景，需裁剪和透明度，必须保存状态
	if (!m_blurredBackground.isNull())
	{
		painter.save();
		painter.setClipPath(clipPath);
		painter.setOpacity(m_opacity);	// 根据需要设置透明度
		painter.drawImage(drawRect, m_blurredBackground);
		painter.restore();
	}

	// 2. 绘制边框
	QPen pen(QColor(255, 255, 255, 100));
	pen.setWidth(borderWidth);
	painter.setPen(pen);
	painter.drawRoundedRect(drawRect, cornerRadius, cornerRadius);

	// 3. 绘制光泽渐变，需要裁剪
	painter.setClipPath(clipPath);
	QLinearGradient glossGradient(drawRect.topLeft(), drawRect.bottomLeft());
	glossGradient.setColorAt(0.0, QColor(255, 255, 255, 10));
	glossGradient.setColorAt(1.0, QColor(255, 255, 255, 80));
	painter.fillRect(drawRect, glossGradient);

	// 4. 绘制圆形头像
	// 计算头像的左上角位置，确保它位于左上角合适的位置
	int x = drawRect.width() * 1 / 10;  // 调整 x 位置
	int y = x;
	int imageSize = qMin(drawRect.width(), drawRect.height()); // 确保头像是正方形
	if (!m_pixmap.isNull())
	{
		painter.save();
		// 定义头像的矩形区域
		QRect avatarRect = QRect(x, y, imageSize / 4, imageSize / 4);
		// 创建一个圆形裁剪路径
		QPainterPath avatarPath;
		avatarPath.addEllipse(avatarRect);
		// 裁剪并绘制头像
		painter.setClipPath(avatarPath);
		painter.drawPixmap(avatarRect, m_pixmap);
		painter.restore();
	}

	// 5. 绘制标题和文本
	int spacing = 10;
	painter.setFont(m_titleFont);
	painter.setPen(Qt::black);

	// 绘制标题
	QRect titleRect = QRect(x, y + imageSize / 4 + spacing, width() - x, height() * 1 / 5);			// 给标题留出足够空间
	painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText);

	// 绘制文本（标题下方）
	painter.setFont(m_textFont);
	QRect textRect = QRect(x, titleRect.bottom(), width() - x, height() - titleRect.bottom() - spacing);  // 让文本位于标题下方
	painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, m_text);
}