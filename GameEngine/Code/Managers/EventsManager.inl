#pragma once

#include "EventsManager.h"

namespace Engine
{
    //////////////////////////////////////////////////////////////////////////

    template <typename EventType>
    void EventsManager::subscribe(EventCallback<EventType> callback)
    {
        auto& listeners = getListeners<EventType>();
        listeners.push_back(std::move(callback));
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename EventType>
    void EventsManager::emit(const EventType& event) const
    {
        auto& listeners = getListeners<EventType>();
        for (auto& listener : listeners) 
        {
            listener(event);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename EventType>
    std::vector<EventsManager::EventCallback<EventType>>& EventsManager::getListeners() const 
    {
        auto type = std::type_index(typeid(EventType));
        auto it = m_eventsListeners.find(type);
        if (it == m_eventsListeners.end())
        {
            it = m_eventsListeners.emplace(type, std::make_unique<ListenerHolder<EventType>>()).first;
        }
        return static_cast<ListenerHolder<EventType>*>(it->second.get())->listeners;
    }

    //////////////////////////////////////////////////////////////////////////

}
