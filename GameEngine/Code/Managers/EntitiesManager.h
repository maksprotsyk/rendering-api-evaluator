#pragma once

#include <set>
#include <memory>

namespace Engine
{
	using EntityID = int;

	class EntitiesManager
	{
	public:
		EntityID createEntity();
		void destroyEntity(EntityID id);
		void clear();

	private:
		static std::unique_ptr<EntitiesManager> _instance;
		std::set<EntityID> _takenIds;
	};
		
}

