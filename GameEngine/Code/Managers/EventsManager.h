#pragma once

#include <typeindex>
#include <functional>
#include <memory>

namespace Engine
{
    class EventsManager 
    {
    public:

        template <typename EventType>
        using EventCallback = std::function<void(const EventType&)>;

        template <typename EventType>
        void subscribe(EventCallback<EventType> callback);

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
            std::vector<EventCallback<EventType>> listeners;
        };

    private:
        template <typename EventType>
        std::vector<EventCallback<EventType>>& getListeners() const;

    private:
        mutable std::unordered_map<std::type_index, std::unique_ptr<IListenerHolder>> m_eventsListeners;

    };
}

#include "EventsManager.inl"
