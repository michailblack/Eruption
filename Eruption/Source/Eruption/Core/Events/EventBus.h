#pragma once
#include "Eruption/Core/Events/Event.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace Eruption
{
	class IEventDispatcher
	{
	public:
		virtual ~IEventDispatcher() = default;

		[[nodiscard]] virtual bool Dispatch(Event& event) = 0;

		[[nodiscard]] virtual uint32_t GetPriority() const = 0;
	};

	template <CEvent TEvent, CEventCallback<TEvent> TEventCallback>
	class TEventDispatcher : public IEventDispatcher
	{
	public:
		TEventDispatcher(TEventCallback&& callback, uint32_t priority) :
		    m_Callback(std::forward<TEventCallback>(callback)), m_Priority(priority)
		{}

		[[nodiscard]] bool Dispatch(Event& event) override
		{
			if (event.GetEventType() == TEvent::GetStaticType() && !event.IsHandled)
			{
				event.IsHandled = m_Callback(static_cast<TEvent&>(event));
				return true;
			}

			return false;
		}

		[[nodiscard]] uint32_t GetPriority() const override { return m_Priority; }

	private:
		TEventCallback m_Callback;
		uint32_t       m_Priority = 0;
	};

	class EventBus
	{
	public:
		template <CEvent TEvent, CEventCallback<TEvent> TEventCallback>
		void Subscribe(TEventCallback&& callback, uint32_t priority = 0)
		{
			auto& handlers = m_Dispatchers[TEvent::GetStaticType()];

			auto handler = CreateScope<TEventDispatcher<TEvent, TEventCallback>>(
			    std::forward<TEventCallback>(callback), priority
			);

			auto insertIt = std::ranges::lower_bound(
			    handlers, priority, std::greater<>{}, [](const Scope<IEventDispatcher>& h) { return h->GetPriority(); }
			);

			handlers.insert(insertIt, std::move(handler));
		}

		template <CEvent TEvent>
		void Queue(TEvent&& event)
		{
			m_EventQueue.push_back(CreateScope<TEvent>(std::forward<TEvent>(event)));
		}

		void ProcessQueue()
		{
			for (auto& event : m_EventQueue)
				Handle(*event);

			m_EventQueue.clear();
		}

		void Clear()
		{
			m_Dispatchers.clear();
			m_EventQueue.clear();
		}

		void Clear(EventType type) { m_Dispatchers.erase(type); }

		size_t GetHandlerCount(EventType type) const
		{
			const auto it = m_Dispatchers.find(type);
			return it != m_Dispatchers.end() ? it->second.size() : 0;
		}

	private:
		void Handle(Event& event)
		{
			const auto it = m_Dispatchers.find(event.GetEventType());
			if (it == m_Dispatchers.end())
				return;

			for (auto& dispatcher : std::views::reverse(it->second))
			{
				if (event.IsHandled)
					return;

				std::ignore = dispatcher->Dispatch(event);
			}
		}

	private:
		std::unordered_map<EventType, std::vector<Scope<IEventDispatcher>>> m_Dispatchers;
		std::vector<Scope<Event>>                                           m_EventQueue;
	};
}        // namespace Eruption
