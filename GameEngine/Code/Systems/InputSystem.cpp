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
		float angleDistance = dt;
		Components::Transform& transform = ComponentsManager::get().getComponentSet<Components::Transform>().getElement(_cameraId);

		Utils::Vector3& rotation = transform.rotation;
		float rotationY = getAxisInput((char)37, (char)39);
		rotation.y += angleDistance * rotationY;

		float rotationX = getAxisInput((char)38, (char)40);
		rotation.x += angleDistance * rotationX;

		float movementX = getAxisInput('A', 'D');
		float movementZ = getAxisInput('S', 'W');

		auto forwardDir = Utils::Vector3(0, 0, 1);
		forwardDir.rotateArroundVector(Utils::Vector3(0, 1, 0), rotation.y);
		forwardDir.rotateArroundVector(Utils::Vector3(1, 0, 0), rotation.x);

		auto rightDir = Utils::Vector3::crossProduct(forwardDir, Utils::Vector3(0.0f, -1.0f, 0.0f));

		Utils::Vector3& position = transform.position;
		position += rightDir * distance * movementX;
		position += forwardDir * distance * movementZ;

	}

	void InputSystem::onStop()
	{

	}

	int InputSystem::getPriority() const
	{
		return 0;
	}

	bool InputSystem::isPressed(char key) const
	{
		auto itr = _keyStates.find(key);
		if (itr == _keyStates.end())
		{
			return false;
		}
		return itr->second;
	}

	float InputSystem::getAxisInput(char negativeKey, char positiveKey) const
	{
		float input = 0;
		if (isPressed(negativeKey))
		{
			input -= 1.0f;
		}

		if (isPressed(positiveKey))
		{
			input += 1.0f;
		}

		return input;
	}
}