#include "PaginationWidget.h"
#include "StyleSheet.h"
#include "DesignSystem.h"
#include <QLabel>

PaginationWidget::PaginationWidget(QSize buttonSize, QWidget* parent)
	: QWidget(parent), m_totalPages(1), m_currentPage(1), btnSize(buttonSize)
{
	setObjectName("PaginationWidget");

	m_layout = new QHBoxLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSpacing(10);

	auto* ins = DesignSystem::instance();
	m_prevButton = createNavButton(ins->prevBtnIcon(), ins->prevBtnDisableIcon());
	connect(m_prevButton, &QPushButton::clicked, this, [this]()
		{
			if (m_currentPage > 1)
				setCurrentPage(m_currentPage - 1);
		});

	m_nextButton = createNavButton(ins->nextBtnIcon(), ins->nextBtnDisableIcon());
	connect(m_nextButton, &QPushButton::clicked, this, [this]()
		{
			if (m_currentPage < m_totalPages)
				setCurrentPage(m_currentPage + 1);
		});

	// 先插入prev和next，中间动态插入按钮
	m_layout->addWidget(m_prevButton);
	m_layout->addWidget(m_nextButton);

	refreshButtons();

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			auto theme = DesignSystem::instance()->currentTheme();
			for (int i = 0; i < m_layout->count(); ++i)
			{
				QLayoutItem* item = m_layout->itemAt(i);
				QWidget* widget = item->widget();
				if (auto btn = qobject_cast<QPushButton*>(widget))
				{
					// 更新按钮样式
					btn->setStyleSheet(StyleSheet::paginationWidgetQss(theme.primaryColor, theme.widgetHoverBgColor));

					if (i == 0)
					{
						// 后退箭头
						updateBtnIcon(btn, DesignSystem::instance()->prevBtnIcon(), DesignSystem::instance()->prevBtnDisableIcon());
					}
					else if (i == m_layout->count() - 1)
					{
						// 前进箭头
						updateBtnIcon(btn, DesignSystem::instance()->nextBtnIcon(), DesignSystem::instance()->nextBtnDisableIcon());
					}
				}
			}
		});
}

void PaginationWidget::updateBtnIcon(QPushButton* btn, const QString& iconPathNormal, const QString& iconPathDisabled)
{
	auto theme = DesignSystem::instance()->currentTheme();
	btn->setStyleSheet(StyleSheet::paginationWidgetQss(theme.primaryColor, theme.widgetHoverBgColor));
	QIcon icon;
	icon.addFile(iconPathNormal, QSize(), QIcon::Normal);
	icon.addFile(iconPathDisabled, QSize(), QIcon::Disabled);
	btn->setIcon(icon);
}

QPushButton* PaginationWidget::createNavButton(const QString& iconPathNormal, const QString& iconPathDisabled)
{
	auto theme = DesignSystem::instance()->currentTheme();
	QPushButton* btn = new QPushButton(this);
	btn->setFixedSize(btnSize);
	btn->setCursor(Qt::PointingHandCursor);

	QIcon icon;
	icon.addFile(iconPathNormal, QSize(), QIcon::Normal);
	icon.addFile(iconPathDisabled, QSize(), QIcon::Disabled);
	btn->setIcon(icon);

	btn->setIconSize(QSize(btnSize.width() - 17, btnSize.height() - 17));
	btn->setStyleSheet(StyleSheet::paginationWidgetQss(theme.primaryColor, theme.widgetHoverBgColor));
	return btn;
}

void PaginationWidget::setTotalPages(int pages)
{
	m_totalPages = qMax(1, pages);
	m_currentPage = qMin(m_currentPage, m_totalPages);
	refreshButtons();
}

void PaginationWidget::setCurrentPage(int page)
{
	if (page < 1 || page > m_totalPages || page == m_currentPage)
		return;
	m_currentPage = page;
	refreshButtons();
	emit currentPageChanged(m_currentPage);
}

void PaginationWidget::refreshButtons()
{
	// 从后往前删除，跳过第0个和最后一个
	while (m_layout->count() > 2)
	{
		QLayoutItem* item = m_layout->takeAt(1);
		if (item)
		{
			if (QWidget* w = item->widget())
			{
				w->deleteLater();
			}
			delete item;
		}
	}

	// 按钮插入函数
	auto insertPageButton = [&](int page)
		{
			QPushButton* btn = new QPushButton(QString::number(page), this);
			auto theme = DesignSystem::instance()->currentTheme();
			btn->setFixedSize(btnSize);
			btn->setCursor(Qt::PointingHandCursor);
			btn->setStyleSheet(StyleSheet::paginationWidgetQss(theme.primaryColor, theme.widgetHoverBgColor));
			btn->setCheckable(true);
			btn->setFocusPolicy(Qt::NoFocus);
			btn->setChecked(page == m_currentPage);

			connect(btn, &QPushButton::clicked, this, [this, page, btn]()
				{
					if (page == m_currentPage)
					{
						// 禁止重复点击导致取消选中，强制恢复 checked 状态
						btn->setChecked(true);
						return;
					}
					setCurrentPage(page);
				});

			m_layout->insertWidget(m_layout->count() - 1, btn);
		};

	// 省略号
	auto insertEllipsis = [&]()
		{
			QLabel* label = new QLabel("...", this);
			label->setFixedSize(btnSize);
			label->setAlignment(Qt::AlignCenter);
			m_layout->insertWidget(m_layout->count() - 1, label);
		};

	if (m_totalPages <= 7)
	{
		// 总页数不多，直接显示所有页码
		for (int i = 1; i <= m_totalPages; ++i)
		{
			insertPageButton(i);
		}
	}
	else
	{
		// 总页数多，复杂情况
		insertPageButton(1);  // 首页按钮

		if (m_currentPage > 4)
			insertEllipsis();

		int start = qMax(2, m_currentPage - 2);
		int end = qMin(m_totalPages - 1, m_currentPage + 2);

		// 调整区间，保证显示5个页码
		if (m_currentPage <= 4)
		{
			start = 2;
			end = 5;
		}
		else if (m_currentPage >= m_totalPages - 3)
		{
			start = m_totalPages - 4;
			end = m_totalPages - 1;
		}

		for (int i = start; i <= end; ++i)
		{
			insertPageButton(i);
		}

		if (m_currentPage < m_totalPages - 3)
			insertEllipsis();

		insertPageButton(m_totalPages);  // 尾页按钮
	}

	// 更新上一页下一页按钮状态
	m_prevButton->setEnabled(m_currentPage > 1);
	m_nextButton->setEnabled(m_currentPage < m_totalPages);
}