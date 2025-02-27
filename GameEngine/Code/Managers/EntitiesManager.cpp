#include "EntitiesManager.h"

namespace Engine
{

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

	void EntitiesManager::clear()
	{
		_takenIds.clear();
	}
}