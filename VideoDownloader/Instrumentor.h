//
// Thread-safe instrumentation profiler by Cherno (Modified for multi-threading)
//
// Usage: include this header file somewhere in your code (eg. precompiled header), and then use like:
//
// Instrumentor::Get().BeginSession("Session Name");        // Begin session 
// {
//     InstrumentationTimer timer("Profiled Scope Name");   // Place code like this in scopes you'd like to include in profiling
//     // Code
// }
// Instrumentor::Get().EndSession();                        // End Session
//
#pragma once

#include <string>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <memory>

struct ProfileResult
{
	std::string Name;
	long long Start, End;
	uint32_t ThreadID;
};

struct InstrumentationSession
{
	std::string Name;
};

class Instrumentor
{
private:
	InstrumentationSession* m_CurrentSession;
	std::ofstream m_OutputStream;
	std::atomic<int> m_ProfileCount;

	// Thread-safe queue implementation
	std::queue<ProfileResult> m_Queue;
	std::mutex m_QueueMutex;
	std::condition_variable m_QueueCondition;
	std::atomic<bool> m_Running;
	std::thread m_WorkerThread;

	void WorkerThread()
	{
		while (m_Running || !m_Queue.empty()) {
			ProfileResult result;
			bool hasWork = false;

			{
				std::unique_lock<std::mutex> lock(m_QueueMutex);
				m_QueueCondition.wait(lock, [this]() {
					return !m_Queue.empty() || !m_Running;
					});

				if (!m_Queue.empty()) {
					result = m_Queue.front();
					m_Queue.pop();
					hasWork = true;
				}
			}

			if (hasWork) {
				WriteProfileToFile(result);
			}
		}
	}

	void WriteProfileToFile(const ProfileResult& result)
	{
		if (!m_OutputStream.is_open()) return;

		if (m_ProfileCount++ > 0)
			m_OutputStream << ",";

		std::string name = result.Name;
		std::replace(name.begin(), name.end(), '"', '\'');

		m_OutputStream << "{";
		m_OutputStream << "\"cat\":\"function\",";
		m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
		m_OutputStream << "\"name\":\"" << name << "\",";
		m_OutputStream << "\"ph\":\"X\",";
		m_OutputStream << "\"pid\":0,";
		m_OutputStream << "\"tid\":" << result.ThreadID << ",";
		m_OutputStream << "\"ts\":" << result.Start;
		m_OutputStream << "}";

		m_OutputStream.flush();
	}

public:
	Instrumentor()
		: m_CurrentSession(nullptr), m_ProfileCount(0), m_Running(false)
	{
	}

	~Instrumentor()
	{
		if (m_Running) {
			EndSession();
		}
	}

	void BeginSession(const std::string& name, const std::string& filepath = "results.json")
	{
		// Stop previous session if any
		if (m_Running) {
			EndSession();
		}

		std::lock_guard<std::mutex> lock(m_QueueMutex);

		m_OutputStream.open(filepath);
		if (!m_OutputStream.is_open()) {
			// Handle error - cannot open file
			return;
		}

		WriteHeader();
		m_CurrentSession = new InstrumentationSession{ name };
		m_ProfileCount = 0;
		m_Running = true;

		// Start worker thread
		m_WorkerThread = std::thread(&Instrumentor::WorkerThread, this);
	}

	void EndSession()
	{
		if (!m_Running) return;

		m_Running = false;
		m_QueueCondition.notify_all();

		if (m_WorkerThread.joinable()) {
			m_WorkerThread.join();
		}

		{
			std::lock_guard<std::mutex> lock(m_QueueMutex);
			WriteFooter();
			if (m_OutputStream.is_open()) {
				m_OutputStream.close();
			}
			delete m_CurrentSession;
			m_CurrentSession = nullptr;
			m_ProfileCount = 0;
		}
	}

	void WriteProfile(const ProfileResult& result)
	{
		if (!m_Running) return;

		{
			std::lock_guard<std::mutex> lock(m_QueueMutex);
			m_Queue.push(result);
		}
		m_QueueCondition.notify_one();
	}

	void WriteHeader()
	{
		m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
		m_OutputStream.flush();
	}

	void WriteFooter()
	{
		m_OutputStream << "]}";
		m_OutputStream.flush();
	}

	static Instrumentor& Get()
	{
		static Instrumentor instance;
		return instance;
	}
};

class InstrumentationTimer
{
public:
	InstrumentationTimer(const char* name)
		: m_Name(name), m_Stopped(false)
	{
		m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}

	~InstrumentationTimer()
	{
		if (!m_Stopped)
			Stop();
	}

	void Stop()
	{
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

		uint32_t threadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
		Instrumentor::Get().WriteProfile({ m_Name, start, end, threadID });

		m_Stopped = true;
	}

private:
	const char* m_Name;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
	bool m_Stopped;
};

#if defined(_DEBUG)
// 平台特定的函数签名宏
#if defined(_WIN32)
#define FUNC_SIG __FUNCSIG__
#elif defined(__linux__) || defined(__APPLE__)
#define FUNC_SIG __PRETTY_FUNCTION__
#else
#define FUNC_SIG __func__
#endif

// 使用宏重载技术
#define BENCHMARKING_START_ARG_1(filePath) Instrumentor::Get().BeginSession(FUNC_SIG, filePath)
#define BENCHMARKING_START_ARG_0() Instrumentor::Get().BeginSession(FUNC_SIG)

// 选择正确的宏版本
#define BENCHMARKING_START_SELECT(_1, _2, NAME, ...) NAME
#define BENCHMARKING_START(...) BENCHMARKING_START_SELECT(__VA_ARGS__, BENCHMARKING_START_ARG_1, BENCHMARKING_START_ARG_0)(__VA_ARGS__)

#define BENCHMARKING_STOP() Instrumentor::Get().EndSession()
#define BENCHMARKING_SCOPE(name) InstrumentationTimer timer##__LINE__(name)
#define BENCHMARKING_FUNCTION() BENCHMARKING_SCOPE(FUNC_SIG)
#else
#define BENCHMARKING_START(...)
#define BENCHMARKING_STOP()
#define BENCHMARKING_SCOPE(name)
#define BENCHMARKING_FUNCTION()
#define FUNC_SIG ""
#endif