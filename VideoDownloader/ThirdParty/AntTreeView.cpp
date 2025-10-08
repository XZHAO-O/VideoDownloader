#include "AntTreeView.h"

AntTreeView::AntTreeView(int itemHeight, QWidget* parent)
	: QTreeView(parent), m_arrowRotation(0), m_itemHeight(itemHeight)
{
	setAnimated(true);
	setMouseTracking(true);
	setHeaderHidden(true);
	setExpandsOnDoubleClick(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setFrameShape(QFrame::NoFrame);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setIndentation(0);	// 缩进

	m_delegate = new AntTreeItemDelegate(itemHeight, 26, 40, 60, this);
	setItemDelegate(m_delegate);

	// 初始化动画
	m_animation.setDuration(250);
	m_animation.setEasingCurve(QEasingCurve::OutCubic);
	connect(&m_animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
		{
			if (!m_currentAnimationIndex.isValid())
				return;

			m_arrowRotations[m_currentAnimationIndex] = value.toInt();
			m_delegate->setRotationMap(m_arrowRotations);
			viewport()->update(visualRect(m_currentAnimationIndex)); // 只刷新当前项
		});

	connect(DesignSystem::instance(), &DesignSystem::themeChanged, this, [this]()
		{
			m_delegate->updateStyle();
			update();
		});
}

AntTreeView::~AntTreeView()
{
}

QStandardItemModel* AntTreeView::createModel(const QList<ItemData>& data)
{
	auto* model = new QStandardItemModel(this);
	model->setColumnCount(1);

	// 递归构建函数（定义在 createModel 内部）
	std::function<QStandardItem* (const ItemData&)> buildItem = [&](const ItemData& itemData) -> QStandardItem*
		{
			QStandardItem* item = new QStandardItem(itemData.icon, itemData.text);
			for (const auto& child : itemData.children)
			{
				item->appendRow(buildItem(child)); // 递归添加子节点
			}
			return item;
		};

	// 构建顶层节点
	for (const auto& rootItemData : data)
	{
		model->appendRow(buildItem(rootItemData));
	}

	return model;
}

void AntTreeView::calculateFullHeight()
{
	if (!model())
		return;

	int totalCount = 0;

	std::function<void(const QModelIndex&)> countAll = [&](const QModelIndex& parent)
		{
			int rows = model()->rowCount(parent);
			for (int i = 0; i < rows; ++i)
			{
				QModelIndex idx = model()->index(i, 0, parent);
				totalCount++;          // 统计所有节点（无视展开状态）
				countAll(idx);         // 递归统计所有子节点
			}
		};

	countAll(QModelIndex());

	int totalHeight = totalCount * m_itemHeight;
	setFixedHeight(totalHeight);
}

bool AntTreeView::viewportEvent(QEvent* event)
{
	if (event->type() == QEvent::MouseButtonPress)
	{
		auto* mouseEvent = static_cast<QMouseEvent*>(event);
		QModelIndex index = indexAt(mouseEvent->pos());

		if (index.isValid())
		{
			bool expanded = isExpanded(index);
			startArrowAnimation(index, expanded ? 0 : 90);

			if (expanded)
				collapse(index);
			else
				expand(index);

			return true; // 事件处理完毕
		}
	}
	return QTreeView::viewportEvent(event);
}

void AntTreeView::mouseMoveEvent(QMouseEvent* event)
{
	QModelIndex index = indexAt(event->pos());
	if (index != m_delegate->hoverIndex)
	{
		m_delegate->hoverIndex = index;
		viewport()->update();  // 触发重绘，代理的paint会用到hoverIndex
	}
	QTreeView::mouseMoveEvent(event);
}

void AntTreeView::leaveEvent(QEvent* event)
{
	if (m_delegate->hoverIndex.isValid())
	{
		m_delegate->hoverIndex = QModelIndex();
		viewport()->update();
	}
	QTreeView::leaveEvent(event);
}

void AntTreeView::startArrowAnimation(const QModelIndex& index, int target)
{
	m_animation.stop();
	m_currentAnimationIndex = index;
	m_animation.setStartValue(m_arrowRotations.value(index, 0));
	m_animation.setEndValue(target);
	m_animation.start();
}