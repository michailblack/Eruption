#pragma once
#include "Eruption/Core/Log.h"

#ifdef ER_PLATFORM_WINDOWS
#	define ER_DEBUG_BREAK __debugbreak()
#elif defined(ER_COMPILER_CLANG)
#	define ER_DEBUG_BREAK __builtin_debugtrap()
#else
#	define ER_DEBUG_BREAK
#endif

#ifdef ER_DEBUG
#	define ER_ENABLE_ASSERTS
#endif

#define ER_ENABLE_VERIFY

#ifdef ER_ENABLE_ASSERTS
#	ifdef ER_COMPILER_CLANG
#		define ER_CORE_ASSERT_MESSAGE_INTERNAL(...) \
			::Eruption::Log::PrintAssertMessage(::Eruption::Log::Type::Core, "Assertion Failed", ##__VA_ARGS__)
#		define ER_ASSERT_MESSAGE_INTERNAL(...) \
			::Eruption::Log::PrintAssertMessage(::Eruption::Log::Type::Client, "Assertion Failed", ##__VA_ARGS__)
#	else
#		define ER_CORE_ASSERT_MESSAGE_INTERNAL(...)                                       \
			::Eruption::Log::PrintAssertMessage(                                           \
			    ::Eruption::Log::Type::Core, "Assertion Failed" __VA_OPT__(, ) __VA_ARGS__ \
			)
#		define ER_ASSERT_MESSAGE_INTERNAL(...)                                              \
			::Eruption::Log::PrintAssertMessage(                                             \
			    ::Eruption::Log::Type::Client, "Assertion Failed" __VA_OPT__(, ) __VA_ARGS__ \
			)
#	endif

#	define ER_CORE_ASSERT(condition, ...)                    \
		do                                                    \
		{                                                     \
			if (!(condition))                                 \
			{                                                 \
				ER_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); \
				ER_DEBUG_BREAK;                               \
			}                                                 \
		} while (0)
#	define ER_ASSERT(condition, ...)                    \
		do                                               \
		{                                                \
			if (!(condition))                            \
			{                                            \
				ER_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); \
				ER_DEBUG_BREAK;                          \
			}                                            \
		} while (0)
#else
#	define ER_CORE_ASSERT(condition, ...) ((void) (condition))
#	define ER_ASSERT(condition, ...) ((void) (condition))
#endif

#ifdef ER_ENABLE_VERIFY
#	ifdef ER_COMPILER_CLANG
#		define ER_CORE_VERIFY_MESSAGE_INTERNAL(...) \
			::Eruption::Log::PrintAssertMessage(::Eruption::Log::Type::Core, "Verify Failed", ##__VA_ARGS__)
#	else
#		define ER_CORE_VERIFY_MESSAGE_INTERNAL(...) \
			::Eruption::Log::PrintAssertMessage(::Eruption::Log::Type::Core, "Verify Failed" __VA_OPT__(, ) __VA_ARGS__)
#	endif

#	define ER_CORE_VERIFY(condition, ...)                    \
		do                                                    \
		{                                                     \
			if (!(condition))                                 \
			{                                                 \
				ER_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); \
				ER_DEBUG_BREAK;                               \
			}                                                 \
		} while (0)
#else
#	define ER_CORE_VERIFY(condition, ...) ((void) (condition))
#endif
