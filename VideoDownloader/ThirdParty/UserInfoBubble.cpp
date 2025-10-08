#include "UserInfoBubble.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QTimer>
#include <QDateTime>
#include "StyleSheet.h"
#include "AntMessageManager.h"
#include "DesignSystem.h"

UserInfoBubble::UserInfoBubble(QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowFlags(Qt::FramelessWindowHint);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(margin + arrowWidth + spacing, margin, margin + spacing, margin);
	layout->setSpacing(3);

	// 创建统一字体对象
	QFont font;
	font.setPointSize(11);  // 使用磅值单位（适应DPI缩放）

	// 顶部
	QLabel* nameLabel = new QLabel("用户名：小明", this);
	nameLabel->setFont(font);

	QLabel* emailLabel = new QLabel("邮箱：xiaoming123@example.com", this);
	emailLabel->setFont(font);

	// 中心
	font.setPointSize(10);
	QWidget* gradientWidget = new QWidget(this);
	gradientWidget->setObjectName("gradientWidget");
	gradientWidget->setMinimumHeight(50);
	auto theme = DesignSystem::instance()->currentTheme();
	gradientWidget->setStyleSheet(StyleSheet::gradientQss(theme.vipGradientStartColor, theme.vipGradientMidColor, theme.vipGradientEndColor));

	QWidget* leftWid = new QWidget(gradientWidget);
	m_antBtn = new AntButton("续费", 10, gradientWidget);
	m_antBtn->setMinimumHeight(50);

	QLabel* vipLabel = new QLabel("会员到期时间:", this);

	QString currentTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
	QLabel* timeLabel = new QLabel(currentTimeStr, this);

	vipLabel->setFont(font);
	timeLabel->setFont(font);

	QHBoxLayout* hlay = new QHBoxLayout(gradientWidget);
	QVBoxLayout* vlay = new QVBoxLayout(leftWid);

	hlay->addWidget(leftWid, 3);
	hlay->addWidget(m_antBtn, 1);
	vlay->addWidget(vipLabel);
	vlay->addWidget(timeLabel);

	// 底部
	QWidget* bottomWid = new QWidget(this);
	QHBoxLayout* bottomHlay = new QHBoxLayout(bottomWid);
	bottomHlay->setContentsMargins(0, 0, 0, 0);
	QStringList strList;
	strList << "签到" << "退出登录";
	QStringList objectNames, qssList;
	objectNames << "signInButton" << "logoutButton";
	qssList << StyleSheet::signInBtnQss() << StyleSheet::logoutBtnQss();

	for (int i = 0; i < strList.size(); ++i)
	{
		QPushButton* btn = new QPushButton(strList[i], bottomWid);
		btn->setFont(font);
		btn->setObjectName(objectNames[i]);		// 设置对象名
		btn->setMinimumWidth(70);
		btn->setMinimumHeight(25);				// 设置一个最小高度，让按钮看起来更舒服
		btn->setCursor(Qt::PointingHandCursor); // 设置鼠标悬停样式为手型
		btn->setStyleSheet(qssList[i]);
		bottomHlay->addWidget(btn);				// 将按钮添加到水平布局中

		// 在两个按钮中间插入 spacer
		if (i == 0)
		{
			bottomHlay->addStretch(); // 插入一个弹性间距，把第一个按钮和第二个按钮分开
		}

		// 连接信号槽
		connect(btn, &QPushButton::clicked, this, [this]()
			{
				QPushButton* btn = qobject_cast<QPushButton*>(sender());
				if (!btn) return;

				QString btnName = btn->objectName();

				if (btnName == "signInButton")
				{
					// 签到逻辑
					AntMessageManager::instance()->showMessage(AntMessage::Success, "签到成功,再接再厉");
				}
				else if (btnName == "logoutButton")
				{
					// 退出逻辑
					emit exitLogin(false);
				}
			});
	}

	// 主布局
	layout->addSpacing(8);
	layout->addWidget(nameLabel);
	layout->addWidget(emailLabel);
	layout->addSpacing(8);
	layout->addWidget(gradientWidget);
	layout->addSpacing(8);
	layout->addWidget(bottomWid);
	layout->addSpacing(14);
}

UserInfoBubble::~UserInfoBubble()
{
}

void UserInfoBubble::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing);
	p.setPen(Qt::NoPen);

	// 参数定义
	const int radius = 6;
	const int shadowWidth = 10;
	QRect rectBubble(arrowWidth, 0, width() - 5, height() - 5);
	QRectF r = rectBubble.adjusted(margin, margin, -margin, -margin);

	int ax = r.left();                  // 箭头连接处 X
	int ay = arrowOffsetY;             // 箭头顶部 Y
	int ah = arrowHeight;              // 箭头高度

	// ---------------- 构建连通路径 ----------------
	QPainterPath path;

	// 起点：左箭头顶部
	path.moveTo(ax, ay);
	path.lineTo(ax - arrowWidth, ay + ah / 2);
	path.lineTo(ax, ay + ah);           // 箭头底部

	// 左边直线到左下角圆角开始
	path.lineTo(ax, r.bottom() - radius);
	path.arcTo(QRectF(r.left(), r.bottom() - 2 * radius, 2 * radius, 2 * radius), 180, 90); // 左下角

	// 下边
	path.lineTo(r.right() - radius, r.bottom());
	path.arcTo(QRectF(r.right() - 2 * radius, r.bottom() - 2 * radius, 2 * radius, 2 * radius), 270, 90); // 右下角

	// 右边
	path.lineTo(r.right(), r.top() + radius);
	path.arcTo(QRectF(r.right() - 2 * radius, r.top(), 2 * radius, 2 * radius), 0, 90); // 右上角

	// 上边
	path.lineTo(r.left() + radius, r.top());
	path.arcTo(QRectF(r.left(), r.top(), 2 * radius, 2 * radius), 90, 90); // 左上角

	// 封闭路径：回到箭头顶部
	path.lineTo(ax, ay);

	// ---------------- 绘制气泡主体 ----------------
	p.setBrush(DesignSystem::instance()->currentTheme().userBubbleBgColor);
	p.drawPath(path);
}

void UserInfoBubble::leaveEvent(QEvent* e)
{
	QWidget::leaveEvent(e);

	QTimer::singleShot(100, this, [this]()
		{
			if (!this->underMouse())
			{
				emit requestHide(); // 通知外部隐藏，避免气泡自己控制自己
			}
		});
}