#pragma once
#include "Eruption/Core/Base.h"

#include <ostream>
#include <string>

namespace Eruption
{

	enum class EventType
	{
		None = 0,

		AppRender,
		AppTick,
		AppUpdate,

		WindowClose,
		WindowFocus,
		WindowLostFocus,
		WindowMinimize,
		WindowMoved,
		WindowResize,

		KeyPressed,
		KeyReleased,
		KeyTyped,

		MouseButtonDown,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseScrolled
	};

	enum EventCategory
	{
		None                     = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput       = BIT(1),
		EventCategoryKeyboard    = BIT(2),
		EventCategoryMouse       = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

#define EVENT_CLASS_TYPE(type)              \
	static EventType GetStaticType()        \
	{                                       \
		return EventType::type;             \
	}                                       \
	EventType GetEventType() const override \
	{                                       \
		return GetStaticType();             \
	}                                       \
	const char* GetName() const override    \
	{                                       \
		return #type;                       \
	}

#define EVENT_CLASS_CATEGORY(category)    \
	int GetCategoryFlags() const override \
	{                                     \
		return category;                  \
	}

	class Event
	{
	public:
		bool Handled = false;

		virtual ~Event()                                           = default;
		[[nodiscard]] virtual EventType   GetEventType() const     = 0;
		[[nodiscard]] virtual const char* GetName() const          = 0;
		[[nodiscard]] virtual int         GetCategoryFlags() const = 0;
		[[nodiscard]] virtual std::string ToString() const { return GetName(); }

		[[nodiscard]] bool IsInCategory(EventCategory category) const { return GetCategoryFlags() & category; }
	};

	class EventDispatcher
	{
		template <typename T>
		using EventFn = std::function<bool(T&)>;

	public:
		explicit EventDispatcher(Event& event) : m_Event(event) {}

		template <typename T>
		bool Dispatch(EventFn<T> func)
		{
			if (m_Event.GetEventType() == T::GetStaticType() && !m_Event.Handled)
			{
				m_Event.Handled = func(*static_cast<T*>(&m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}
}        // namespace Eruption
