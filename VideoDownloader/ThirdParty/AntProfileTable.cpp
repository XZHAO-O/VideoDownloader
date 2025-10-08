#include "AntProfileTable.h"

AntProfileTable::AntProfileTable(int headerHeight, int rowsPerPage, int rowHeight, const TableColumnLayout& layout, QWidget* parent)
	: AntBaseTableView(parent),
	m_headerHeight(headerHeight),
	m_rowsPerPage(rowsPerPage),
	m_rowHeight(rowHeight),
	m_columnLayout(layout)
{
	setMouseTracking(true);
	setObjectName("AntProfileTable");

	// 定义表头
	verticalHeader()->setVisible(false);
	verticalHeader()->setHighlightSections(false);

	// 替换水平表头为自定义的
	customHeader = new AntBaseHeaderView(Qt::Horizontal, m_headerHeight, this);
	setHorizontalHeader(customHeader);

	// 禁止点击和拖动（如果需要）
	customHeader->setSectionsClickable(false);
	customHeader->setSectionsMovable(false);
	customHeader->setSectionResizeMode(QHeaderView::Fixed);

	// 禁用表格编辑
	setEditTriggers(QAbstractItemView::NoEditTriggers);

	// 设置每行固定高度
	this->verticalHeader()->setDefaultSectionSize(m_rowHeight);

	// 计算表格高度
	int idealHeight = m_headerHeight + m_rowsPerPage * m_rowHeight;

	// 设置宽度策略为 Expanding
	this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// 设置高度
	this->setFixedHeight(idealHeight);

	// 设置样式
	tableStyle = new AntProfileTableStyle(style());
	setStyle(tableStyle);

	// 设置项代理
	itemDelegate = new AntProfileItemDelegate(this, m_columnLayout, m_rowHeight, this);
	setItemDelegate(itemDelegate);

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			itemDelegate->updateStyle();
			customHeader->updateBaseStyle();
			update();
		});
}

AntProfileTable::~AntProfileTable()
{
}

QStandardItemModel* AntProfileTable::createModel(const QVector<TableColumnItems>& rowItems)
{
	// 创建 QStandardItemModel，6 列
	QStandardItemModel* model = new QStandardItemModel(rowItems.size(), 6, this);  // 6 列，行数与 rowItems 的大小相同

	// 设置列头
	model->setHorizontalHeaderLabels({ "头像", "名字", "年龄", "地址", "标签", "操作" });

	// 填充数据
	for (int row = 0; row < rowItems.size(); ++row)
	{
		const TableColumnItems& item = rowItems[row];

		// 头像列（使用 QPixmap）
		QStandardItem* avatarItem = new QStandardItem();
		avatarItem->setData(QVariant::fromValue(item.avatar), Qt::DecorationRole);  // 将 QPixmap 设置为装饰

		// 名字列（字符串）
		QStandardItem* nameItem = new QStandardItem(item.name);

		// 年龄列（字符串）
		QStandardItem* ageItem = new QStandardItem(item.age);

		// 地址列（字符串）
		QStandardItem* addressItem = new QStandardItem(item.address);

		// 标签列（QVariantList）
		QStandardItem* tagsItem = new QStandardItem();
		QVariantList tagList;
		for (const TagItem& tag : item.tags)
		{
			QVariantMap map;
			map["text"] = tag.text;
			map["color"] = tag.color;
			tagList.append(map);
		}
		tagsItem->setData(tagList, Qt::UserRole + 1);

		// 操作列（字符串）
		QStandardItem* actionItem = new QStandardItem(item.action);

		// 将行添加到模型
		model->setItem(row, 0, avatarItem);
		model->setItem(row, 1, nameItem);
		model->setItem(row, 2, ageItem);
		model->setItem(row, 3, addressItem);
		model->setItem(row, 4, tagsItem);
		model->setItem(row, 5, actionItem);
	}

	return model;
}

void AntProfileTable::mouseMoveEvent(QMouseEvent* event)
{
	// 获取视图的坐标系
	QPoint localPos = viewport()->mapFromGlobal(event->globalPosition().toPoint());

	// 获取鼠标指针在视图中的位置对应的索引
	QModelIndex hoveredIndex = indexAt(localPos);

	if (selectionBehavior() == QAbstractItemView::SelectRows)
	{
		itemDelegate->setCurrentHoverRow(hoveredIndex.row());
		update();
	}

	QTableView::mouseMoveEvent(event);
}

void AntProfileTable::leaveEvent(QEvent* event)
{
	if (selectionBehavior() == QAbstractItemView::SelectRows)
	{
		itemDelegate->setCurrentHoverRow(-1);
		update();
	}
	QTableView::leaveEvent(event);
}

void AntProfileTable::resizeEvent(QResizeEvent* event)
{
	QTableView::resizeEvent(event);
	updateColumnWidths();  // 每次表格尺寸变化时更新列宽
}

void AntProfileTable::updateColumnWidths()
{
	int total = this->viewport()->width();
	int flexible = total - m_columnLayout.avatarWidth - m_columnLayout.actionWidth;
	if (flexible < 0) flexible = 0;

	this->setColumnWidth(0, m_columnLayout.avatarWidth);
	this->setColumnWidth(5, m_columnLayout.actionWidth);
	this->setColumnWidth(1, int(flexible * m_columnLayout.nameRatio));
	this->setColumnWidth(2, int(flexible * m_columnLayout.ageRatio));
	this->setColumnWidth(3, int(flexible * m_columnLayout.addrRatio));
	this->setColumnWidth(4, int(flexible * m_columnLayout.tagRatio));
}