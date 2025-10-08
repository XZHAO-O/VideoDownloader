#include "EventBus.h"

EventBus& EventBus::instance()
{
	static EventBus instance;
	return instance;
}

EventBus::EventBus(QObject* parent) : QObject(parent)
{
}

void EventBus::unsubscribe(QObject* receiver)
{
	for (auto it = m_receivers.begin(); it != m_receivers.end(); ++it) {
		it.value().removeAll(receiver);
	}
}