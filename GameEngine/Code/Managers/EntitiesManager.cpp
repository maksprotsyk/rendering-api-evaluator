#include "EntitiesManager.h"

namespace Engine
{

	std::unique_ptr<EntitiesManager> EntitiesManager::_instance = nullptr;

	EntitiesManager& EntitiesManager::get()
	{
		if (!_instance)
		{
			_instance = std::make_unique<EntitiesManager>();
		}
		return *_instance;
	}

	EntityID EntitiesManager::createEntity()
	{
		EntityID id = 0;
		for (EntityID takenId : _takenIds)
		{
			if (takenId != id)
			{
				break;
			}
			id++;
		}

		_takenIds.insert(id);
		return id;
	}

	void EntitiesManager::destroyEntity(EntityID id)
	{
		_takenIds.erase(id);
	}
}