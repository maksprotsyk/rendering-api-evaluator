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
		std::unordered_map<char, bool> _keyStates;

		EntityID _cameraId = -1;

		bool isPressed(char key) const;

		float getAxisInput(char negativeKey, char positiveKey) const;
	};
}