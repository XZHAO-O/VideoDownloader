#include "ThemeSwitcher.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include "DesignSystem.h"

ThemeSwitcher::ThemeSwitcher(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_TranslucentBackground);
	setFocusPolicy(Qt::NoFocus);

	isDarkTheme = DesignSystem::instance()->themeMode() == DesignSystem::Dark ? true : false;

	anim = new QPropertyAnimation(this, "circleRadius");
	anim->setDuration(800);
	anim->setEasingCurve(QEasingCurve::InOutCubic);
	connect(anim, &QAbstractAnimation::stateChanged, this, [this](QAbstractAnimation::State newState, QAbstractAnimation::State)
		{
			if (newState == QAbstractAnimation::Running && !isDarkTheme)
			{
				// 正向时切换到深色
				DesignSystem::instance()->switchTheme();
				themeBtn->setIcon(DesignSystem::instance()->setThemeIcon());
				setThemeColor();
				emit DesignSystem::instance()->themeChanged();
			}
			else if (newState == QAbstractAnimation::Running && isDarkTheme)
			{
				// 反向时切换到浅色
				DesignSystem::instance()->switchTheme();
				themeBtn->setIcon(DesignSystem::instance()->setThemeIcon());
				setThemeColor();
				emit DesignSystem::instance()->themeChanged();
			}
		});

	connect(anim, &QPropertyAnimation::finished, this, [this]()
		{
			isDarkTheme = !isDarkTheme;
			currentBackground = QPixmap();  // 主动清理截图
		});
}

ThemeSwitcher::~ThemeSwitcher()
{
}

void ThemeSwitcher::setThemeColor()
{
	QPalette palette = QApplication::palette();
	QColor defaultColor = DesignSystem::instance()->themeMode() == DesignSystem::ThemeMode::Light ? Qt::black : Qt::white;
	palette.setColor(QPalette::WindowText, defaultColor);
	palette.setColor(QPalette::Text, defaultColor);
	palette.setColor(QPalette::ButtonText, defaultColor);
	palette.setColor(QPalette::PlaceholderText, DesignSystem::instance()->currentTheme().placeholderColor);
	QApplication::setPalette(palette);
}

void ThemeSwitcher::paintEvent(QPaintEvent* e)
{
	if (!currentBackground.isNull())
	{
		QPainter painter(this);
		painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

		QPainterPath path;
		QPainterPath circlePath;
		circlePath.addEllipse(circleCenter.x() - radius, circleCenter.y() - radius, radius * 2, radius * 2);

		if (anim->direction() == QAbstractAnimation::Forward)
		{
			// 正向：从中心向外扩张 => 挖掉中间圆
			path.addRect(rect());     // 添加整个窗口区域
			path -= circlePath;       // 减去中间圆形区域（形成挖空效果）
		}
		else
		{
			// 反向：从外向中心收缩 => 只保留中间圆
			path = circlePath;
		}

		painter.setClipPath(path);
		painter.drawPixmap(0, 0, currentBackground);
	}
}

void ThemeSwitcher::resizeByMainWindow(int parentW, int parentH)
{
	resize(parentW, parentH);
	// 计算对角线长度作为最大半径
	int maxRadius = static_cast<int>(std::ceil(std::sqrt(parentW * parentW + parentH * parentH)));
	anim->setStartValue(0);
	anim->setEndValue(maxRadius);
}

void ThemeSwitcher::startSwitchTheme(QPixmap grab, QPushButton* btn, QPoint btnGlobalCenter)
{
	if (anim->state() == QAbstractAnimation::Running)
	{
		return;
	}

	if (btn)
	{
		themeBtn = btn;
		currentBackground = grab;

		// 获取圆形的中心点 (全局坐标转为当前控件的局部坐标)
		circleCenter = mapFromGlobal(btnGlobalCenter);

		if (!isDarkTheme)
		{
			anim->setDirection(QAbstractAnimation::Forward);
		}
		else
		{
			anim->setDirection(QAbstractAnimation::Backward);
		}

		anim->start();
	}
}