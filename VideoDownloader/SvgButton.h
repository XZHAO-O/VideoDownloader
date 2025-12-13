#pragma once

#include <QPushButton>

#include "AntTooltipManager.h"

class SvgButton : public QPushButton
{
	Q_OBJECT

public:
	enum IconSize
	{
		Small = 24,
		Medium = 32,
		Large = 40,
		ExtraLarge = 48
	};

	explicit SvgButton(QWidget* parent = nullptr);
	SvgButton(const QString& iconKey, QWidget* parent = nullptr);
	~SvgButton();

	// 设置图标键值
	void setIconKey(const QString& iconKey);
	QString iconKey() const { return m_iconKey; }

	// 设置图标大小
	void setIconSize(IconSize size);
	void setIconSize(int width, int height);
	QSize iconSize() const { return m_iconSize; }

	// 设置图标缩放比例
	void setIconScale(qreal scale);
	qreal iconScale() const { return m_iconScale; }

	// 设置图标边距
	void setIconMargin(int margin);
	int iconMargin() const { return m_iconMargin; }

	// 设置悬停时是否切换图标
	void setHoverIconEnabled(bool enabled);
	bool isHoverIconEnabled() const { return m_hoverIconEnabled; }

	// 重写setToolTip，保存tooltip文本
	void setToolTip(const QString& text);

	// 设置是否启用自定义tooltip
	void setToolTipEnabled(bool enabled);
	bool isToolTipEnabled() const { return m_toolTipEnabled; }

protected:
	// 重写绘制事件
	void paintEvent(QPaintEvent* event) override;

	// 鼠标事件
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

	// 大小调整事件
	void resizeEvent(QResizeEvent* event) override;

	// 显示tooltip
	void showCustomTooltip();

	// 隐藏tooltip
	void hideCustomTooltip();

private:
	// 根据当前主题和悬停状态获取pixmap
	QPixmap getCurrentPixmap() const;

	// 更新图标矩形
	void updateIconRect();

	// 更新按钮大小
	void updateButtonSize();

	// 图标键值
	QString m_iconKey;

	// 状态
	bool m_hovered;
	bool m_pressed;
	bool m_hoverIconEnabled;
	bool m_toolTipEnabled;  // 是否启用自定义tooltip

	// 几何属性
	QRectF m_iconRect;
	int m_iconMargin;
	qreal m_iconScale;
	QSize m_iconSize;

	// Tooltip相关
	QString m_toolTipText;  // 存储tooltip文本
	AntTooltipManager::Position m_toolTipPosition;  // tooltip显示位置
};