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

		PDH_HQUERY m_gpuQuery;
		PDH_HCOUNTER m_gpuUsageCounter;
		PDH_HQUERY m_cpuQuery;
		PDH_HCOUNTER m_cpuUsageCounter;

		float m_creationTime;
		std::vector<float> m_frameTimes;
		std::vector<float> m_memoryUsage;
		std::vector<float> m_cpuUsage;
		std::vector<float> m_gpuUsage;

		bool m_firstUpdate;
		float m_timePassed;

	};
}