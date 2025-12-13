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

	// 立即隐藏（无动画）
	void hideTooltipImmediately();

	// 清理所有工具提示资源
	void cleanup();

signals:
	void hideTip();

private:
	explicit AntTooltipManager(QWidget* parent = nullptr);
	~AntTooltipManager();

	// 更新工具提示内容
	void updateTooltipContent(const QString& text, AntTooltip::ArrowDir dir);

	// 计算工具提示位置
	QPoint calculateTooltipPosition(QWidget* targetWidget, const QSize& tooltipSize, Position position);

	Position m_position;
	AntTooltipViewController* m_tooltipView = nullptr;  // 单例工具提示视图
	QWidget* m_currentTargetWidget = nullptr;  // 当前目标控件
	QString m_currentText;  // 当前显示文本
	Position m_currentPosition;  // 当前位置

	static AntTooltipManager* m_instance;

	// 延迟隐藏定时器
	QTimer* m_hideTimer;
};