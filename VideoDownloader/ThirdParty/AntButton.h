#pragma once

#include <QPushButton>

#include "Ripple.h"
#include "AntTooltipManager.h"

class AntButton : public QPushButton
{
	Q_OBJECT

public:
	// 按钮模式枚举
	enum ButtonMode
	{
		Filled = 0,      // 填充模式（默认）
		Outlined         // 描边模式
	};

	AntButton(QString btnText, qreal textSize, QWidget* parent);
	~AntButton();

	// 设置图标键值（类似SvgButton）
	void setIconKey(const QString& iconKey);
	QString iconKey() const { return m_iconKey; }

	// 设置按钮模式
	void setButtonMode(ButtonMode mode);
	ButtonMode buttonMode() const { return m_buttonMode; }

	// 设置描边宽度
	void setStrokeWidth(int width);
	int strokeWidth() const { return m_strokeWidth; }

	// 设置按钮颜色（如果不设置，则使用主题色）
	void setButtonColor(const QColor& color);
	QColor buttonColor() const { return m_buttonColor; }

	// 设置文字颜色（如果不设置，则根据模式自动选择）
	void setTextColor(const QColor& color);
	QColor textColor() const { return m_textColor; }

	// 设置图标缩放比例
	void setIconScale(qreal scale);
	qreal iconScale() const { return m_scaleFactor; }

	// 设置是否启用悬停图标
	void setHoverIconEnabled(bool enabled);
	bool isHoverIconEnabled() const { return m_hoverIconEnabled; }

	// 重写setToolTip，保存tooltip文本
	void setToolTip(const QString& text);

	// 设置是否启用自定义tooltip
	void setToolTipEnabled(bool enabled);
	bool isToolTipEnabled() const { return m_toolTipEnabled; }

protected:
	// 重写绘制逻辑
	void paintEvent(QPaintEvent* event) override;

	// 鼠标事件
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	// 获取当前状态的图标
	QPixmap getCurrentIcon() const;

	// 更新按钮大小
	void updateButtonSize();

	// 显示tooltip
	void showCustomTooltip();

	// 隐藏tooltip
	void hideCustomTooltip();

	// 动画属性
	int animTime = 500;
	int m_radius;
	int m_margin;

	// 按钮模式
	ButtonMode m_buttonMode = Filled;

	// 描边相关属性
	int m_strokeWidth = 1;

	// 自定义颜色（如果为无效颜色，则使用主题色）
	QColor m_buttonColor;
	QColor m_textColor;

	// 按钮当前状态
	bool m_hovered;
	bool m_pressed;

	// 图标相关
	QString m_iconKey;  // 图标键值
	bool m_hoverIconEnabled = true;  // 是否启用悬停图标
	qreal m_scaleFactor = 0.65;  // 图标缩放比例

	// Tooltip相关
	bool m_toolTipEnabled = true;  // 是否启用自定义tooltip
	QString m_toolTipText;  // 存储tooltip文本
	AntTooltipManager::Position m_toolTipPosition;  // tooltip显示位置

	// 防止短时间内多次点击
	QElapsedTimer m_clickTimer;
	int m_clickIntervalMs = 100;

	// 存储多个波纹
	QList<Ripple*> m_ripples;
	Ripple* m_ripp = nullptr;
};