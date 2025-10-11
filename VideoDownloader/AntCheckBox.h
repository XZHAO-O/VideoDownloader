#pragma once
#include <QCheckBox>
#include <QPropertyAnimation>
#include <QEnterEvent>
#include <QMouseEvent>
#include "DesignSystem.h"

class AntCheckBox : public QCheckBox
{
	Q_OBJECT
		Q_PROPERTY(qreal innerRatio READ innerRatio WRITE setInnerRatio)

public:
	explicit AntCheckBox(QWidget* parent = nullptr);
	explicit AntCheckBox(const QString& text, QWidget* parent = nullptr);

	QSize sizeHint() const override;

	qreal innerRatio() const { return m_innerRatio; }
	void setInnerRatio(qreal ratio);

protected:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
	void onStateChanged(int state);

private:
	void init();
	QRect getCheckBoxRect() const;

	qreal m_innerRatio = 0.0;
	QPropertyAnimation* m_animation = nullptr;

	// 可调参数
	int m_marginLeft = 8;
	int m_spacing = 8;
	int m_squareSize = 18;

	QColor m_borderColorDefault = DesignSystem::instance()->borderColor();
	QColor m_borderColorHover = DesignSystem::instance()->borderColorHover();
	QColor m_fillColor = DesignSystem::instance()->primaryColor();

	bool m_hovered = false;
	bool m_pressed = false;
};