#include "BubbleViewController.h"
#include <QGraphicsDropShadowEffect>

BubbleViewController::BubbleViewController(QWidget* parent)
	: QGraphicsView(parent)
{
	// 窗口属性设置
	setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
	setAttribute(Qt::WA_TranslucentBackground);  // 关键：启用透明背景

	// 场景设置
	scene = new QGraphicsScene(this);
	setScene(scene);

	// 视图设置
	setFrameStyle(QFrame::NoFrame);
	setStyleSheet("background: transparent; border: none;");
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// 关键性能优化
	setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
	setCacheMode(QGraphicsView::CacheNone);  // 小动画不需要缓存

	// 用户气泡信息
	bubble = new UserInfoBubble(nullptr);
	// 让bubble根据内容自动调整大小
	bubble->adjustSize();  // 计算内部实际的总尺寸 根据子控件sizehint
	// 获取bubble的实际大小
	QSize contentSize = bubble->sizeHint();  // 或者 bubble->size()
	// 添加一些边距，为阴影效果留空间
	int shadowMargin = 12;  // 阴影模糊半径 + 额外边距
	m_bubbleSize = QSize(contentSize.width() + shadowMargin * 2, contentSize.height() + shadowMargin * 2);
	setFixedSize(m_bubbleSize.width(), m_bubbleSize.height());
	setSceneRect(QRectF(0, 0, m_bubbleSize.width(), m_bubbleSize.height()));

	// 设置图形代理
	proxy = scene->addWidget(bubble);
	proxy->setTransformOriginPoint(proxy->boundingRect().topLeft());
	// 性能优化：设置代理缓存模式
	proxy->setCacheMode(QGraphicsItem::ItemCoordinateCache);

	// 阴影效果
	auto* shadow = new QGraphicsDropShadowEffect(this);
	shadow->setBlurRadius(20);
	shadow->setOffset(0, 0);
	shadow->setColor(QColor(50, 50, 50, 120));
	proxy->setGraphicsEffect(shadow);

	// 动画设置
	scaleAnim = new QPropertyAnimation(proxy, "scale");
	scaleAnim->setDuration(300);
	scaleAnim->setEasingCurve(QEasingCurve::OutCubic);

	opacityAnim = new QPropertyAnimation(proxy, "opacity");
	opacityAnim->setDuration(300);
	opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

	scaleAnim->setStartValue(0.5);
	scaleAnim->setEndValue(1.0);
	opacityAnim->setStartValue(0.0);
	opacityAnim->setEndValue(1.0);

	groupAnim = new QParallelAnimationGroup(this);
	groupAnim->addAnimation(scaleAnim);
	groupAnim->addAnimation(opacityAnim);

	// 动画完成后
	connect(groupAnim, &QParallelAnimationGroup::finished, this, [this]()
		{
			if (groupAnim->direction() == QAbstractAnimation::Backward)
				hide();
		});

	// 转发信号
	connect(bubble, &UserInfoBubble::requestHide, this, &BubbleViewController::requestHide);
	connect(bubble, &UserInfoBubble::exitLogin, this, &BubbleViewController::exitLogin);
	hide();
}

BubbleViewController::~BubbleViewController()
{
	if (bubble) bubble->deleteLater();
}

void BubbleViewController::showAnimated(QPoint globalPos)
{
	proxy->update();  // 强制刷新一次图像缓存 预热缓存，避免动画前卡顿
	// 设置初始缩放
	QRect startRect(globalPos.x(), globalPos.y(), m_bubbleSize.width(), m_bubbleSize.height());
	setGeometry(startRect);
	show();

	// 开始播放动画
	groupAnim->stop();
	groupAnim->setDirection(QAbstractAnimation::Forward);
	groupAnim->start();
}

void BubbleViewController::hideAnimated()
{
	groupAnim->stop();
	groupAnim->setDirection(QAbstractAnimation::Backward);
	groupAnim->start();
}