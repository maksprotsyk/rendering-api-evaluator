#include "InputSystem.h"

#include <iostream>

#include "Managers/EventsManager.h"
#include "Events/NativeInputEvents.h"
#include "Components/Transform.h"
#include "Components/Tag.h"

namespace Engine::Systems
{
	void InputSystem::onStart()
	{
		EventsManager::get().subscribe<Events::NativeKeyStateChanged>(
			[this](const Events::NativeKeyStateChanged& e)
			{
				_keyStates[(char)e.key] = e.pressed;
			}
		);

		auto& tagSet = ComponentsManager::get().getComponentSet<Components::Tag>();
		for (EntityID id : ComponentsManager::get().entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				_cameraId = id;
			}
		}
	}

	void InputSystem::onUpdate(float dt)
	{
		float distance = dt;
		Components::Transform& transform = ComponentsManager::get().getComponentSet<Components::Transform>().getElement(_cameraId);
		if (isPressed('W'))
		{
			transform.position.z += distance;
		}

		if (isPressed('S'))
		{
			transform.position.z -= distance;
		}

		if (isPressed('A'))
		{
			transform.position.x += distance;
		}

		if (isPressed('D'))
		{
			transform.position.x -= distance;
		}

		if (isPressed('Z'))
		{
			transform.position.y += distance;
		}

		if (isPressed('X'))
		{
			transform.position.y -= distance;
		}

	}

	void InputSystem::onStop()
	{

	}

	int InputSystem::getPriority() const
	{
		return 0;
	}

	bool InputSystem::isPressed(char key)
	{
		auto itr = _keyStates.find(key);
		if (itr == _keyStates.end())
		{
			return false;
		}
		return itr->second;
	}
}