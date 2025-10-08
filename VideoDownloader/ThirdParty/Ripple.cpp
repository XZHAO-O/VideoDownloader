#include "Ripple.h"

Ripple::Ripple(const QRectF& initRect, qreal cornerRadius, QObject* parent)
	:QObject(parent),
	m_initialRect(initRect),
	m_cornerRadius(cornerRadius),
	m_offset(0.0),
	m_opacity(1.0)
{
}

Ripple::~Ripple()
{
}