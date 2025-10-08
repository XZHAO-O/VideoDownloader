#include "CircularAvatar.h"
#include <qpainterpath.h>
#include <QEvent>
#include <QEnterEvent>
#include <QTimer>
#include "AntTooltipManager.h"

CircularAvatar::CircularAvatar(QSize size, QString prevImgPath, QString afterImgPath, QWidget* parent)
	: QWidget(parent)
{
	setFixedSize(size);

	if (!m_bubble)
	{
		m_bubble = new BubbleViewController(this);
		connect(this, &CircularAvatar::playAnim, m_bubble, &BubbleViewController::showAnimated);
		connect(this, &CircularAvatar::hideAnim, m_bubble, &BubbleViewController::hideAnimated);
		connect(m_bubble, &BubbleViewController::requestHide, this, &CircularAvatar::checkShouldHideBubble);
		connect(m_bubble, &BubbleViewController::exitLogin, this, &CircularAvatar::allowLogin);
	}

	// 设置图集
	setImgs(prevImgPath, afterImgPath);
}

CircularAvatar::~CircularAvatar()
{
}

void CircularAvatar::setImgs(QString prevImgPath, QString afterImgPath)
{
	m_prevSvg = new QSvgRenderer(prevImgPath, this);
	m_afterSvg = new QSvgRenderer(afterImgPath, this);
	update();
}

void CircularAvatar::addDialog(DialogViewController* dialog)
{
	m_dialogView = dialog;
	connect(this, &CircularAvatar::showDialog, m_dialogView, &DialogViewController::showAnim);
}

void CircularAvatar::allowLogin(bool loginState)
{
	m_isLogin = loginState;
	m_isClicked = loginState ? true : false;
	update();
}

void CircularAvatar::setAvatar(QString svgFilePath)
{
	// 扩展接口
}

void CircularAvatar::paintEvent(QPaintEvent* e)
{
	Q_UNUSED(e);

	QPainter p(this);

	p.setRenderHint(QPainter::Antialiasing, true);

	// 创建圆形裁剪路径
	QPainterPath path;
	path.addEllipse(rect());
	p.setClipPath(path);

	// 渲染 SVG 画头像
	if (m_prevSvg && m_prevSvg->isValid() && m_afterSvg && m_afterSvg->isValid())
	{
		if (!m_isClicked)
		{
			m_prevSvg->render(&p, rect());
		}
		else
		{
			m_afterSvg->render(&p, rect());
		}
	}
}

void CircularAvatar::enterEvent(QEnterEvent* e)
{
	QWidget::enterEvent(e);

	setCursor(Qt::PointingHandCursor);
	if (!m_lastEnterTime.isValid() || m_lastEnterTime.elapsed() > m_enterIntervalMs)
	{
		// 会把计时器重置到当前时间点
		m_lastEnterTime.restart();

		if (!m_isEnter)
		{
			m_isEnter = true;

			if (m_isLogin)
			{
				// 将气泡框移动到头像右侧的位置上
				QPoint rightPos = mapToGlobal(rect().topRight());
				// 自己调整到合适的位置即可
				emit playAnim(QPoint(rightPos.x(), rightPos.y() - 6));
			}
			else
			{
				AntTooltipManager::instance()->showTooltip(this, "未登录,登录每日签到可获得积分.", AntTooltipManager::Position::Right);
			}
		}
	}
}

void CircularAvatar::leaveEvent(QEvent* e)
{
	QWidget::leaveEvent(e);
	setCursor(Qt::ArrowCursor);
	// 延迟判断防止光标不在2个控件上
	QTimer::singleShot(100, this, [this]()
		{
			checkShouldHideBubble();
		});
	AntTooltipManager::instance()->hideTooltip();
}

void CircularAvatar::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit showDialog(m_isLogin ? MaterialDialog::Profile : MaterialDialog::Login);
	}
}

void CircularAvatar::checkShouldHideBubble()
{
	bool avatarHovered = this->underMouse();
	bool bubbleHovered = m_bubble && m_bubble->underMouse();

	if (!avatarHovered && !bubbleHovered)
	{
		m_isEnter = false;
		emit hideAnim();
	}
}