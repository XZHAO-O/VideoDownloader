#include "PopupViewController.h"
#include "DesignSystem.h"
#include <QGraphicsDropShadowEffect>

PopupViewController::PopupViewController(int maxHeight, bool enableMultiLevel, QWidget* parent)
	: QGraphicsView(parent), m_maxHeight(maxHeight), m_isVisible(false)
{
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	scene = new QGraphicsScene(this);
	setScene(scene);
	setFrameStyle(QFrame::NoFrame);
	setStyleSheet("background: transparent; border: none;");
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	setCacheMode(QGraphicsView::CacheNone);

	// 使用最大高度创建 PopupWidget
	popup = new PopupWidget(m_maxHeight, enableMultiLevel, nullptr);
	proxy = scene->addWidget(popup);
	proxy->setCacheMode(QGraphicsItem::NoCache);

	scaleTransform = new QGraphicsScale();
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
			emit itemSelected(idx);
		});
}

PopupViewController::~PopupViewController()
{
}

int PopupViewController::getActualHeight() const
{
	return popup->calculateAdaptiveHeight();
}

void PopupViewController::showAnimated(const QPoint& pos, int width)
{
	if (groupAnim->state() == QAbstractAnimation::Running)
		return;

	// 计算自适应高度
	int actualHeight = getActualHeight();

	// 使用新的方法设置尺寸，确保内容正确显示
	setFixedSize(width, actualHeight);
	setSceneRect(QRectF(0, 0, width, actualHeight));
	popup->setFixedSizeWithAdaptiveHeight(width, m_maxHeight);

	proxy->setTransformOriginPoint(QPointF(proxy->boundingRect().width() / 2, 0));
	scaleTransform->setOrigin(QVector3D(proxy->boundingRect().width() / 2, 0, 0));

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
		return;

	m_isVisible = false;
	groupAnim->stop();
	groupAnim->setDirection(QAbstractAnimation::Backward);
	groupAnim->start();
}

void PopupViewController::updateSize(int width, int height)
{
	// 这里仍然使用最大高度，因为实际高度会在显示时计算
	setFixedSize(width, m_maxHeight);
	setSceneRect(QRectF(0, 0, width, m_maxHeight));
	popup->setFixedSize(width, m_maxHeight);
	proxy->setTransformOriginPoint(QPointF(proxy->boundingRect().width() / 2, 0));
	scaleTransform->setOrigin(QVector3D(proxy->boundingRect().width() / 2, 0, 0));
}

void PopupViewController::follow(QWidget* anchorWidget, AnchorPoint anchor)
{
	if (!anchorWidget) return;

	QRect rect = anchorWidget->rect();
	QPoint globalPos;

	switch (anchor)
	{
	case TopLeft:
		globalPos = anchorWidget->mapToGlobal(rect.topLeft());
		break;
	case TopRight:
		globalPos = anchorWidget->mapToGlobal(rect.topRight());
		break;
	case BottomLeft:
		globalPos = anchorWidget->mapToGlobal(rect.bottomLeft());
		break;
	case BottomRight:
		globalPos = anchorWidget->mapToGlobal(rect.bottomRight());
		break;
	}

	this->move(globalPos);
}