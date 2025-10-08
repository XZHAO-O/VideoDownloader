#include "PopupViewController.h"
#include "DesignSystem.h"
#include <QGraphicsDropShadowEffect>

PopupViewController::PopupViewController(int height, bool enableMultiLevel, QWidget* parent)
	: QGraphicsView(parent), m_height(height), m_isVisible(false)
{
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	scene = new QGraphicsScene(this);
	setScene(scene);
	setFrameStyle(QFrame::NoFrame);
	setStyleSheet("background: transparent; border: none;");
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// 关键性能优化
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setCacheMode(QGraphicsView::CacheNone);

	// 设置图形代理
	popup = new PopupWidget(height, enableMultiLevel, nullptr);
	proxy = scene->addWidget(popup);
	proxy->setCacheMode(QGraphicsItem::NoCache);

	// 动画
	scaleTransform = new QGraphicsScale();	// 支持三维缩放，它可以分别沿 X、Y、Z 三个轴设置缩放比例
	proxy->setTransformations({ scaleTransform });

	opacityAnim = new QPropertyAnimation(proxy, "opacity");
	opacityAnim->setDuration(200);
	opacityAnim->setStartValue(0.0);
	opacityAnim->setEndValue(1.0);
	opacityAnim->setEasingCurve(QEasingCurve::InOutCubic);

	scaleAnim = new QPropertyAnimation(scaleTransform, "yScale");
	scaleAnim->setDuration(200);
	scaleAnim->setStartValue(0.8);
	scaleAnim->setEndValue(1.0);
	scaleAnim->setEasingCurve(QEasingCurve::InOutCubic);

	groupAnim = new QParallelAnimationGroup(this);
	groupAnim->addAnimation(scaleAnim);
	groupAnim->addAnimation(opacityAnim);

	connect(groupAnim, &QParallelAnimationGroup::finished, this, [this]()
		{
			if (!m_isVisible)
			{
				hide();
			}
		});
	connect(popup, &PopupWidget::itemSelected, this, [this](const QModelIndex& idx)
		{
			emit itemSelected(idx);  // 转发选中信号
		});
}

PopupViewController::~PopupViewController()
{
}

void PopupViewController::showAnimated(const QPoint& pos, int width)
{
	if (groupAnim->state() == QAbstractAnimation::Running)
		return;  // 如果动画正在运行，直接返回

	// 设置视图场景尺寸
	proxy->update();
	show();
	move(pos.x(), pos.y() - 3);

	m_isVisible = true;
	groupAnim->stop();
	groupAnim->setDirection(QAbstractAnimation::Forward);
	groupAnim->start();
}

void PopupViewController::hideAnimated()
{
	if (groupAnim->state() == QAbstractAnimation::Running)
		return;  // 如果动画正在运行，直接返回

	m_isVisible = false;
	groupAnim->stop();
	groupAnim->setDirection(QAbstractAnimation::Backward);
	groupAnim->start();
}

void PopupViewController::updateSize(int width, int height)
{
	setFixedSize(width, m_height);
	setSceneRect(QRectF(0, 0, width, m_height));
	popup->setFixedSize(width, m_height);  // 确保popup大小正确
	proxy->setTransformOriginPoint(QPointF(proxy->boundingRect().width() / 2, 0));	// 顶部中心
	scaleTransform->setOrigin(QVector3D(proxy->boundingRect().width() / 2, 0, 0));
}

void PopupViewController::follow(QWidget* anchorWidget, AnchorPoint anchor)
{
	if (!anchorWidget) return;

	QRect rect = anchorWidget->rect();
	QPoint globalPos;

	switch (anchor)
	{
	case AnchorPoint::TopLeft:
		globalPos = anchorWidget->mapToGlobal(rect.topLeft());
		break;
	case AnchorPoint::TopRight:
		globalPos = anchorWidget->mapToGlobal(rect.topRight());
		break;
	case AnchorPoint::BottomLeft:
		globalPos = anchorWidget->mapToGlobal(rect.bottomLeft());
		break;
	case AnchorPoint::BottomRight:
		globalPos = anchorWidget->mapToGlobal(rect.bottomRight());
		break;
	}

	this->move(globalPos);
}