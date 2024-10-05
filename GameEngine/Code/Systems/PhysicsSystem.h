#pragma once

#include "ISystem.h"

namespace Engine::Systems
{
	class PhysicsSystem: public ISystem
	{
	public:
		void onStart() override;
		void onUpdate(float dt) override;
		void onStop() override;
		int getPriority() const override;
	};
}