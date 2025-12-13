#include "AntTooltipViewController.h"

AntTooltipViewController::AntTooltipViewController(QString text, AntTooltip::ArrowDir dir, QWidget* parent)
	: QGraphicsView(parent)
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
	setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
	setCacheMode(QGraphicsView::CacheBackground);

	// 设置图形代理
	tooltip = new AntTooltip(text, dir, nullptr);
	proxy = scene->addWidget(tooltip);
	proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);
	int width = tooltip->width();
	int height = tooltip->height();
	setFixedSize(width, height);
	setSceneRect(QRectF(0, 0, width, height));
	updateTransformOrigin();

	// 动画
	scaleAnim = new QPropertyAnimation(proxy, "scale");
	scaleAnim->setDuration(200);
	scaleAnim->setStartValue(0.8);
	scaleAnim->setEndValue(1.0);
	scaleAnim->setEasingCurve(QEasingCurve::OutCubic);

	opacityAnim = new QPropertyAnimation(proxy, "opacity");
	opacityAnim->setDuration(200);
	opacityAnim->setStartValue(0.0);
	opacityAnim->setEndValue(1.0);
	opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

	groupAnim = new QParallelAnimationGroup(this);
	groupAnim->addAnimation(scaleAnim);
	groupAnim->addAnimation(opacityAnim);

	connect(tooltip, &AntTooltip::resized, this, [this](int width, int height) {
		setFixedSize(width, height);
		setSceneRect(QRectF(0, 0, width, height));
		proxy->setGeometry(QRectF(0, 0, width, height));
		updateTransformOrigin();
		});

	// 信号槽
	connect(groupAnim, &QParallelAnimationGroup::finished, this, [this]()
		{
			if (isHide)
			{
				hide();
				emit hidden();
			}
		});
}

AntTooltipViewController::~AntTooltipViewController()
{
	// 停止所有动画
	if (groupAnim) {
		groupAnim->stop();
	}

	// 清理场景和代理
	if (scene) {
		scene->clear();
		delete scene;
	}
}

void AntTooltipViewController::showAnimated(QPoint globalPos)
{
	isHide = false;

	// 如果正在隐藏，停止动画
	if (groupAnim && groupAnim->state() == QAbstractAnimation::Running) {
		groupAnim->stop();
	}

	// 设置视图场景尺寸
	proxy->update();
	show();
	move(globalPos.x(), globalPos.y());

	// 确保在显示前重置缩放和透明度
	proxy->setScale(0.8);
	proxy->setOpacity(0.0);

	groupAnim->stop();
	groupAnim->setDirection(QAbstractAnimation::Forward);
	groupAnim->start();
}

void AntTooltipViewController::hideAnimated()
{
	isHide = true;
	groupAnim->setDirection(QAbstractAnimation::Backward);
	groupAnim->start();
}

void AntTooltipViewController::updateTransformOrigin()
{
	QPointF origin;
	QRectF rect = proxy->boundingRect();

	switch (tooltip->getArrowDirection())
	{
	case AntTooltip::ArrowTop:    origin = QPointF(rect.width() / 2, 0); break;
	case AntTooltip::ArrowBottom: origin = QPointF(rect.width() / 2, rect.height()); break;
	case AntTooltip::ArrowLeft:   origin = QPointF(0, rect.height() / 2); break;
	case AntTooltip::ArrowRight:  origin = QPointF(rect.width(), rect.height() / 2); break;
	}

	proxy->setTransformOriginPoint(origin);
}