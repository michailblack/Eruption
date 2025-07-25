#pragma once
#include "Eruption/Core/Log.h"

#include <chrono>
#include <utility>

namespace Eruption
{
	class Timer
	{
	public:
		Timer() { Reset(); }
		void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

		[[nodiscard]] float Elapsed() const
		{
			using namespace std::chrono;

			const auto end = high_resolution_clock::now();
			return static_cast<float>(duration_cast<microseconds>(end - m_Start).count()) * 1e-6f;
		}

		[[nodiscard]] float ElapsedMillis() const
		{
			using namespace std::chrono;

			const auto end = high_resolution_clock::now();
			return static_cast<float>(duration_cast<microseconds>(end - m_Start).count()) * 1e-3f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

	class ScopedTimer
	{
	public:
		explicit ScopedTimer(std::string name) : m_Name(std::move(name)) {}

		~ScopedTimer()
		{
			float time = m_Timer.ElapsedMillis();
			ER_CORE_TRACE_TAG("Timer", "{0} - {1}ms", m_Name, time);
		}

	private:
		std::string m_Name;
		Timer       m_Timer;
	};

}        // namespace Eruption
