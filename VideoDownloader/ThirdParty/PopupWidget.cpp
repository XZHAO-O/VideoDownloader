#include "PopupWidget.h"
#include "StyleSheet.h"
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <QScrollBar>

PopupWidget::PopupWidget(int maxHeight, bool enableMultiLevel, QWidget* parent)
	: QListView(parent),
	m_enableMultiLevel(enableMultiLevel),
	m_maxHeight(maxHeight)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	auto theme = DesignSystem::instance()->currentTheme();
	setStyleSheet(StyleSheet::popupListViewQss(theme.popupScrollBarColor));
	m_style = new ListViewStyle(theme.popupBgColor, theme.popupScrollBarColor, theme.shadowColor, style());
	setStyle(m_style);

	// 自定义 Item Delegate
	m_itmeDele = new ListItemDelegate(36, enableMultiLevel, this);
	setItemDelegate(m_itmeDele);

	QFont font = this->font();
	font.setPointSize(11);
	setFont(font);

	connect(this, &QListView::clicked, this, [this, enableMultiLevel](const QModelIndex& idx)
		{
			emit itemSelected(idx);
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			auto theme = DesignSystem::instance()->currentTheme();
			setStyleSheet(StyleSheet::popupListViewQss(theme.popupScrollBarColor));
			m_style->updateStyle(theme.popupBgColor, theme.popupScrollBarColor, theme.shadowColor);
			m_itmeDele->updateStyle(theme.primaryColor, theme.popupItemBgColor, theme.popupTextColor);
			update();
		});
}

int PopupWidget::calculateAdaptiveHeight() const
{
	if (!model()) {
		return 0;
	}

	int itemCount = model()->rowCount();
	if (itemCount == 0) {
		return 0;
	}

	// 计算所有项的总高度
	int totalItemsHeight = 0;
	for (int i = 0; i < itemCount; ++i) {
		QSize itemSize = sizeHintForIndex(model()->index(i, 0));
		totalItemsHeight += itemSize.height();
	}

	// 考虑边框、内边距和滚动条
	int frameAndPadding = 16; // 增加一些边距

	// 如果总高度小于最大高度，则使用总高度
	int adaptiveHeight = totalItemsHeight + frameAndPadding;

	// 如果自适应高度小于最大高度，确保不会显示滚动条
	if (adaptiveHeight < m_maxHeight) {
		// 再增加一点高度确保完全显示所有项
		adaptiveHeight += 4;
	}

	// 返回自适应高度和最大高度的较小值
	return qMin(adaptiveHeight, m_maxHeight);
}

void PopupWidget::setFixedSizeWithAdaptiveHeight(int width, int maxHeight)
{
	// 计算自适应高度
	int adaptiveHeight = calculateAdaptiveHeight();

	// 设置固定尺寸
	setFixedSize(width, adaptiveHeight);

	// 更新视口尺寸，确保内容正确显示
	updateGeometry();

	// 如果自适应高度小于最大高度，禁用垂直滚动条
	if (adaptiveHeight < maxHeight) {
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
	else {
		setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	}
}

void PopupWidget::setCurrentIndex(const QModelIndex& index)
{
	if (index.isValid())
	{
		QListView::setCurrentIndex(index);
	}
}