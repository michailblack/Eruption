#pragma once
#include <memory>
#include <type_traits>
#include <utility>

#if !defined(ER_PLATFORM_WINDOWS) && !defined(ER_PLATFORM_LINUX) && !defined(ER_PLATFORM_MACOS)
#	error Unknown platform.
#endif

#define ER_BIT(x) (1u << (x))

#define ER_EXPAND_MACRO(x) x
#define ER_STRINGIFY_MACRO(x) #x

namespace Eruption
{
	// Unique ownership.
	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	// Shared ownership.
	template <typename T>
	using Ref = std::shared_ptr<T>;

	template <typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	// Non-owning weak handle to a Ref.
	template <typename T>
	using WeakRef = std::weak_ptr<T>;

	template <typename Derived, typename Base>
	    requires std::is_base_of_v<Base, Derived>
	Ref<Derived> As(const Ref<Base>& base)
	{
		return std::static_pointer_cast<Derived>(base);
	}
}        // namespace Eruption
