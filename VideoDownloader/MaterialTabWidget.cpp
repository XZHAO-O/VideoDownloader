#include "MaterialTabWidget.h"
#include "MaterialTabBar.h"
#include "SlideStackedWidget.h"
#include <QPainter>
#include "StyleSheet.h"
#include <QVBoxLayout>
#include "DesignSystem.h"
#include <QWheelEvent>
#include <QDateTime>

MaterialTabWidget::MaterialTabWidget(QWidget* parent)
	: QWidget(parent)
{
	m_tabBar = new MaterialTabBar(this);
	m_stackedWidget = new SlideStackedWidget(this);

	layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(m_tabBar);
	layout->addWidget(m_stackedWidget);

	connect(m_tabBar, &QTabBar::currentChanged, this, &MaterialTabWidget::onTabClicked);

	// 快速滚动初始化
	m_wheelTimer = new QTimer(this);
	m_wheelTimer->setSingleShot(true);
	m_wheelTimer->setInterval(120);

	connect(m_wheelTimer, &QTimer::timeout, this, &MaterialTabWidget::handleWheelChange);

	// 只给标签栏安装事件过滤器
	m_tabBar->installEventFilter(this);

	m_currentIndex = 0;

	// 设置焦点策略
	setFocusPolicy(Qt::StrongFocus);
	m_tabBar->setFocusPolicy(Qt::StrongFocus);
	m_stackedWidget->setFocusPolicy(Qt::StrongFocus);
}

MaterialTabWidget::~MaterialTabWidget()
{
}

void MaterialTabWidget::setCurrentIndex(int index)
{
	if (index < 0 || index >= m_stackedWidget->count())
		return;

	// 取消待处理的滚轮切换
	m_wheelTimer->stop();
	m_targetIndex = -1;
	m_rapidScrolling = false;

	m_tabBar->setCurrentIndex(index);
	m_stackedWidget->setCurrentIndex(index);
	m_currentIndex = index;
	emit itemIndexChanged(index);
}

void MaterialTabWidget::onTabClicked(int index)
{
	if (m_isAnimating || index == m_currentIndex)
		return;

	// 取消快速滚动状态
	m_wheelTimer->stop();
	m_targetIndex = -1;
	m_rapidScrolling = false;

	bool forward = index > m_currentIndex;

	QWidget* nextWidget = m_stackedWidget->widget(index);

	m_isAnimating = true;

	m_stackedWidget->slideToPage(nextWidget,
		forward ? SlideStackedWidget::RightToLeft : SlideStackedWidget::LeftToRight,
		m_animationDuration, SlideStackedWidget::OutCubic, [this, index]()
		{
			m_currentIndex = index;
			m_tabBar->setCurrentIndex(index);
			m_isAnimating = false;
			emit itemIndexChanged(index);
		});
}

void MaterialTabWidget::addTab(QWidget* wid, QString tabName)
{
	m_stackedWidget->addWidget(wid);
	m_tabBar->addTab(tabName);
}

QWidget* MaterialTabWidget::getWidget(int index)
{
	return m_stackedWidget->widget(index);
}

QVBoxLayout* MaterialTabWidget::getLayout()
{
	return layout;
}

bool MaterialTabWidget::eventFilter(QObject* obj, QEvent* event)
{
	// 只处理标签栏的滚轮事件
	if (obj == m_tabBar && event->type() == QEvent::Wheel) {
		QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
		handleWheelEvent(wheelEvent);
		return true;
	}
	return QWidget::eventFilter(obj, event);
}

void MaterialTabWidget::wheelEvent(QWheelEvent* event)
{
	// 完全忽略滚轮事件，让子组件处理
	event->ignore();
}

void MaterialTabWidget::handleWheelEvent(QWheelEvent* event)
{
	if (m_isAnimating) {
		// 动画期间完全忽略滚轮事件
		event->accept();
		return;
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
	int delta = event->angleDelta().y();

	// 时间防抖：如果距离上次滚轮时间很近，累积delta
	if (currentTime - m_lastWheelTime < 80) {
		m_wheelAccumulator += delta;
		m_rapidScrolling = true;
	}
	else {
		m_wheelAccumulator = delta;
		m_rapidScrolling = false;
	}

	m_lastWheelTime = currentTime;

	// 计算方向
	int direction = 0;
	if (m_wheelAccumulator > 40) {
		direction = -1;
		m_wheelAccumulator = 0;
	}
	else if (m_wheelAccumulator < -40) {
		direction = 1;
		m_wheelAccumulator = 0;
	}
	else {
		event->accept();
		m_wheelTimer->start();
		return;
	}

	// 计算目标索引
	int newIndex = m_currentIndex + direction;
	newIndex = qMax(0, qMin(m_tabBar->count() - 1, newIndex));

	if (newIndex != m_currentIndex) {
		if (m_rapidScrolling) {
			m_targetIndex = newIndex;
			jumpToTargetIndex();
		}
		else {
			m_targetIndex = newIndex;
			m_wheelTimer->start();
		}
	}

	event->accept();
}

void MaterialTabWidget::handleWheelChange()
{
	if (m_targetIndex != -1 && !m_isAnimating && m_targetIndex != m_currentIndex) {
		if (m_rapidScrolling) {
			jumpToTargetIndex();
		}
		else {
			setCurrentIndex(m_targetIndex);
		}
		m_targetIndex = -1;
		m_rapidScrolling = false;
	}
}

void MaterialTabWidget::jumpToTargetIndex()
{
	if (m_targetIndex != -1 && !m_isAnimating && m_targetIndex != m_currentIndex) {
		m_currentIndex = m_targetIndex;
		m_tabBar->setCurrentIndex(m_targetIndex);
		m_stackedWidget->setCurrentIndex(m_targetIndex);
		emit itemIndexChanged(m_targetIndex);
		m_targetIndex = -1;
	}
}

int MaterialTabWidget::count() const
{
	// 返回标签页的数量，假设 m_stackedWidget 和 m_tabBar 的数量是同步的
	if (m_stackedWidget) {
		return m_stackedWidget->count();
	}
	return 0;
}

void MaterialTabWidget::removeTab(int index)
{
	if (index < 0 || index >= count()) {
		return;
	}

	// 从 stacked widget 中移除页面
	QWidget* widget = m_stackedWidget->widget(index);
	if (widget) {
		m_stackedWidget->removeWidget(widget);
		widget->deleteLater();
	}

	// 从 tab bar 中移除标签
	m_tabBar->removeTab(index);

	// 如果移除了当前标签页，需要更新当前索引
	if (m_currentIndex >= index && m_currentIndex > 0) {
		m_currentIndex--;
	}

	// 如果没有标签页了，重置当前索引
	if (count() == 0) {
		m_currentIndex = 0;
	}
}