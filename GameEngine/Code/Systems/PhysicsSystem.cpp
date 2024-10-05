#include "PhysicsSystem.h"
#include "Managers/ComponentsManager.h"
#include "Components/Transform.h"
#include "Components/Tag.h"


namespace Engine::Systems
{
	void PhysicsSystem::onStart()
	{

	}

	void PhysicsSystem::onUpdate(float dt)
	{
		ComponentsManager& compManager = ComponentsManager::get();
		auto& transformSet = compManager.getComponentSet<Components::Transform>();
		const auto& tagSet = compManager.getComponentSet<Components::Tag>();
		_scalingTime += dt;
		if (_scalingTime > 1.0f)
		{
			_scalingTime = 0.0f;
		}

		for (EntityID id : compManager.entitiesWithComponents<Components::Transform>())
		{
			if (tagSet.isPresent(id))
			{
				continue;
			}
			/*
			Components::Transform& transform = transformSet.getElement(id);
			transform.rotation.y += dt;
			if (_scalingTime < 0.5f)
			{
				transform.scale.x -= dt;
			}
			else
			{
				transform.scale.x += dt;
			}
			*/
		}
	}

	void PhysicsSystem::onStop()
	{

	}

	int PhysicsSystem::getPriority() const
	{
		return 0;
	}
}