#pragma once

#include "ISystem.h"
#include <vector>
#include <string>
#include <algorithm>
#include <pdh.h>

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

		PDH_HQUERY gpuQuery;
		PDH_HCOUNTER gpuUsageCounter;
		PDH_HQUERY cpuQuery;
		PDH_HCOUNTER cpuUsageCounter;

		float creationTime;
		std::vector<float> frameTimes;
		std::vector<float> memoryUsage;
		std::vector<float> cpuUsage;
		std::vector<float> gpuUsage;

		bool firstUpdate = true;
	};
}