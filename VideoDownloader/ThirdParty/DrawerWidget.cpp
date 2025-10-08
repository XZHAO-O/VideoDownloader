#include "DrawerWidget.h"
#include "DesignSystem.h"
#include "StyleSheet.h"
#include <QLabel>
#include <QVBoxLayout>

DrawerWidget::DrawerWidget(int width, QWidget* parent)
	: QWidget(parent)
{
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setObjectName("DrawerWidget");
	setFixedWidth(width);
	move(-width, 0);

	QLabel* titile = new QLabel("抽屉", this);
	QFont font;
	font.setBold(true);
	font.setPointSize(15);
	titile->setFont(font);
	titile->move(15, 15);

	animation = new QPropertyAnimation(this, "pos");
	animation->setDuration(300);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	animation->setStartValue(QPoint(-width, 0));	// 初始位置
	animation->setEndValue(QPoint(0, 0));			// 结束位置

	connect(DesignSystem::instance()->getDarkMask(), &MaskWidget::clicked, this, [this]()
		{
			closeDrawer();
			DesignSystem::instance()->getDarkMask()->hideAnim();
		});

	connect(animation, &QPropertyAnimation::finished, this, [this]()
		{
			if (animation->direction() == QAbstractAnimation::Backward)
			{
				hide();
			}
		});

	hide();
}

DrawerWidget::~DrawerWidget()
{
}

void DrawerWidget::showDrawer()
{
	if (animation->state() == QAbstractAnimation::Running)
		return;

	show();
	animation->setDirection(QAbstractAnimation::Forward);
	animation->start();
}

void DrawerWidget::closeDrawer()
{
	if (animation->state() == QAbstractAnimation::Running)
		return;

	animation->setDirection(QAbstractAnimation::Backward);
	animation->start();
}

void DrawerWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setBrush(QBrush(DesignSystem::instance()->currentTheme().backgroundColor)); // 设定背景颜色
	painter.setPen(Qt::NoPen);
	painter.drawRect(0, 0, width(), height());  // 绘制背景
}