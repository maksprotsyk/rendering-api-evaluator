#pragma once

#include <set>
#include <memory>

namespace Engine
{
	using EntityID = int;

	class EntitiesManager
	{
	public:
		static EntitiesManager& get();
		EntityID createEntity();
		void destroyEntity(EntityID id);

	private:
		static std::unique_ptr<EntitiesManager> _instance;
		std::set<EntityID> _takenIds;
	};
		
}

