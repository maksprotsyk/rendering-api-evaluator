#include "InputSystem.h"

#include <iostream>

#include "Managers/GameController.h"
#include "Events/NativeInputEvents.h"
#include "Components/Transform.h"
#include "Components/Tag.h"

REGISTER_SYSTEM(Engine::Systems::InputSystem);

namespace Engine::Systems
{
	//////////////////////////////////////////////////////////////////////////

	void InputSystem::onStart()
	{
		m_keyStateChangedListenerId = GameController::get().getEventsManager().subscribe<Events::NativeKeyStateChanged>(
			[this](const Events::NativeKeyStateChanged& e)
			{
				m_keyStates[(char)e.key] = e.pressed;
			}
		);

		auto& tagSet = GameController::get().getComponentsManager().getComponentSet<Components::Tag>();
		for (EntityID id : GameController::get().getComponentsManager().entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				m_cameraId = id;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void InputSystem::onUpdate(float dt)
	{
		float distance = dt;
		float angleDistance = dt;

		float rotationY = getAxisInput((char)37, (char)39);
		float rotationX = getAxisInput((char)38, (char)40);

		float movementX = getAxisInput('A', 'D');
		float movementZ = getAxisInput('S', 'W');
		float movementY = getAxisInput('Q', 'E');

		Components::Transform& transform = GameController::get().getComponentsManager().getComponentSet<Components::Transform>().getElement(m_cameraId);

		Utils::Vector3& position = transform.position;

		Utils::Vector3 forward = Utils::Vector3(0, 0, 1);
		forward.rotateArroundVector(Utils::Vector3(0, 1, 0), transform.rotation.y);

		Utils::Vector3 right = Utils::Vector3(1, 0, 0);
		right.rotateArroundVector(Utils::Vector3(0, 1, 0), transform.rotation.y);

		position += forward * distance * movementZ;
		position += right * distance * movementX;
		position.y += distance * movementY;

		Utils::Vector3& rotation = transform.rotation;
		rotation.y += angleDistance * rotationY;
		rotation.x += angleDistance * rotationX;
	}

	//////////////////////////////////////////////////////////////////////////

	void InputSystem::onStop()
	{
		GameController::get().getEventsManager().unsubscribe<Events::NativeKeyStateChanged>(m_keyStateChangedListenerId);
	}

	//////////////////////////////////////////////////////////////////////////

	int InputSystem::getPriority() const
	{
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////

	bool InputSystem::isPressed(char key) const
	{
		auto itr = m_keyStates.find(key);
		if (itr == m_keyStates.end())
		{
			return false;
		}
		return itr->second;
	}

	//////////////////////////////////////////////////////////////////////////

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

	//////////////////////////////////////////////////////////////////////////
}