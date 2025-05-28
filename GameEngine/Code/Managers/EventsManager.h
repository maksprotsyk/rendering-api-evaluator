#pragma once

#include <typeindex>
#include <functional>
#include <memory>

namespace Engine
{
    using EventListenerID = int;

    class EventsManager 
    {
    public:

        template <typename EventType>
        using EventCallback = std::function<void(const EventType&)>;

        template <typename EventType>
        EventListenerID subscribe(EventCallback<EventType> callback);

        template <typename EventType>
		void unsubscribe(EventListenerID id);

        template <typename EventType>
        void emit(const EventType& event) const;

    private:
        struct IListenerHolder
        {
            virtual ~IListenerHolder() = default;
        };

        template <typename EventType>
        struct ListenerHolder : IListenerHolder
        {
            std::vector<std::pair<EventListenerID, EventCallback<EventType>>> listeners;
			EventListenerID nextID = 0;
        };

    private:
        template <typename EventType>
        ListenerHolder<EventType>& getListenersHolder() const;

    private:
        mutable std::unordered_map<std::type_index, std::unique_ptr<IListenerHolder>> m_eventsListeners;

    };
}

#include "EventsManager.inl"
