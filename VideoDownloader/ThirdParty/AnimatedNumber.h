#pragma once

#include <QWidget>
#include <QVariantAnimation>

class AnimatedNumber : public QWidget
{
	Q_OBJECT
		Q_PROPERTY(int m_displayedValue READ displayedValue WRITE setDisplayedValue)

public:
	explicit AnimatedNumber(QWidget* parent = nullptr);

	int displayedValue() const;
	void setDisplayedValue(int val);

	void animateTo(int targetValue);
	void setFontSzie(int size) { m_fontSize = size; };
	void setTextWidth(int w) { m_textWidth = w; };
protected:
	void paintEvent(QPaintEvent* event) override;
	QSize sizeHint() const;
private:
	int m_displayedValue = 0;
	QVariantAnimation* m_animation = nullptr;
	int m_fontSize = 15;     //字体大小
	int m_textWidth = 150;
};
