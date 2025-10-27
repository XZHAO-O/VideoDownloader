#include "CircularAvatar.h"

#include <qpainterpath.h>
#include <QEvent>
#include <QEnterEvent>
#include <QTimer>

#include "BubbleViewController.h"
#include "DialogViewController.h"

CircularAvatar::CircularAvatar(QSize size, QString prevImgPath, QString afterImgPath, BubbleViewController* bubble, DialogViewController* dialogView, QWidget* parent)
	: QWidget(parent)
	, m_bubble(bubble)
	, m_dialogView(dialogView)
{
	setFixedSize(size);

	connect(this, &CircularAvatar::playAnim, m_bubble, &BubbleViewController::showAnimated);
	connect(this, &CircularAvatar::hideAnim, m_bubble, &BubbleViewController::hideAnimated);
	connect(m_bubble, &BubbleViewController::requestHide, this, &CircularAvatar::checkShouldHideBubble);
	connect(m_bubble, &BubbleViewController::exitLogin, this, &CircularAvatar::exitLogin);

	connect(this, &CircularAvatar::showDialog, m_dialogView, &DialogViewController::showAnim);


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

void CircularAvatar::exitLogin(bool loginState)
{
	m_isLogin = loginState;
	m_isClicked = loginState ? true : false;
	update();
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
}

void CircularAvatar::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		emit showDialog(MaterialDialog::QRCodeLogin);
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