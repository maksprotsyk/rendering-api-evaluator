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
        void subscribe(EventCallback<EventType> callback)
        {
            auto& listeners = getListeners<EventType>();
            listeners.push_back(std::move(callback));
        }

        template <typename EventType>
        void emit(const EventType& event) const
        {
            auto& listeners = getListeners<EventType>();
            for (auto& listener : listeners) {
                listener(event);
            }
        }

    private:
        template <typename EventType>
        std::vector<EventCallback<EventType>>& getListeners() const 
        {
            auto type = std::type_index(typeid(EventType));
            auto it = _eventsListeners.find(type);
            if (it == _eventsListeners.end())
            {
                it = _eventsListeners.emplace(type, std::make_unique<ListenerHolder<EventType>>()).first;
            }
            return static_cast<ListenerHolder<EventType>*>(it->second.get())->listeners;
        }

        struct IListenerHolder
        {
            virtual ~IListenerHolder() = default;
        };

        template <typename EventType>
        struct ListenerHolder : IListenerHolder 
        {
            std::vector<EventCallback<EventType>> listeners;
        };

        mutable std::unordered_map<std::type_index, std::unique_ptr<IListenerHolder>> _eventsListeners;

    };
}
