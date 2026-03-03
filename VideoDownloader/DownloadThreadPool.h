#pragma once

#include <QMap>
#include <QThread>
#include <type_traits>

template<typename T>
class DownloadThreadPool
{
public:
	explicit DownloadThreadPool()
		: m_downloadThreadPool{}
		, MAX_POOL_SIZE{ 5 }
	{
		for (int i = 0; i < MAX_POOL_SIZE; ++i)
		{
			QThread* thread = new QThread{};
			thread->setObjectName("DownloadThread_" % QString::number(i));
			m_downloadThreadPool.insert(getNullValue(), thread);
		}
	}

	~DownloadThreadPool()
	{
		clearPool();
	}

	bool allocateThread(T downloadWorker)
	{
		if (m_downloadThreadPool.contains(downloadWorker))
			return true;

		for (auto it = m_downloadThreadPool.begin(); it != m_downloadThreadPool.end(); ++it)
		{
			if (it.key() == getNullValue())
			{
				QThread* thread = it.value();
				m_downloadThreadPool.erase(it);
				m_downloadThreadPool.insert(downloadWorker, thread);

				// 直接调用 moveToThread
				getPointer(downloadWorker)->moveToThread(thread);

				if (!thread->isRunning())
				{
					thread->start();
				}
				return true;
			}
		}
		if (m_downloadThreadPool.size() < MAX_POOL_SIZE)
		{
			QThread* thread = new QThread{};
			m_downloadThreadPool.insert(downloadWorker, thread);
			getPointer(downloadWorker)->moveToThread(thread);
			thread->start();
			return true;
		}
		return false;
	}

	void releaseThread(T downloadWorker, QThread* targetThread = QThread::currentThread(), bool quitThread = false)
	{
		if (auto it = m_downloadThreadPool.find(downloadWorker); it != m_downloadThreadPool.end())
		{
			QThread* thread = it.value();

			auto workerObj = getPointer(downloadWorker);

			QMetaObject::invokeMethod(workerObj, [workerObj, targetThread]() {
				workerObj->moveToThread(targetThread);
				}, Qt::BlockingQueuedConnection);

			if (m_downloadThreadPool.size() <= MAX_POOL_SIZE)
			{
				if (quitThread)
				{
					thread->quit();
					thread->wait();
				}
				m_downloadThreadPool.erase(it);
				m_downloadThreadPool.insert(getNullValue(), thread);
				return;
			}
			thread->quit();
			thread->wait();
			delete thread;
			m_downloadThreadPool.erase(it);
		}
	}

	bool isWorkerRunning(T downloadWorker) const
	{
		return m_downloadThreadPool.contains(downloadWorker);
	}

	void setMaxPoolSize(int size) { MAX_POOL_SIZE = size; }

	void clearPool()
	{
		for (auto it = m_downloadThreadPool.begin(); it != m_downloadThreadPool.end(); ++it)
		{
			QThread* thread = it.value();
			thread->quit();
			thread->wait();
			delete thread;
		}
		m_downloadThreadPool.clear();
	}

	int getPoolSize() const { return m_downloadThreadPool.size(); }

private:
	//QObject* getQObject(T worker) const
	//{
	//	if constexpr (std::is_pointer<T>::value) {
	//		if constexpr (std::is_base_of<QObject, std::remove_pointer_t<T>>::value) {
	//			return static_cast<QObject*>(worker);
	//		}
	//	}
	//	else {
	//		if constexpr (std::is_base_of<QObject, T>::value) {
	//			return static_cast<QObject*>(&worker);
	//		}
	//	}
	//	return nullptr;
	//}
	// 获取对象的指针
	auto getPointer(T worker) const
	{
		if constexpr (std::is_pointer<T>::value)
			return worker;
		else
			return &worker;
	}

	// 获取指针的指向的对象
	auto getObject(T worker) const
	{
		return *getPointer(worker);
	}

	// 获取空值的辅助函数
	T getNullValue() const
	{
		if constexpr (std::is_pointer<T>::value)
			return nullptr;
		else
			return T{};
	}

private:
	QMap<T, QThread*> m_downloadThreadPool;
	int MAX_POOL_SIZE = 5;
};