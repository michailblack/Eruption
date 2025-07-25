#pragma once
#include <spdlog/spdlog.h>

#include <cstdint>
#include <format>
#include <map>
#include <memory>

namespace Eruption
{
	class Log
	{
	public:
		enum class Type : uint8_t
		{
			Core = 0,
		};

		enum class Level : uint8_t
		{
			Trace = 0,
			Info,
			Warn,
			Error,
			Fatal
		};

		struct TagDetails
		{
			bool  Enabled     = true;
			Level LevelFilter = Level::Trace;
		};

	public:
		static void Init();
		static void Shutdown();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

		static bool HasTag(const std::string& tag) { return s_EnabledTags.contains(tag); }

		static std::map<std::string, TagDetails>& EnabledTags() { return s_EnabledTags; }

		template <typename... Args>
		static void PrintMessage(Type type, Level level, std::format_string<Args...> format, Args&&... args);

		template <typename... Args>
		static void PrintMessageTag(
		    Type type, Level level, std::string_view tag, std::format_string<Args...> format, Args&&... args
		);

		static void PrintMessageTag(Type type, Level level, std::string_view tag, std::string_view message);

		template <typename... Args>
		static void PrintAssertMessage(
		    Type type, std::string_view prefix, std::format_string<Args...> message, Args&&... args
		);

		static void PrintAssertMessage(Type type, std::string_view prefix);

	public:
		// Enum utils
		static const char* LevelToString(Level level)
		{
			switch (level)
			{
				case Level::Trace: return "Trace";
				case Level::Info:  return "Info";
				case Level::Warn:  return "Warn";
				case Level::Error: return "Error";
				case Level::Fatal: return "Fatal";
			}
			return "";
		}
		static Level LevelFromString(std::string_view string)
		{
			if (string == "Trace")
				return Level::Trace;
			if (string == "Info")
				return Level::Info;
			if (string == "Warn")
				return Level::Warn;
			if (string == "Error")
				return Level::Error;
			if (string == "Fatal")
				return Level::Fatal;

			return Level::Trace;
		}

	private:
		inline static std::shared_ptr<spdlog::logger> s_CoreLogger;

		inline static std::map<std::string, TagDetails> s_EnabledTags;
	};

}        // namespace Eruption

// Core logging
#define ER_CORE_TRACE_TAG(tag, ...) \
	::Eruption::Log::PrintMessageTag(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Trace, tag, __VA_ARGS__)
#define ER_CORE_INFO_TAG(tag, ...) \
	::Eruption::Log::PrintMessageTag(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Info, tag, __VA_ARGS__)
#define ER_CORE_WARN_TAG(tag, ...) \
	::Eruption::Log::PrintMessageTag(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Warn, tag, __VA_ARGS__)
#define ER_CORE_ERROR_TAG(tag, ...) \
	::Eruption::Log::PrintMessageTag(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Error, tag, __VA_ARGS__)
#define ER_CORE_FATAL_TAG(tag, ...) \
	::Eruption::Log::PrintMessageTag(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Fatal, tag, __VA_ARGS__)

// Core Logging
#define ER_CORE_TRACE(...) \
	::Eruption::Log::PrintMessage(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Trace, __VA_ARGS__)
#define ER_CORE_INFO(...) \
	::Eruption::Log::PrintMessage(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Info, __VA_ARGS__)
#define ER_CORE_WARN(...) \
	::Eruption::Log::PrintMessage(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Warn, __VA_ARGS__)
#define ER_CORE_ERROR(...) \
	::Eruption::Log::PrintMessage(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Error, __VA_ARGS__)
#define ER_CORE_FATAL(...) \
	::Eruption::Log::PrintMessage(::Eruption::Log::Type::Core, ::Eruption::Log::Level::Fatal, __VA_ARGS__)

namespace Eruption
{
	template <typename... Args>
	void Log::PrintMessage(Log::Type type, Log::Level level, std::format_string<Args...> format, Args&&... args)
	{
		const auto detail = s_EnabledTags[""];
		if (detail.Enabled && detail.LevelFilter <= level)
		{
			auto logger = GetCoreLogger();
			switch (level)
			{
				case Level::Trace: logger->trace(format, std::forward<Args>(args)...); break;
				case Level::Info:  logger->info(format, std::forward<Args>(args)...); break;
				case Level::Warn:  logger->warn(format, std::forward<Args>(args)...); break;
				case Level::Error: logger->error(format, std::forward<Args>(args)...); break;
				case Level::Fatal: logger->critical(format, std::forward<Args>(args)...); break;
			}
		}
	}

	template <typename... Args>
	void Log::PrintMessageTag(
	    Log::Type type, Log::Level level, std::string_view tag, const std::format_string<Args...> format, Args&&... args
	)
	{
		auto detail = s_EnabledTags[std::string(tag)];
		if (detail.Enabled && detail.LevelFilter <= level)
		{
			const auto  logger    = GetCoreLogger();
			std::string formatted = std::format(format, std::forward<Args>(args)...);
			switch (level)
			{
				case Level::Trace: logger->trace("[{0}] {1}", tag, formatted); break;
				case Level::Info:  logger->info("[{0}] {1}", tag, formatted); break;
				case Level::Warn:  logger->warn("[{0}] {1}", tag, formatted); break;
				case Level::Error: logger->error("[{0}] {1}", tag, formatted); break;
				case Level::Fatal: logger->critical("[{0}] {1}", tag, formatted); break;
			}
		}
	}

	inline void Log::PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, std::string_view message)
	{
		auto detail = s_EnabledTags[std::string(tag)];
		if (detail.Enabled && detail.LevelFilter <= level)
		{
			const auto logger = GetCoreLogger();
			switch (level)
			{
				case Level::Trace: logger->trace("[{0}] {1}", tag, message); break;
				case Level::Info:  logger->info("[{0}] {1}", tag, message); break;
				case Level::Warn:  logger->warn("[{0}] {1}", tag, message); break;
				case Level::Error: logger->error("[{0}] {1}", tag, message); break;
				case Level::Fatal: logger->critical("[{0}] {1}", tag, message); break;
			}
		}
	}

	template <typename... Args>
	void Log::PrintAssertMessage(
	    Log::Type type, std::string_view prefix, std::format_string<Args...> message, Args&&... args
	)
	{
		const auto logger    = GetCoreLogger();
		auto       formatted = std::format(message, std::forward<Args>(args)...);
		logger->error("{0}: {1}", prefix, formatted);
	}

	inline void Log::PrintAssertMessage(Log::Type type, std::string_view prefix)
	{
		const auto logger = GetCoreLogger();
		logger->error("{0}", prefix);
	}
}        // namespace Eruption
