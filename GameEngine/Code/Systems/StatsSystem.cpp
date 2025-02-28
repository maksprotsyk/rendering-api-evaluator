#include "StatsSystem.h"
#include "Managers/GameController.h"
#include "Events/NativeInputEvents.h"
#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"

#include <iostream>
#include <fstream>
#include <psapi.h>

REGISTER_SYSTEM(Engine::Systems::StatsSystem);

namespace Engine::Systems
{

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onStart()
	{
		m_firstUpdate = true;

		PdhOpenQuery(nullptr, 0, &m_cpuQuery);
		PdhAddCounter(m_cpuQuery, TEXT("\\Processor(_Total)\\% Processor Time"), 0, &m_cpuUsageCounter);

		//PdhOpenQuery(nullptr, 0, &gpuQuery);
		//PdhAddCounter(gpuQuery, TEXT("\\GPU Engine(*)\\Utilization Percentage"), 0, &gpuUsageCounter);

	}

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onUpdate(float dt)
	{
		if (m_firstUpdate)
		{
			creationTime = dt;
			m_firstUpdate = false;

			PdhCollectQueryData(m_cpuQuery);
			PDH_FMT_COUNTERVALUE counterVal;
			PdhGetFormattedCounterValue(m_cpuUsageCounter, PDH_FMT_DOUBLE, nullptr, &counterVal);
			m_cpuUsage.push_back((float)counterVal.doubleValue);


			//PdhCollectQueryData(gpuQuery);
			//PdhGetFormattedCounterValue(gpuUsageCounter, PDH_FMT_DOUBLE, nullptr, &counterVal);
			//gpuUsage.push_back((float)counterVal.doubleValue);

			// Memory usage data collection
			PROCESS_MEMORY_COUNTERS memCounter;
			if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter)))
			{
				m_memoryUsage.push_back(memCounter.WorkingSetSize / (1024.0 * 1024.0)); // in MB
			}

			return;
		}

		m_frameTimes.push_back(dt);
		if (m_frameTimes.size() >= 1000)
		{
			//EventsManager::get().emit<Events::NativeExitRequested>({});
		}

	}

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onStop()
	{
		//PdhCloseQuery(gpuQuery);
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
		//float averageGPUUsage = std::accumulate(gpuUsage.begin(), gpuUsage.end(), 0.0) / gpuUsage.size();
		float averageMemoryUsage = std::accumulate(m_memoryUsage.begin(), m_memoryUsage.end(), 0.0) / m_memoryUsage.size();


		std::ofstream outFile(outputPath);

		if (!outFile.is_open()) {
			return;
		}

		outFile << "Renderer: " << rendererName << std::endl;
		outFile << "Objects count: " << objectsCount << std::endl;
		outFile << "Total number of vertices: " << totalNumberOfVertices << std::endl;
		outFile << "Creation time: " << creationTime << std::endl;
		outFile << "Average frame time: " << averageFrameTime << std::endl;
		outFile << "Average CPU usage: " << averageCPUUsage << std::endl;
		//outFile << "Average GPU usage: " << averageGPUUsage << std::endl;
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