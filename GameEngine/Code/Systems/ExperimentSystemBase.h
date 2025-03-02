#pragma once

#include "ISystem.h"

namespace Engine::Systems
{
	class ExperimentSystemBase: public ISystem
	{
	public:
		void onStart() override;
		void onUpdate(float dt) override;
	private:
		void checkExperimentEnd(float dt);
	protected:
		static constexpr const char* k_experimentObjectTag = "ExperimentObject";

		std::string m_prefabName = "Cube";
		size_t m_prefabsCount = 10;
		float m_timeLeft = 10.0f;
	};
}