#pragma once

#include <QObject>
#include <QMap>

class EventBus : public QObject
{
	Q_OBJECT
public:
	// 改为公共的静态方法获取实例
	static QSharedPointer<EventBus> instance();
	static void destroyInstance();

	template<typename EventType>
	void publish(const EventType& event) {
		QString typeName = typeid(EventType).name();
		auto receivers = m_receivers.value(typeName);
		for (auto receiver : receivers) {
			if (receiver) {
				QMetaObject::invokeMethod(receiver, "onEvent",
					Qt::QueuedConnection,
					Q_ARG(EventType, event));
			}
		}
		emit eventPublished(typeName, QVariant::fromValue(event));
	}

	template<typename EventType>
	void subscribe(QObject* receiver) {
		QString typeName = typeid(EventType).name();
		if (!m_receivers[typeName].contains(receiver)) {
			m_receivers[typeName].append(receiver);
			connect(receiver, &QObject::destroyed, this, [this, receiver, typeName]() {
				m_receivers[typeName].removeAll(receiver);
				});
		}
	}

	void unsubscribe(QObject* receiver);

signals:
	void eventPublished(const QString& eventType, const QVariant& eventData);

private:
	explicit EventBus(QObject* parent = nullptr);

	// 将析构函数改为public
public:
	~EventBus() = default;

private:
	static QSharedPointer<EventBus> s_instance;

	QMap<QString, QList<QObject*>> m_receivers;
};