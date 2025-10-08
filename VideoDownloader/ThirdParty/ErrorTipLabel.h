#pragma once

#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class ErrorTipLabel : public QLabel
{
	Q_OBJECT
		Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
	ErrorTipLabel(QWidget* parent);
	~ErrorTipLabel();

	void setTargetWidget(QWidget* widget);     // 设置哪个控件是它的锚点（一般为输入框）
	void showError(const QString text);       // 显示错误提示文字并淡入
	void hideError();                          // 淡出并隐藏
	void reposition();
public:
	qreal opacity() const { return m_opacity; }
	void setOpacity(qreal opacity)
	{
		m_opacity = opacity;
		update();
	}
	void clearHintText()
	{
		m_opacity = 0.0;
		update();
	}
protected:
	void paintEvent(QPaintEvent* event);
private:
	QString m_errorText;
	qreal m_opacity = 1.0;
	QPropertyAnimation* m_opacityAnimation = nullptr;
	QWidget* m_targetWidget = nullptr;  // 锚点控件（例如 MaterialLineEdit）
};
