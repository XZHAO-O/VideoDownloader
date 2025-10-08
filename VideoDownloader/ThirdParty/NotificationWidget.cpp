#include "NotificationWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QToolButton>
#include "StyleSheet.h"
#include "AntButton.h"
#include "LoadingArc.h"
#include "DesignSystem.h"

int NotificationWidget::taskCount = 0;

NotificationWidget::NotificationWidget(const QString& title, QSize size, QWidget* parentWindow)
	: QWidget(parentWindow), m_size(size)
{
	// 透明+无边框
	setWindowFlags(Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	resize(m_size);

	// 背景控件
	bg = new QWidget(this);
	bg->setObjectName("Notification");
	bg->setStyleSheet(StyleSheet::notificationQss(DesignSystem::instance()->currentTheme().notifBgColor));

	// 外层 layout 负责撑满
	QVBoxLayout* outerLayout = new QVBoxLayout(this);
	outerLayout->setContentsMargins(15, 15, 15, 15);	// 给阴影留的四周间距
	outerLayout->setSpacing(0);
	outerLayout->addWidget(bg);

	// 内层 label 内容
	QVBoxLayout* contentLayout = new QVBoxLayout(bg);
	contentLayout->setContentsMargins(15, 15, 15, 15);
	contentLayout->setSpacing(20);
	QHBoxLayout* msgLay = new QHBoxLayout();
	// 字体
	QFont font;
	font.setPointSizeF(11.5);
	font.setBold(true);
	// 标题
	QHBoxLayout* titleLay = new QHBoxLayout();
	closeBtn = new QToolButton(bg);
	closeBtn->setIcon(QIcon(":/Imgs/Shut down-2.svg"));
	closeBtn->setStyleSheet(StyleSheet::toolBtnQss());
	QLabel* titleLab = new QLabel(title, bg);
	titleLab->setFont(font);
	titleLay->addWidget(titleLab);
	titleLay->addStretch();
	titleLay->addWidget(closeBtn);

	// 内容描述
	m_text = QString("当前有%1个任务正在后台执行中").arg(QString::number(taskCount));
	m_descLab = new QLabel(m_text, bg);
	LoadingArc* loadingArc = new LoadingArc(bg);
	loadingArc->setFixedSize(38, 38);
	loadingArc->start();
	msgLay->setContentsMargins(0, 0, 0, 0);
	msgLay->addWidget(m_descLab);
	msgLay->addSpacing(10);
	msgLay->addWidget(loadingArc);
	msgLay->addStretch();  // 右侧留白
	AntButton* btn = new AntButton("查看详情", 12, bg);
	btn->setFixedHeight(50);
	contentLayout->addStretch();
	contentLayout->addLayout(titleLay);
	contentLayout->addLayout(msgLay);
	contentLayout->addWidget(btn);
	contentLayout->addStretch();
	// 阴影
	auto* shadow = new QGraphicsDropShadowEffect(bg);
	shadow->setBlurRadius(30);
	shadow->setOffset(0, 0.5);
	shadow->setColor(QColor(50, 50, 50, 120));
	bg->setGraphicsEffect(shadow);

	// 信号槽
	connect(btn, &AntButton::clicked, this, [this]()
		{
			emit exitAnim();
		});

	connect(closeBtn, &QToolButton::clicked, this, [this]()
		{
			emit exitAnim();
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			bg->setStyleSheet(StyleSheet::notificationQss(DesignSystem::instance()->currentTheme().notifBgColor));
			closeBtn->setStyleSheet(StyleSheet::toolBtnQss());
		});
}

NotificationWidget::~NotificationWidget()
{
}

void NotificationWidget::animateIn(const QPoint& startPos, const QPoint& endPos)
{
	move(startPos);
	show();

	auto* anim = new QPropertyAnimation(this, "pos", this);
	anim->setDuration(500);
	anim->setStartValue(startPos);
	anim->setEndValue(endPos);
	anim->setEasingCurve(QEasingCurve::InOutCubic);
	anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void NotificationWidget::animateOut(const QPoint& endPos, std::function<void()> onFinished)
{
	auto* slide = new QPropertyAnimation(this, "pos", this);
	slide->setDuration(500);
	slide->setStartValue(pos());
	slide->setEndValue(endPos);  // 向右滑出到目标点
	slide->setEasingCurve(QEasingCurve::InOutCubic);
	slide->start(QAbstractAnimation::DeleteWhenStopped);

	connect(slide, &QPropertyAnimation::finished, this, [this, onFinished]()
		{
			if (onFinished) onFinished();
		});
}

void NotificationWidget::addTaskCount()
{
	taskCount++;
	m_text = QString("当前有%1个任务正在后台执行中").arg(QString::number(taskCount));
	m_descLab->setText(m_text);
}