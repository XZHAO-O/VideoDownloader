#include "SlideStackedWidget.h"
#include <QParallelAnimationGroup>
#include "DesignSystem.h"

SlideStackedWidget::SlideStackedWidget(QWidget* parent)
	: QStackedWidget(parent)
{
	setObjectName("SlideStackedWidget");
	setFrameShape(QFrame::NoFrame);
	setContentsMargins(0, 0, 0, 0);
	setStyleSheet(QString("#SlideStackedWidget { background-color: %1; }").arg(DesignSystem::instance()->backgroundColor().name()));

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			setStyleSheet(QString("#SlideStackedWidget { background-color: %1; }").arg(DesignSystem::instance()->backgroundColor().name()));
		});
}

SlideStackedWidget::~SlideStackedWidget()
{
}

void SlideStackedWidget::slideToPage(QWidget* nextWidget, SlideDirection direction, int duration,
	AnimationStyle style, std::function<void()> onFinished)
{
	if (isAnimating || nextWidget == currentWidget()) return;
	isAnimating = true;

	QWidget* currentPage = currentWidget();
	int w = width();
	int h = height();

	currentPage->resize(w, h);
	nextWidget->resize(w, h);

	// 小幅滑入距离
	int offset = (direction == RightToLeft) ? 50 : -50;
	QPoint nextStart(offset, 0);
	QPoint nextEnd(0, 0);

	// 初始化新页面位置
	nextWidget->move(nextStart);
	nextWidget->show();

	// 缓动曲线
	QEasingCurve easing = getEasingCurve(style);

	// 位置动画
	QPropertyAnimation* anim = new QPropertyAnimation(nextWidget, "pos");
	anim->setDuration(duration);
	anim->setStartValue(nextStart);
	anim->setEndValue(nextEnd);
	anim->setEasingCurve(easing);

	connect(anim, &QPropertyAnimation::finished, this, [=]()
		{
			setCurrentWidget(nextWidget);
			currentPage->move(0, 0);
			isAnimating = false;
			if (onFinished) onFinished();
		});

	currentPage->hide();

	anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// 额外的辅助方法：提供不同风格缓动曲线选项
QEasingCurve SlideStackedWidget::getEasingCurve(AnimationStyle style)
{
	switch (style) {
	case InOutSine:
		return QEasingCurve(QEasingCurve::InOutSine);

	case Standard:
		// iOS标准页面切换曲线
		return QEasingCurve(QEasingCurve::OutCubic);

	case OutCubic:
		// iOS弹簧阻尼效果
		return QEasingCurve(QEasingCurve::OutCubic);

	case Custom:
	{
		// 自定义贝塞尔曲线，更接近真实iOS效果
		QEasingCurve curve(QEasingCurve::BezierSpline);
		// iOS 的 cubic-bezier(0.25, 0.1, 0.25, 1)
		curve.addCubicBezierSegment(QPointF(0.25, 0.1),
			QPointF(0.25, 1.0),
			QPointF(1.0, 1.0));
		return curve;
	}

	case Win11:
	{
		// Win11 风格：温和启动、柔和结束，类似 cubic-bezier(0.2, 0.0, 0.0, 1)
		QEasingCurve curve(QEasingCurve::BezierSpline);
		curve.addCubicBezierSegment(QPointF(0.2, 0.0),
			QPointF(0.0, 1.0),
			QPointF(1.0, 1.0));
		return curve;
	}

	default:
		return QEasingCurve(QEasingCurve::OutCubic);
	}
}

void SlideStackedWidget::slideFromBottomToTop(QWidget* nextWidget, int duration, std::function<void()> onFinished)
{
	if (!nextWidget) return;
	if (nextWidget == currentWidget())
	{
		if (onFinished) onFinished();
		return;
	}

	isAnimating = true;
	QWidget* currentPage = currentWidget();
	int w = width();
	int h = height();

	// 确保尺寸固定（防止布局跳动）
	currentPage->resize(w, h);
	nextWidget->resize(w, h);

	// 新页面初始位置：y = 0.3 * h
	QPoint nextStart(0, static_cast<int>(0.3 * h));
	QPoint nextEnd(0, 0);

	// 准备新页面
	nextWidget->move(nextStart);
	nextWidget->show();

	// 缓动曲线
	QEasingCurve easing = getEasingCurve(OutCubic);

	// 动画：新页面位置动画
	QPropertyAnimation* animNextPos = new QPropertyAnimation(nextWidget, "pos");
	animNextPos->setDuration(duration);
	animNextPos->setStartValue(nextStart);
	animNextPos->setEndValue(nextEnd);
	animNextPos->setEasingCurve(easing);

	// 动画完成时：切换页面，复原位置
	connect(animNextPos, &QPropertyAnimation::finished, this, [=]()
		{
			setCurrentWidget(nextWidget);
			nextWidget->move(0, 0);  // 确保到位
			isAnimating = false;
			if (onFinished) onFinished();	// 调用回调函数
		});

	// 隐藏当前页面（立即消失，不动画）
	currentPage->hide();

	animNextPos->start(QAbstractAnimation::DeleteWhenStopped);
}