#include "StatsSystem.h"

#include <iostream>
#include <fstream>
#include <psapi.h>

#include "Managers/GameController.h"
#include "Events/NativeInputEvents.h"
#include "Utils/DebugMacros.h"
#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"

REGISTER_SYSTEM(Engine::Systems::StatsSystem);

namespace Engine::Systems
{

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onStart()
	{
		m_firstUpdate = true;

		PDH_STATUS cpuOpenRes = PdhOpenQuery(nullptr, 0, &m_cpuQuery);
		ASSERT(cpuOpenRes == ERROR_SUCCESS, "Failed to open CPU query");
		PDH_STATUS cpuAddRes = PdhAddCounter(m_cpuQuery, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &m_cpuUsageCounter);
		ASSERT(cpuAddRes == ERROR_SUCCESS, "Failed to add CPU counter");

		PDH_STATUS cpuCollectRes = PdhCollectQueryData(m_cpuQuery);
		ASSERT(cpuCollectRes == ERROR_SUCCESS, "Failed to collect CPU query data");

		PDH_STATUS gpuOpenRes = PdhOpenQuery(nullptr, 0, &m_gpuQuery);
		ASSERT(gpuOpenRes == ERROR_SUCCESS, "Failed to open GPU query");
		PDH_STATUS gpuAddRes = PdhAddCounter(m_gpuQuery, TEXT("\\GPU Engine(*_3D)\\Utilization Percentage"), 0, &m_gpuUsageCounter);
		ASSERT(gpuAddRes == ERROR_SUCCESS, "Failed to add GPU counter");

		PDH_STATUS gpuCollectRes = PdhCollectQueryData(m_gpuQuery);
		ASSERT(gpuCollectRes == ERROR_SUCCESS, "Failed to collect GPU query data");
	}

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onUpdate(float dt)
	{
		if (m_firstUpdate)
		{
			m_creationTime = dt;
			m_firstUpdate = false;
			m_timePassed = 0.0f;
			return;
		}
		m_timePassed += dt;
		m_frameTimes.push_back(dt);

		if (m_timePassed > 1.0f)
		{
			m_timePassed = 0.0f;

			PDH_STATUS res = PdhCollectQueryData(m_cpuQuery);
			ASSERT(res == ERROR_SUCCESS, "Failed to collect CPU query data");

			PDH_FMT_COUNTERVALUE cpuCounterVal;
			res = PdhGetFormattedCounterValue(m_cpuUsageCounter, PDH_FMT_DOUBLE, nullptr, &cpuCounterVal);
			ASSERT(res == ERROR_SUCCESS, "Failed to format CPU query data");

			m_cpuUsage.push_back((float)cpuCounterVal.doubleValue);

			res = PdhCollectQueryData(m_gpuQuery);
			ASSERT(res == ERROR_SUCCESS, "Failed to collect GPU query data");

			PDH_FMT_COUNTERVALUE gpuCounterVal;
			res = PdhGetFormattedCounterValue(m_gpuUsageCounter, PDH_FMT_DOUBLE, nullptr, &gpuCounterVal);
			ASSERT(res == ERROR_SUCCESS, "Failed to format GPU query data");

			m_gpuUsage.push_back((float)gpuCounterVal.doubleValue);

			// Memory usage data collection
			PROCESS_MEMORY_COUNTERS memCounter;
			if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter)))
			{
				m_memoryUsage.push_back(memCounter.WorkingSetSize / (1024.0 * 1024.0)); // in MB
			}
		}

		if (m_frameTimes.size() >= 10000)
		{
			GameController::get().getEventsManager().emit<Events::NativeExitRequested>({});
		}

	}

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onStop()
	{
		PdhCloseQuery(m_gpuQuery);
		PdhCloseQuery(m_cpuQuery);

		std::string rendererName = m_config["renderer"];
		std::string outputPath = m_config["outputFile"];

		if (outputPath.empty())
		{
			return;
		}

		auto& compManager = GameController::get().getComponentsManager();
		auto const& models = compManager.getComponentSet<Components::Model>();
		size_t objectsCount = models.size();
		size_t totalNumberOfVertices = 0;

		std::sort(m_frameTimes.begin(), m_frameTimes.end());
		float medianFrameTime = m_frameTimes[m_frameTimes.size() / 2];
		float averageFrameTime = std::accumulate(m_frameTimes.begin(), m_frameTimes.end(), 0.0f) / m_frameTimes.size();
		
		size_t fivePercents = m_frameTimes.size() / 20;
		float percentile95 = m_frameTimes[m_frameTimes.size() - fivePercents];
		float percentile5 = m_frameTimes[fivePercents];

		float averageCPUUsage = std::accumulate(m_cpuUsage.begin(), m_cpuUsage.end(), 0.0) / m_cpuUsage.size();
		float averageGPUUsage = std::accumulate(m_gpuUsage.begin(), m_gpuUsage.end(), 0.0) / m_gpuUsage.size();
		float averageMemoryUsage = std::accumulate(m_memoryUsage.begin(), m_memoryUsage.end(), 0.0) / m_memoryUsage.size();

		std::ofstream outFile(outputPath);

		if (!outFile.is_open())
		{
			return;
		}

		outFile << "Renderer: " << rendererName << std::endl;
		outFile << "Objects count: " << objectsCount << std::endl;
		outFile << "Total number of vertices: " << totalNumberOfVertices << std::endl;
		outFile << "Creation time: " << m_creationTime << std::endl;
		outFile << "Average frame time: " << averageFrameTime << std::endl;
		outFile << "Average CPU usage: " << averageCPUUsage << std::endl;
		outFile << "Average GPU usage: " << averageGPUUsage << std::endl;
		outFile << "Average memory usage: " << averageMemoryUsage << std::endl;
		outFile << "Median frame time: " << medianFrameTime << std::endl;
		outFile << "95th percentile frame time: " << percentile95 << std::endl;
		outFile << "5th percentile frame time: " << percentile5 << std::endl;
	}

	//////////////////////////////////////////////////////////////////////////

	int StatsSystem::getPriority() const
	{
		return 1;
	}

	//////////////////////////////////////////////////////////////////////////
}