#include "AntTabBarWidget.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QResizeEvent>
#include "StyleSheet.h"
#include "DesignSystem.h"

AntTabBarWidget::AntTabBarWidget(int tabwidth, int tabHeight, bool isEnableAddBtn, QWidget* parent)
	: QWidget(parent), m_isEnableAddBtn(isEnableAddBtn), m_tabWidth(tabwidth), m_tabHeight(tabHeight)
{
	setFixedHeight(tabHeight);
	setObjectName("AntTabBarWidget");
	setStyleSheet(StyleSheet::antTabBarWidgetQss(DesignSystem::instance()->currentTheme().tabBarBgColor));

	if (m_isEnableAddBtn)
	{
		m_addButton = new QPushButton("+", this);
		m_addButton->setFixedSize(25, 25);
		m_addButton->setStyleSheet(QString(R"(
            QPushButton {
                border: none;
                background-color: transparent;
                font-weight: bold;
                font-size: 22px;
            }
            QPushButton:hover {
                background-color: %1;
                border-radius: 4px;
            }
        )").arg(DesignSystem::instance()->widgetHoverBgColor().name()));
		connect(m_addButton, &QPushButton::clicked, this, [=]()
			{
				// 业务逻辑
				emit addTabAndContent(QString("Tab %1").arg(m_tabs.size()));
			});
	}

	connect(this, &AntTabBarWidget::currentIndexChanged, this, [this](int currentIndex)
		{
			for (int i = 0; i < m_tabs.size(); ++i)
			{
				if (i == currentIndex)
				{
					m_tabs[i]->setSelected(true);
				}
				else
				{
					m_tabs[i]->setSelected(false);
				}
			}
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			if (m_addButton)
			{
				m_addButton->setStyleSheet(QString(R"(
				QPushButton {
					border: none;
					background-color: transparent;
					font-weight: bold;
					font-size: 22px;
				}
				QPushButton:hover {
					background-color: %1;
					border-radius: 4px;
				}
				)").arg(DesignSystem::instance()->widgetHoverBgColor().name()));
			}
			setStyleSheet(StyleSheet::antTabBarWidgetQss(DesignSystem::instance()->currentTheme().tabBarBgColor));
			update();
		});
}

void AntTabBarWidget::addTab(const QString& title, const QString& icon)
{
	AntTabWidget* tab = new AntTabWidget(title, icon, m_tabWidth, m_tabHeight, this);
	m_tabs.append(tab);
	for (int i = 0; i < m_tabs.size(); ++i)
	{
		if (m_tabs[i] == tab)
		{
			m_tabs[i]->setSelected(true);
		}
		else
		{
			m_tabs[i]->setSelected(false);
		}
	}

	m_currentIndex = m_tabs.size() - 1;
	int tabX = m_currentIndex * (tab->tabWidth() + m_spacing) + m_startX;
	int tabY = 0;

	// 初始状态：宽度为 0，高度正常
	QRect startRect(tabX, tabY, 0, tab->tabHeight());

	// 最终状态：宽度展开到目标
	QRect endRect(tabX, tabY, tab->tabWidth(), tab->tabHeight());

	tab->setGeometry(startRect); // 初始位置 & 尺寸
	tab->show();

	QPropertyAnimation* anim = new QPropertyAnimation(tab, "geometry", this);
	anim->setStartValue(startRect);
	anim->setEndValue(endRect);
	anim->setDuration(300);
	anim->setEasingCurve(QEasingCurve::OutCubic);
	anim->start(QAbstractAnimation::DeleteWhenStopped);

	// 平滑移动加号按钮
	updateAddButtonPositionAnimated();

	// 关闭逻辑
	connect(tab, &AntTabWidget::requestClose, this, [this](AntTabWidget* tab)
		{
			int index = m_tabs.indexOf(tab);
			if (index == -1)
				return;

			QRect startGeometry = tab->geometry();
			QRect endGeometry = QRect(startGeometry.topLeft(), QSize(0, startGeometry.height()));

			QPropertyAnimation* closeAnim = new QPropertyAnimation(tab, "geometry");
			closeAnim->setDuration(300);
			closeAnim->setEasingCurve(QEasingCurve::OutCubic);
			closeAnim->setStartValue(startGeometry);
			closeAnim->setEndValue(endGeometry);

			connect(closeAnim, &QPropertyAnimation::finished, this, [this, tab]()
				{
					int idx = m_tabs.indexOf(tab);
					if (idx != -1)
						m_tabs.removeAt(idx);

					tab->deleteLater();

					// 更新当前索引
					if (m_currentIndex >= m_tabs.size())
						m_currentIndex = m_tabs.size() - 1;

					emit currentIndexChanged(m_currentIndex);
					emit removeContent(idx, m_currentIndex);
					updateTabPositions();
					updateAddButtonPositionAnimated();
				});

			closeAnim->start(QAbstractAnimation::DeleteWhenStopped);
		});
}

int AntTabBarWidget::currentIndex() const
{
	return m_currentIndex;
}

void AntTabBarWidget::setCurrentIndex(int index)
{
	if (index >= 0 && index < m_tabs.size() && index != m_currentIndex)
	{
		m_currentIndex = index;
		emit currentIndexChanged(m_currentIndex);  // 发射信号
	}
}

void AntTabBarWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);

	// 设置背景色为灰色
	painter.setBrush(DesignSystem::instance()->widgetBgColor());
	painter.setPen(Qt::NoPen);
	painter.drawRect(rect());
}

