#pragma once

#include <QObject>
#include <QRect>

class Ripple : public QObject
{
	Q_OBJECT
		Q_PROPERTY(qreal m_offset READ offset WRITE setOffset NOTIFY offsetChanged)
		Q_PROPERTY(qreal m_opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)
public:
	Ripple(const QRectF& initRect, qreal cornerRadius, QObject* parent = nullptr);
	~Ripple();

	// Getter 和 Setter for offset
	qreal offset() const { return m_offset; }
	void setOffset(qreal of) {
		if (!qFuzzyCompare(m_offset, of))
		{
			m_offset = of;
			emit offsetChanged();
		}
	}

	// Getter 和 Setter for opacity
	qreal opacity() const { return m_opacity; }
	void setOpacity(qreal o) {
		if (!qFuzzyCompare(m_opacity, o))
		{
			m_opacity = o;
			emit opacityChanged();
		}
	}
	void setBeginValue(int offset, qreal opacity)
	{
		m_offset = offset;
		m_opacity = opacity;
	}
	// 辅助数据访问
	QRectF initialRect() const { return m_initialRect; }
	qreal cornerRadius() const { return m_cornerRadius; }
signals:
	void offsetChanged();
	void opacityChanged();
private:
	QRectF m_initialRect;   // 波纹起始矩形（按钮背景大小）
	qreal m_cornerRadius;   // 圆角半径（与按钮保持一致）
	qreal m_offset;         // 当前动画扩散程度（0.0~1.0）
	qreal m_opacity;        // 当前透明度（1.0~0.0）
};
