#include "EventsManager.h"

namespace Engine
{

	std::unique_ptr<EventsManager> EventsManager::_instance = nullptr;

	EventsManager& EventsManager::get()
	{
		if (!_instance)
		{
			_instance = std::make_unique<EventsManager>();
		}
		return *_instance;
	}
}