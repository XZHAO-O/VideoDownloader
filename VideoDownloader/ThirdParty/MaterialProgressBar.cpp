#include "MaterialProgressBar.h"
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QPainterPath>
#include "DesignSystem.h"

MaterialProgressBar::MaterialProgressBar(QWidget* parent)
	: QProgressBar(parent),
	m_backgroundColor(DesignSystem::instance()->currentTheme().progressBarBgColor),
	m_chunkColor(DesignSystem::instance()->primaryColor()),
	m_animation(new QVariantAnimation(this)) // 初始化动画对象
{
	setRange(0, 100);         // 设置默认最小值和最大值
	setTextVisible(false);   // 默认不显示文字

	// 配置动画
	m_animation->setDuration(300);
	m_animation->setEasingCurve(QEasingCurve::InOutCubic);

	// 动画值变化时更新进度条 value
	connect(m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
		{
			setValue(value.toInt());	// 每一帧更新实际 value()
		});

	// 不确定动画
	m_indeterminateAnimation = new QVariantAnimation(this);
	m_indeterminateAnimation->setStartValue(0.0);
	m_indeterminateAnimation->setEndValue(1.0);
	m_indeterminateAnimation->setDuration(1500);
	m_indeterminateAnimation->setLoopCount(-1);
	m_indeterminateAnimation->setEasingCurve(QEasingCurve::InOutSine);
	connect(m_indeterminateAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
		{
			m_indeterminatePosition = value.toReal();
			update();
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_backgroundColor = DesignSystem::instance()->currentTheme().progressBarBgColor;
			m_chunkColor = DesignSystem::instance()->primaryColor();
			update();
		});
}

QSize MaterialProgressBar::sizeHint() const
{
	return QSize(200, 10);
}

void MaterialProgressBar::setProgressAnimated(int targetValue)
{
	targetValue = std::clamp(targetValue, minimum(), maximum());

	if (m_animation->state() == QAbstractAnimation::Running)
	{
		m_animation->stop();
	}

	m_animation->setStartValue(value());
	m_animation->setEndValue(targetValue);
	m_animation->start();
}

void MaterialProgressBar::startTestPattern()
{
	// 初始启动不确定动画
	QTimer::singleShot(300, this, [this]()
		{
			startIndeterminate();
		});

	// 准备切换动画的定时器
	QTimer::singleShot(3000, this, [this]()
		{
			updateProgress();
		});
}

void MaterialProgressBar::updateProgress()
{
	stopIndeterminate();
	setValue(0);
	connect(m_animation, &QAbstractAnimation::finished, this, &MaterialProgressBar::startTestPattern, Qt::UniqueConnection);
	setRealProgress(100);
}

void MaterialProgressBar::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QRectF rect = this->rect();
	qreal radius = rect.height() / 2.0;

	// 画背景
	painter.setPen(Qt::NoPen);
	painter.setBrush(m_backgroundColor);
	painter.drawRoundedRect(rect, radius, radius);

	if (m_isBeginIndeterminate)
	{
		// 不确定动画：绘制一个宽度为 40% 的滑块，在0~1之间循环滑动
		qreal chunkWidthRatio = 0.4;
		qreal startX = rect.left() + (rect.width() + chunkWidthRatio * rect.width()) * m_indeterminatePosition - chunkWidthRatio * rect.width();
		QRectF chunkRect(startX, rect.top(), chunkWidthRatio * rect.width(), rect.height());

		// 裁剪圆角矩形外的一切
		QPainterPath clipPath;
		clipPath.addRoundedRect(rect, radius, radius);
		painter.setClipPath(clipPath);

		QColor chunkColor = m_chunkColor;
		painter.setBrush(chunkColor);
		painter.drawRoundedRect(chunkRect, radius, radius);
	}
	else
	{
		// 正常进度条
		double progress = qBound(0.0, double(value() - minimum()) / double(maximum() - minimum()), 1.0);
		double chunkWidth = rect.width() * progress;

		QColor chunkColor = (progress > 0.5) ? QColor("#52c41a") : m_chunkColor;

		QRectF chunkRect = QRectF(rect.left(), rect.top(), chunkWidth, rect.height());
		painter.setBrush(chunkColor);
		painter.drawRoundedRect(chunkRect, radius, radius);
	}
}

void MaterialProgressBar::setRealProgress(int targetValue)
{
	targetValue = std::clamp(targetValue, minimum(), maximum());

	// 任务回退或停滞，不动画，直接显示
	if (targetValue <= value())
	{
		if (m_animation->state() == QAbstractAnimation::Running)
			m_animation->stop();
		setValue(targetValue);
		return;
	}

	// 当前正在补间，如果目标没变，不重复启动动画
	if (m_animation->state() == QAbstractAnimation::Running &&
		m_animation->endValue().toInt() == targetValue)
	{
		return;
	}

	// 停止当前动画，准备新的补间
	if (m_animation->state() == QAbstractAnimation::Running)
		m_animation->stop();

	m_animation->setStartValue(value());
	m_animation->setEndValue(targetValue);

	// 动画时间取决于距离，可调节
	constexpr int minDuration = 500;
	constexpr int maxDuration = 3000;
	constexpr double maxSpeed = 0.02;  // 百分比/ms

	double range = maximum() - minimum();
	double deltaRatio = double(targetValue - value()) / range;

	int duration = static_cast<int>((deltaRatio * 100) / maxSpeed);
	duration = std::clamp(duration, minDuration, maxDuration);

	m_animation->setDuration(duration);
	m_animation->setEasingCurve(QEasingCurve::InOutSine);
	m_animation->start();
}