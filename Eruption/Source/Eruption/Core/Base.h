#pragma once
#include <memory>

#if !defined(ER_PLATFORM_WINDOWS) && !defined(ER_PLATFORM_LINUX)
#	error Unknown platform.
#endif

#define BIT(x) (1u << x)

namespace Eruption
{
	// Pointer wrappers
	template <typename T>
	using Scope = std::unique_ptr<T>;
	template <typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename Derived, typename Base>
	    requires std::is_base_of_v<Base, Derived>
	constexpr Scope<Derived> As(const Scope<Base>& base)
	{
		return std::static_pointer_cast<Derived>(base);
	}

	template <typename T>
	using Ref = std::shared_ptr<T>;
	template <typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename Derived, typename Base>
	    requires std::is_base_of_v<Base, Derived>
	constexpr Ref<Derived> As(const Ref<Base>& base)
	{
		return std::static_pointer_cast<Derived>(base);
	}
}        // namespace Eruption