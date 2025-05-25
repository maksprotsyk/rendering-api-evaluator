#pragma once

#include "EventsManager.h"

namespace Engine
{
    //////////////////////////////////////////////////////////////////////////

    template <typename EventType>
    EventListenerID EventsManager::subscribe(EventCallback<EventType> callback)
    {
        ListenerHolder<EventType>& listenerHolder = getListenersHolder<EventType>();
		EventListenerID id = listenerHolder.nextID++;
        listenerHolder.listeners.emplace_back(id, std::move(callback));
        return id;
    }

    //////////////////////////////////////////////////////////////////////////

    template<typename EventType>
    void EventsManager::unsubscribe(EventListenerID id)
    {
        ListenerHolder<EventType>& listenerHolder = getListenersHolder<EventType>();
        auto itr = std::remove_if(listenerHolder.listeners.begin(), listenerHolder.listeners.end(), [id](const auto& listener) { return listener.first == id; });
		listenerHolder.listeners.erase(itr, listenerHolder.listeners.end());
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename EventType>
    void EventsManager::emit(const EventType& event) const
    {
        ListenerHolder<EventType>& listenerHolder = getListenersHolder<EventType>();
        for (std::pair<EventListenerID, EventCallback<EventType>>& listener: listenerHolder.listeners)
        {
            listener.second(event);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    template <typename EventType>
    EventsManager::ListenerHolder<EventType>& EventsManager::getListenersHolder() const
    {
        auto type = std::type_index(typeid(EventType));
        auto it = m_eventsListeners.find(type);
        if (it == m_eventsListeners.end())
        {
            it = m_eventsListeners.emplace(type, std::make_unique<ListenerHolder<EventType>>()).first;
        }
        return *static_cast<ListenerHolder<EventType>*>(it->second.get());
    }

    //////////////////////////////////////////////////////////////////////////

}
