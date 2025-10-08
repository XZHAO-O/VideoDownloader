#include "LogoWidget.h"
#include <QPainter>

LogoWidget::LogoWidget(QWidget* parent)
    : QWidget(parent)
{
    // 加载图片资源
    m_logoPixmap.load(":/Imgs/github.png");
}

LogoWidget::~LogoWidget()
{
}

void LogoWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QSize widgetSize = this->size();

    // 乘以缩放比例，比如 0.7
    double scaleFactor = 0.7;
    QSize scaledSize = widgetSize * scaleFactor;

    QPixmap scaledPixmap = m_logoPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 计算居中位置（相对于控件）
    int x = (widgetSize.width() - scaledPixmap.width()) / 2;
    int y = (widgetSize.height() - scaledPixmap.height()) / 2;

    painter.drawPixmap(x, y, scaledPixmap);
}
