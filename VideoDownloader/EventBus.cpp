#include "EventBus.h"

QSharedPointer<EventBus> EventBus::s_instance = nullptr;

QSharedPointer<EventBus> EventBus::instance()
{
	if (!s_instance) {
		s_instance = QSharedPointer<EventBus>(new EventBus());
	}
	return s_instance;
}

void EventBus::destroyInstance()
{
	s_instance.clear();
}

EventBus::EventBus(QObject* parent)
	: QObject(parent)
{
}

void EventBus::unsubscribe(QObject* receiver)
{
	for (auto& receivers : m_receivers) {
		receivers.removeAll(receiver);
	}
}