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

		std::string rendererName;
		size_t objectsCount = 0;
		int totalNumberOfVertices = 0;
		float creationTime;
		std::vector<float> frameTimes;

		int updatesPerformed = 0;
	};
}