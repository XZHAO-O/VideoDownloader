#pragma once

#include <QWidget>

class BadgeWidget : public QWidget
{
	Q_OBJECT

public:
	enum BadgeStatus {
		Success,    // 成功
		Error,		// 失败
		Running,    // 运行中
		Warning,    // 警告
		Default,	// 默认
		None		// 无
	};

	BadgeWidget(int width, int height, QWidget* parent = nullptr);
	~BadgeWidget();
	// 设置徽章显示的数字 isCircle决定是圆形还是圆角矩形
	void setBadge(bool isCircle, BadgeStatus status = None, int count = 0, int maxCount = 0);
protected:
	void paintEvent(QPaintEvent* event) override;
private:
	int m_count;			// 存储数量
	bool showCount;			// 是否显示数量
	bool m_isCircle;		// 是否绘制为圆形（True）或圆角矩形（False）
	int m_maxCount;			// 最大数量限制
	QFont m_font;
	BadgeStatus m_status;	// 当前状态
	QColor m_badgeColor;
};
