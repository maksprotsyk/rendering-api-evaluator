#pragma once

#include "ExperimentSystemBase.h"
#include "Managers/EntitiesManager.h"

namespace Engine::Systems
{
	class Experiment2System: public ExperimentSystemBase
	{
	public:
		void onStart() override;
		void onStop() override;
		int getPriority() const override;

	private:
		float m_distanceDelta = 1.0f;
		size_t m_elementsPerRow = 5;
	};
}