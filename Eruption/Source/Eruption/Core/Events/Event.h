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
		bool IsHandled = false;

		virtual ~Event()                                           = default;
		[[nodiscard]] virtual EventType   GetEventType() const     = 0;
		[[nodiscard]] virtual const char* GetName() const          = 0;
		[[nodiscard]] virtual int         GetCategoryFlags() const = 0;
		[[nodiscard]] virtual std::string ToString() const { return GetName(); }

		[[nodiscard]] bool IsInCategory(EventCategory category) const { return GetCategoryFlags() & category; }
	};

	template <typename TEvent>
	concept CEvent = std::is_base_of_v<Event, TEvent>;

	template <typename TCallable, typename TEvent>
	concept CEventCallback = requires(TCallable callback, TEvent& event) {
		{ callback(event) } -> std::same_as<bool>;
	};

#define ER_BIND_EVENT_FN(OnEventFn) \
	[this] template <CEvent TEvent> \
	(TEvent&&) -> bool { return this->OnEventFn(std::forward<TEvent>(event)); }

	class EventDispatcher
	{
	public:
		explicit EventDispatcher(Event& event) : m_Event(event) {}

		template <CEvent TEvent, CEventCallback<TEvent> TCallback>
		bool Dispatch(TCallback callback)
		{
			if (m_Event.GetEventType() == TEvent::GetStaticType() && !m_Event.IsHandled)
			{
				m_Event.IsHandled = callback(reinterpret_cast<TEvent&>(m_Event));
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
