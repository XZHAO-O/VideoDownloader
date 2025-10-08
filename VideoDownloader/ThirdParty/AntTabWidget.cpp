#include "AntTabWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QEnterEvent>
#include "DesignSystem.h"

AntTabWidget::AntTabWidget(const QString& title, const QString& icon, int tabwidth, int tabHeight, QWidget* parent)
	: QWidget(parent), m_title(title), m_icon(icon), m_tabWidth(tabwidth), m_tabHeight(tabHeight)
{
	resize(m_tabWidth, m_tabHeight);
	m_svgRenderer = new QSvgRenderer(m_icon, this);  // 使用 QSvgRenderer 加载图标

	hoverBgColor = DesignSystem::instance()->currentTheme().tabHoverColor;
	textColor = DesignSystem::instance()->currentTheme().tabTextColor;

	// 关闭按钮
	QIcon closeBtnIcon = DesignSystem::instance()->themeMode() == DesignSystem::ThemeMode::Light ?
		QIcon(":/Imgs/closeItem.svg") : QIcon(":/Imgs/closeItemDark.svg");
	m_closeButton = new QPushButton(this);
	m_closeButton->setIcon(closeBtnIcon);
	m_closeButton->setIconSize(QSize(14, 14)); // 图标大小可按需调整
	m_closeButton->setFixedSize(20, 20);       // 按钮整体大小
	m_closeButton->raise();

	QString hoverColor = DesignSystem::instance()->currentTheme().onTabBtnHoverColor.name();
	m_closeButton->setStyleSheet(QString(R"(
			QPushButton {
				border: none;
				background-color: transparent;
				padding: 2px;
			}
			QPushButton:hover {
				background-color: %1;
				border-radius: 4px;
			}
		)").arg(hoverColor));

	connect(m_closeButton, &QPushButton::clicked, this, [this]()
		{
			m_closeButton->hide();
			emit requestClose(this);  // 通知父组件
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			QIcon closeBtnIcon = DesignSystem::instance()->themeMode() == DesignSystem::ThemeMode::Light ?
				QIcon(":/Imgs/closeItem.svg") : QIcon(":/Imgs/closeItemDark.svg");
			m_closeButton->setIcon(closeBtnIcon);
			m_closeButton->setStyleSheet(QString(R"(
			QPushButton {
				border: none;
				background-color: transparent;
				padding: 2px;
			}
			QPushButton:hover {
				background-color: %1;
				border-radius: 4px;
			}
			)").arg(DesignSystem::instance()->currentTheme().onTabBtnHoverColor.name()));

			hoverBgColor = DesignSystem::instance()->currentTheme().tabHoverColor;
			textColor = DesignSystem::instance()->currentTheme().tabTextColor;
			update();
		});
}

QString AntTabWidget::title() const
{
	return m_title;
}

void AntTabWidget::setSelected(bool selected)
{
	m_selected = selected;
	update();
}

bool AntTabWidget::isSelected() const
{
	return m_selected;
}

// Getter 方法
int AntTabWidget::tabWidth() const
{
	return m_tabWidth;
}

// Setter 方法
void AntTabWidget::setTabWidth(int width)
{
	m_tabWidth = width;
	update();  // 更新界面以反映更改
}

// Getter 方法
int AntTabWidget::tabHeight() const
{
	return m_tabHeight;
}

// Setter 方法
void AntTabWidget::setTabHeight(int height)
{
	m_tabHeight = height;
	update();  // 更新界面以反映更改
}

void AntTabWidget::enterEvent(QEnterEvent* /*event*/)
{
	m_hovered = true;
	update();
}

void AntTabWidget::leaveEvent(QEvent* /*event*/)
{
	m_hovered = false;
	update();
}

void AntTabWidget::paintEvent(QPaintEvent* /*event*/)
{
	QPainter p(this);
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

	// 加载SVG图标
	int topOffset = 8;

	if (m_selected) {
		// 当前选中标签：顶部圆角样式
		p.setBrush(DesignSystem::instance()->currentTheme().backgroundColor);
		p.setPen(Qt::NoPen);

		QPainterPath path;
		int radius = 6;
		QRect r = rect().adjusted(0, topOffset, 0, 2);

		path.moveTo(r.bottomLeft());
		path.lineTo(r.topLeft() + QPoint(0, radius));
		path.quadTo(r.topLeft(), r.topLeft() + QPoint(radius, 0));
		path.lineTo(r.topRight() - QPoint(radius, 0));
		path.quadTo(r.topRight(), r.topRight() + QPoint(0, radius));
		path.lineTo(r.bottomRight());
		path.closeSubpath();

		p.drawPath(path);
	}
	else if (m_hovered) {
		// 悬停时
		p.setBrush(hoverBgColor);
		p.setPen(Qt::NoPen);
		p.drawRoundedRect(rect().adjusted(3, topOffset + 3, -3, -3), 8, 8);
	}
	else {
		// 非选中非悬停：无边框背景
		p.setBrush(Qt::transparent);
		p.setPen(Qt::NoPen);
		p.drawRect(rect());
	}

	// 绘制图标
	QRect iconRect = QRect(rect().left() + 10, m_iconTopOffset + rect().top() + (rect().height() - m_iconSize) / 2, m_iconSize, m_iconSize);
	if (m_svgRenderer->isValid())
	{
		m_svgRenderer->render(&p, iconRect);  // 使用 QPainter 渲染 SVG 图标
	}

	// 文本
	QRect textRect = rect().adjusted(0, topOffset, 0, 2);
	textRect.setLeft(iconRect.right() + 15);  // 确保文本不会与图标重叠
	p.setPen(textColor);
	p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_title);
}

void AntTabWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event); // 保留原有处理

	int marginRight = 10;
	int x = width() - m_closeButton->width() - marginRight;
	int y = m_iconTopOffset + (height() - m_closeButton->height()) / 2;
	m_closeButton->move(x, y);
}