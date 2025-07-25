#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include <spdlog/sinks/basic_file_sink.h>

namespace Eruption
{
	void Log::Init()
	{
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

		consoleSink->set_pattern("%^[%T] [%l] %n:%$ %^%v%$");
		consoleSink->set_color_mode(spdlog::color_mode::always);

		s_CoreLogger = std::make_shared<spdlog::logger>("ERUPTION", consoleSink);
		s_CoreLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		s_CoreLogger.reset();
		spdlog::drop_all();
	}
}        // namespace Eruption