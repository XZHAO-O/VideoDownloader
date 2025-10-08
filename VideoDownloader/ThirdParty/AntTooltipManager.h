#pragma once
#include <QWidget>
#include <QPointer>
#include <QTimer>
#include "AntTooltipViewController.h"

class AntTooltipManager : public QWidget
{
	Q_OBJECT
public:
	static AntTooltipManager* instance()
	{
		if (!m_instance)
		{
			m_instance = new AntTooltipManager();
		}
		return m_instance;
	}

	enum class Position
	{
		Top,
		Bottom,
		Left,
		Right
	};
	// position 表示你要把提示框放在目标的上 下 左 右 哪个位置
	void showTooltip(QWidget* targetWidget, const QString& text, Position position = Position::Right);
	// 隐藏提示框
	void hideTooltip();
signals:
	void hideTip();
private:
	explicit AntTooltipManager(QWidget* parent = nullptr);
	~AntTooltipManager();
	Position m_position;

	AntTooltipViewController* m_tooltipView = nullptr;
	static AntTooltipManager* m_instance;

};
