#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <unordered_map>

#include "ISystem.h"
#include "Managers/EntitiesManager.h"

namespace Engine::Systems
{
	class InputSystem: public ISystem
	{
	public:
		void onStart() override;
		void onUpdate(float dt) override;
		void onStop() override;
		int getPriority() const override;

	private:
		bool isPressed(char key) const;
		float getAxisInput(char negativeKey, char positiveKey) const;

	private:

		std::unordered_map<char, bool> m_keyStates;

		EntityID m_cameraId = -1;
	};
}