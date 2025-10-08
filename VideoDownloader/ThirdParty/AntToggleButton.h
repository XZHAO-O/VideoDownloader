#ifndef ANTTOGGLEBUTTON_H
#define ANTTOGGLEBUTTON_H

#include <QWidget>
#include <QColor>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>

class AntToggleButton : public QWidget
{
	Q_OBJECT
		Q_PROPERTY(int m_circleX READ circleX WRITE setCircleX)
		Q_PROPERTY(int m_circleWidth READ circleWidth WRITE setCircleWidth)
		Q_PROPERTY(QColor m_bgColor READ bgColor WRITE setBgColor)

public:
	AntToggleButton(QSize size = QSize(52, 26), QWidget* parent = nullptr);
	~AntToggleButton();

	void setChecked(bool checked);
	bool isChecked() const { return m_checked; }

	int circleX() const { return m_circleX; }
	void setCircleX(int x) { m_circleX = x; update(); }

	int circleWidth() const { return m_circleWidth; }
	void setCircleWidth(int w) { m_circleWidth = w; update(); }

	QColor bgColor() const { return m_bgColor; }
	void setBgColor(const QColor& color) { m_bgColor = color; update(); }
	// 是否显示上面的文本提示
	void setShowText(bool show);
signals:
	void toggled(bool checked);

protected:
	void paintEvent(QPaintEvent*) override;
	void mousePressEvent(QMouseEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	bool m_checked;
	bool m_showText = false;
	int m_circleX;
	int m_circleWidth;
	QSize m_size;
	QColor m_bgColor;
	QColor m_toggleButtonColor;
	QColor m_textColor;

	QPropertyAnimation* posAnim;
	QPropertyAnimation* colorAnim;

	QParallelAnimationGroup* groupAnim;
};

#endif // ANTTOGGLEBUTTON_H
