#pragma once

#include <QPushButton>

#include "SvgButton.h"

class SvgToggleButton : public SvgButton
{
	Q_OBJECT

public:
	explicit SvgToggleButton(QWidget* parent = nullptr);
	SvgToggleButton(const QString& normalIconKey, const QString& activeIconKey,
		QWidget* parent = nullptr);
	~SvgToggleButton();

	// 设置正常状态图标键值
	void setNormalIconKey(const QString& iconKey);
	QString normalIconKey() const { return m_normalIconKey; }

	// 设置激活状态图标键值
	void setActiveIconKey(const QString& iconKey);
	QString activeIconKey() const { return m_activeIconKey; }

	// 设置当前状态
	void setActive(bool active);
	bool isActive() const { return m_active; }

	// 切换状态
	void toggle();

	// 重写setToolTip，设置统一的tooltip（两个状态使用相同的tooltip）
	void setToolTip(const QString& text);

	// 设置状态相关的tooltip
	void setNormalToolTip(const QString& text);
	void setActiveToolTip(const QString& text);

	// 获取状态相关的tooltip
	QString normalToolTip() const { return m_normalToolTip; }
	QString activeToolTip() const { return m_activeToolTip; }

signals:
	// 激活状态下的点击信号
	void normalized();

	// 正常状态下的点击信号
	void actived();

	// 状态变化信号
	void stateChanged(bool active);

protected:
	// 重写鼠标事件
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

	// 重写进入事件，更新当前tooltip
	void enterEvent(QEnterEvent* event) override;

private:
	// 更新当前图标键值
	void updateIconKey();

	// 更新当前tooltip
	void updateToolTip();

	QString m_normalIconKey;  // 正常状态图标键值
	QString m_activeIconKey;  // 激活状态图标键值
	QString m_normalToolTip;  // 正常状态tooltip
	QString m_activeToolTip;  // 激活状态tooltip
	bool m_active;            // 当前是否为激活状态
};