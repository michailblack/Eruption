#pragma once
#include "Eruption/Core/Assert.h"
#include "Eruption/Core/Base.h"
#include "Eruption/Core/Events/Event.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

namespace Eruption
{
	template <typename TEvent>
	concept CEvent = std::is_base_of_v<Event, TEvent>;

	template <typename TCallable, typename TEvent>
	concept CEventCallback = requires(TCallable callback, TEvent& event) {
		{ callback(event) } -> std::same_as<bool>;
	};

	class IEventHandler
	{
	public:
		virtual ~IEventHandler() = default;

		virtual bool Invoke(Event& event) = 0;

		virtual uint32_t GetPriority() const = 0;
	};

	template <CEvent TEvent, CEventCallback<TEvent> TEventCallback>
	class TEventHandler : public IEventHandler
	{
	public:
		TEventHandler(TEventCallback&& callback, uint32_t priority) :
		    m_Callback(std::forward<TEventCallback>(callback)), m_Priority(priority)
		{}

		bool Invoke(Event& event) override
		{
			if (event.GetEventType() == TEvent::GetStaticType() && !event.Handled)
			{
				event.Handled |= Callback(static_cast<TEvent&>(event));
			}

			return event.Handled;
		}

		uint32_t GetPriority() const override { return m_Priority; }

	private:
		TEventCallback m_Callback;
		uint32_t       m_Priority = 0u;
	};

	class EventBus
	{
	public:
		EventBus()  = default;
		~EventBus() = default;

		template <CEvent TEvent, CEventCallback<TEvent> TEventCallback>
		void Subscribe(TEventCallback&& callback, uint32_t priority = 0)
		{
			auto& handlers = m_Handlers[TEvent::GetStaticType()];

			auto handler = CreateScope<TEventHandler>(std::forward<TEventCallback>(callback), priority);

			// Insert sorted by priority (descending order)
			auto insertIt = std::ranges::lower_bound(
			    handlers,
			    priority,
			    [](const auto& handler, uint32_t prio) { return handler->GetPriority() > prio; },
			    &IEventHandler::GetPriority
			);

			handlers.insert(insertIt, std::move(handler));
		}

		template <CEvent TEvent>
		bool Publish(TEvent& event)
		{
			auto it = m_Handlers.find(event.GetEventType());
			if (it == m_Handlers.end())
				return false;

			for (auto& handler : it->second)
			{
				if (handler->Invoke(event))
					return true;
			}

			return event.Handled;
		}

		template <CEvent TEvent>
		void Queue(TEvent&& event)
		{
			m_EventQueue.push_back(CreateScope<TEvent>(std::forward<TEvent>(event)));
		}

		void ProcessQueue()
		{
			for (auto& event : m_EventQueue)
			{
				PublishDynamic(*event);
			}

			m_EventQueue.clear();
		}

		void Clear()
		{
			m_Handlers.clear();
			m_EventQueue.clear();
		}

		void Clear(EventType type) { m_Handlers.erase(type); }

		size_t GetHandlerCount(EventType type) const
		{
			const auto it = m_Handlers.find(type);
			return it != m_Handlers.end() ? it->second.size() : 0;
		}

	private:
		void PublishDynamic(Event& event)
		{
			const auto it = m_Handlers.find(event.GetEventType());
			if (it == m_Handlers.end())
				return;

			for (auto& handler : it->second)
			{
				if (handler->Invoke(event))
					return;
			}
		}

	private:
		std::unordered_map<EventType, std::vector<Scope<IEventHandler>>> m_Handlers;
		std::vector<Scope<Event>>                                        m_EventQueue;
	};
}        // namespace Eruption