void AntTabBarWidget::resizeEvent(QResizeEvent* /*event*/)
{
	updateTabPositions();
}

void AntTabBarWidget::mousePressEvent(QMouseEvent* event)
{
	// 检测是否点中了某个 tab
	for (int i = m_tabs.size() - 1; i >= 0; --i)
	{
		AntTabWidget* tab = m_tabs[i];
		tab->setSelected(false);    // 其他 tab
		QRect geo = tab->geometry();
		if (geo.contains(event->pos()))
		{
			tab->setSelected(true);		// 当前 tab
			emit tabClicked(i);			// 发射 tabClicked 信号
			m_currentIndex = i;
			m_draggingTab = tab;
			m_draggingIndex = i;
			m_dragOffset = event->pos() - geo.topLeft();
			m_dragStartPos = tab->pos();
			raiseTab(tab);
		}
	}
}

void AntTabBarWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (!m_draggingTab)
		return;

	QPoint newPos = event->pos() - m_dragOffset;
	// 限制纵向位置
	newPos.setY(0);
	m_draggingTab->move(newPos);

	// 计算当前拖动位置对应的新序号
	int newIndex = qBound(0, (newPos.x() + m_draggingTab->width() / 2) / (m_draggingTab->width() + m_spacing), m_tabs.size() - 1);
	if (newIndex != m_draggingIndex) {
		// 改变顺序
		m_tabs.move(m_draggingIndex, newIndex);
		m_draggingIndex = newIndex;
		updateTabPositions(m_draggingTab);
	}

	// 拖动最后一个 tab 向右时，让加号按钮一起跟随移动
	if (m_isEnableAddBtn && m_draggingTab && m_draggingIndex == m_tabs.size() - 1)
	{
		// 只在拖动向右时才跟随移动
		if (newPos.x() > m_dragStartPos.x())
		{
			int tabRight = m_draggingTab->geometry().right();
			int buttonX = tabRight + m_spacing;
			int buttonY = m_startY + (height() - m_addButton->height()) / 2;

			// 判断拖动的 tab 是否已到达最后一个槽位
			int lastTabRight = m_startX + (m_tabs.size() - 1) * (m_draggingTab->width() + m_spacing);
			if (newPos.x() >= lastTabRight)
			{
				// 只有当拖动的 tab 已到达最后一个槽位时，才让加号按钮跟随
				m_addButton->move(buttonX, buttonY);
			}
		}
	}
}

void AntTabBarWidget::mouseReleaseEvent(QMouseEvent* /*event*/)
{
	if (!m_draggingTab)
		return;

	// 拖动结束，动画回到目标位置
	animateTabToPosition(m_draggingTab, m_draggingIndex);

	m_draggingTab = nullptr;
	m_draggingIndex = -1;

	if (m_isEnableAddBtn)
	{
		// 重新定位加号按钮位置
		int lastTabX = m_tabs.size() * (m_tabs[0]->width() + m_spacing) + m_startX;
		int buttonY = m_startY + (height() - m_addButton->height()) / 2;
		QPoint addBtnTargetPos(lastTabX, buttonY);

		QPropertyAnimation* addAnim = new QPropertyAnimation(m_addButton, "pos");
		addAnim->setDuration(200);
		addAnim->setEasingCurve(QEasingCurve::OutCubic);
		addAnim->setStartValue(m_addButton->pos());
		addAnim->setEndValue(addBtnTargetPos);
		addAnim->start(QAbstractAnimation::DeleteWhenStopped);
	}
}

void AntTabBarWidget::updateTabPositions(AntTabWidget* excludeDragTab)
{
	for (int i = 0; i < m_tabs.size(); ++i)
	{
		AntTabWidget* tab = m_tabs[i];
		if (tab == excludeDragTab)
			continue;

		animateTabToPosition(tab, i);
	}
}

void AntTabBarWidget::animateTabToPosition(AntTabWidget* tab, int index)
{
	QPoint targetPos(index * (tab->width() + m_spacing) + m_startX, 0);

	if (tab->pos() == targetPos)
		return;  // 已在目标位置，无需动画

	// 用动画平滑移动
	QPropertyAnimation* animation = new QPropertyAnimation(tab, "pos");
	animation->setDuration(200);
	animation->setEasingCurve(QEasingCurve::OutCubic);
	animation->setStartValue(tab->pos());
	animation->setEndValue(targetPos);
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void AntTabBarWidget::raiseTab(AntTabWidget* tab)
{
	tab->raise();
}

void AntTabBarWidget::updateAddButtonPositionAnimated()
{
	if (!m_isEnableAddBtn || !m_addButton)
		return;

	int lastTabX = m_tabs.size() * (m_tabWidth + m_spacing) + m_startX;
	int buttonY = m_startY + (height() - m_addButton->height()) / 2;
	QPoint addBtnTargetPos(lastTabX, buttonY);

	QPropertyAnimation* addAnim = new QPropertyAnimation(m_addButton, "pos");
	addAnim->setDuration(200);
	addAnim->setEasingCurve(QEasingCurve::OutCubic);
	addAnim->setStartValue(m_addButton->pos());
	addAnim->setEndValue(addBtnTargetPos);
	addAnim->start(QAbstractAnimation::DeleteWhenStopped);
}