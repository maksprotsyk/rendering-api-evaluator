#pragma once

#include "ISystem.h"
#include <vector>
#include <string>
#include <algorithm>

namespace Engine::Systems
{
	class StatsSystem: public ISystem
	{
	public:
		void onStart() override;
		void onUpdate(float dt) override;
		void onStop() override;
		int getPriority() const override;

	private:

		float creationTime;
		std::vector<float> frameTimes;
		bool firstUpdate = true;
	};
}