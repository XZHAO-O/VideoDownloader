#include "FlowLayout.h"

FlowLayout::FlowLayout(QWidget* parent, int margin, int spacing)
	: QLayout(parent), m_spacing(spacing)
{
	setContentsMargins(margin, margin, margin, margin);  // 设置布局的边距
}

FlowLayout::~FlowLayout()
{
	QLayoutItem* item;
	while ((item = takeAt(0)) != nullptr)
		delete item;
}

void FlowLayout::addItem(QLayoutItem* item)
{
	itemList.append(item);
}

int FlowLayout::horizontalSpacing() const
{
	if (m_spacing != -1)
		return m_spacing;
	else
		return spacing();  // 使用spacing函数来获取间距
}

int FlowLayout::verticalSpacing() const
{
	if (m_spacing != -1)
		return m_spacing;
	else
		return spacing();  // 使用spacing函数来获取间距
}

void FlowLayout::setGeometry(const QRect& rect)
{
	QLayout::setGeometry(rect);
	doLayout(rect);
}

QSize FlowLayout::sizeHint() const
{
	return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
	QSize size(0, 0);
	for (QLayoutItem* item : itemList) {
		size = size.expandedTo(item->minimumSize());
	}
	QMargins margins = contentsMargins();  // 获取布局的边距
	size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
	return size;
}

int FlowLayout::count() const
{
	return itemList.count();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
	return itemList.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
	if (index >= 0 && index < itemList.count()) {
		return itemList.takeAt(index);  // 从列表中移除并返回布局项
	}
	return nullptr;
}

void FlowLayout::doLayout(const QRect& rect)
{
	QMargins margins = contentsMargins();  // 获取布局的边距
	int x = rect.left() + margins.left();  // 左边距
	int y = rect.top() + margins.top();   // 上边距
	int lineHeight = 0;
	int width = rect.width() - margins.left() - margins.right();  // 容器宽度减去左右边距

	// 动画列表：用于存储正在执行的动画
	QList<QPropertyAnimation*> animations;

	for (QLayoutItem* item : itemList) {
		QWidget* widget = item->widget();
		int spaceX = horizontalSpacing();
		int spaceY = verticalSpacing();

		QSize itemSize = item->sizeHint();
		if (x + itemSize.width() > rect.right()) {
			// Start new line
			x = rect.left() + margins.left();  // 新的一行从左边距开始
			y += lineHeight + spaceY;  // 下一行的y坐标
			lineHeight = 0;  // 更新行高
		}

		// 获取控件的新位置
		QRect newGeometry(QPoint(x, y), itemSize);

		// 如果控件的位置发生变化，则为该控件创建动画
		if (widget->geometry() != newGeometry) {
			QPropertyAnimation* animation = new QPropertyAnimation(widget, "geometry");
			animation->setDuration(300);  // 设置动画时间
			animation->setEasingCurve(QEasingCurve::OutCubic);
			animation->setStartValue(widget->geometry());
			animation->setEndValue(newGeometry);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
			animations.append(animation);
		}

		x += itemSize.width() + spaceX;  // 更新x坐标
		lineHeight = qMax(lineHeight, itemSize.height());  // 更新行高
	}

	// 执行动画，所有控件都将平滑移动到新位置
	for (QPropertyAnimation* anim : animations) {
		anim->start();
	}
}