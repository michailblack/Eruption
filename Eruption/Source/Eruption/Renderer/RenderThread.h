#pragma once
#include "Eruption/Core/Base.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace Eruption
{
	enum class ThreadingPolicy : uint8_t
	{
		SingleThreaded,
		MultiThreaded
	};

	class RenderThread
	{
	public:
		explicit RenderThread(ThreadingPolicy policy) : m_Policy(policy) {}

		~RenderThread() { Stop(); }

		RenderThread(const RenderThread&)            = delete;
		RenderThread& operator=(const RenderThread&) = delete;

		void Start(std::move_only_function<void()> work)
		{
			m_Work = std::move(work);
			if (m_Policy == ThreadingPolicy::MultiThreaded)
				m_Thread = std::thread([this] { ThreadLoop(); });
		}

		void Kick()
		{
			if (m_Policy == ThreadingPolicy::SingleThreaded)
			{
				m_Work();
				return;
			}

			{
				std::scoped_lock _(m_Mutex);
				m_WorkPending = true;
			}

			m_Condition.notify_one();
		}

		void WaitIdle()
		{
			if (m_Policy == ThreadingPolicy::SingleThreaded)
				return;

			std::unique_lock _(m_Mutex);
			m_Condition.wait(_, [this] { return !m_WorkPending && !m_Working; });
		}

		void Stop()
		{
			if (m_Policy == ThreadingPolicy::SingleThreaded || !m_Thread.joinable())
				return;

			{
				std::scoped_lock _(m_Mutex);
				m_Stop = true;
			}

			m_Condition.notify_one();
			m_Thread.join();
		}

	private:
		void ThreadLoop()
		{
			while (true)
			{
				{
					std::unique_lock _(m_Mutex);
					m_Condition.wait(_, [this] { return m_WorkPending || m_Stop; });
					if (m_Stop)
						return;

					m_WorkPending = false;
					m_Working     = true;
				}

				m_Work();

				{
					std::scoped_lock _(m_Mutex);
					m_Working = false;
				}

				m_Condition.notify_all();
			}
		}

	private:
		ThreadingPolicy                 m_Policy;
		std::move_only_function<void()> m_Work;
		std::thread                     m_Thread;

		std::mutex              m_Mutex;
		std::condition_variable m_Condition;
		bool                    m_WorkPending = false;
		bool                    m_Working     = false;
		bool                    m_Stop        = false;
	};
}        // namespace Eruption
