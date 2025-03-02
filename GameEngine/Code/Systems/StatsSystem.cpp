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

		PDH_STATUS gpuOpenRes = PdhOpenQuery(nullptr, 0, &m_gpuUsageQuery);
		ASSERT(gpuOpenRes == ERROR_SUCCESS, "Failed to open GPU query");
		PDH_STATUS gpuAddRes = PdhAddCounter(m_gpuUsageQuery, TEXT("\\GPU Engine(*_3D)\\Utilization Percentage"), 0, &m_gpuUsageCounter);
		ASSERT(gpuAddRes == ERROR_SUCCESS, "Failed to add GPU counter");

		PDH_STATUS gpuCollectRes = PdhCollectQueryData(m_gpuUsageQuery);
		ASSERT(gpuCollectRes == ERROR_SUCCESS, "Failed to collect GPU query data");

		PDH_STATUS gpuMemoryOpenRes = PdhOpenQuery(nullptr, 0, &m_gpuMemoryUsageQuery);
		ASSERT(gpuMemoryOpenRes == ERROR_SUCCESS, "Failed to open GPU query");
		PDH_STATUS gpuAddMemoryRes = PdhAddCounter(m_gpuMemoryUsageQuery, TEXT("\\GPU Process Memory(*)\\Dedicated Usage"), 0, &m_gpuMemoryUsageCounter);
		ASSERT(gpuAddMemoryRes == ERROR_SUCCESS, "Failed to add GPU counter");

		PDH_STATUS gpuMemoryCollectRes = PdhCollectQueryData(m_gpuMemoryUsageQuery);
		ASSERT(gpuMemoryCollectRes == ERROR_SUCCESS, "Failed to collect GPU query data");

		Sleep(k_initialSleepTime * 1000);
	}

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onUpdate(float dt)
	{
		if (m_firstUpdate)
		{
			m_creationTime = dt - k_initialSleepTime;
			m_firstUpdate = false;
			m_timePassed = 0.0f;
			return;
		}

		m_frameTimes.push_back(dt);

		m_timePassed += dt;
		if (m_timePassed > k_timeBetweenSamples)
		{
			m_timePassed = 0.0f;

			PDH_STATUS res = PdhCollectQueryData(m_cpuQuery);
			ASSERT(res == ERROR_SUCCESS, "Failed to collect CPU query data");
			if (res == ERROR_SUCCESS)
			{
				PDH_FMT_COUNTERVALUE cpuCounterVal;
				res = PdhGetFormattedCounterValue(m_cpuUsageCounter, PDH_FMT_DOUBLE, nullptr, &cpuCounterVal);
				ASSERT(res == ERROR_SUCCESS, "Failed to format CPU query data");
				if (res == ERROR_SUCCESS)
				{
					m_cpuUsage.push_back((float)cpuCounterVal.doubleValue);
				}
			}

			res = PdhCollectQueryData(m_gpuUsageQuery);
			ASSERT(res == ERROR_SUCCESS, "Failed to collect GPU query data");
			if (res == ERROR_SUCCESS)
			{
				PDH_FMT_COUNTERVALUE gpuLoadCounterVal;
				res = PdhGetFormattedCounterValue(m_gpuUsageCounter, PDH_FMT_DOUBLE, nullptr, &gpuLoadCounterVal);
				ASSERT(res == ERROR_SUCCESS, "Failed to format GPU query data");
				if (res == ERROR_SUCCESS)
				{
					m_gpuUsage.push_back((float)gpuLoadCounterVal.doubleValue);
				}
			}

			res = PdhCollectQueryData(m_gpuMemoryUsageQuery);
			ASSERT(res == ERROR_SUCCESS, "Failed to collect GPU query data");
			if (res == ERROR_SUCCESS)
			{
				PDH_FMT_COUNTERVALUE gpuMemoryLoadCounterVal;
				res = PdhGetFormattedCounterValue(m_gpuMemoryUsageCounter, PDH_FMT_DOUBLE, nullptr, &gpuMemoryLoadCounterVal);
				ASSERT(res == ERROR_SUCCESS, "Failed to format GPU query data");
				if (res == ERROR_SUCCESS)
				{
					m_gpuMemoryUsage.push_back((float)gpuMemoryLoadCounterVal.doubleValue / (1024.0 * 1024.0));
				}
			}


			// Memory usage data collection
			PROCESS_MEMORY_COUNTERS memCounter;
			if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter)))
			{
				m_memoryUsage.push_back(memCounter.WorkingSetSize / (1024.0 * 1024.0)); // in MB
			}
		}

	}

	//////////////////////////////////////////////////////////////////////////

	void StatsSystem::onStop()
	{
		PdhCloseQuery(m_gpuUsageQuery);
		PdhCloseQuery(m_cpuQuery);
		PdhCloseQuery(m_gpuMemoryUsageQuery);

		std::string rendererName = m_config["renderer"];
		std::string outputPath = m_config["outputFile"];

		if (outputPath.empty())
		{
			return;
		}

		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
		auto const& models = compManager.getComponentSet<Components::Model>();
		size_t objectsCount = models.size();
		size_t totalNumberOfVertices = 0;

		std::sort(m_frameTimes.begin(), m_frameTimes.end());
		float medianFrameTime = m_frameTimes[m_frameTimes.size() / 2];
		float averageFrameTime = std::accumulate(m_frameTimes.begin(), m_frameTimes.end(), 0.0f) / m_frameTimes.size();
		
		size_t onePercent = m_frameTimes.size() / 100;
		float percentile99 = onePercent > 0? m_frameTimes[m_frameTimes.size() - onePercent]: m_frameTimes.back();
		float percentile1 = m_frameTimes[onePercent];

		float averageCPUUsage = std::accumulate(m_cpuUsage.begin(), m_cpuUsage.end(), 0.0) / m_cpuUsage.size();
		float maxCpuUsage = *std::max_element(m_cpuUsage.begin(), m_cpuUsage.end());
		float minCpuUsage = *std::min_element(m_cpuUsage.begin(), m_cpuUsage.end());
		float averageGPUUsage = std::accumulate(m_gpuUsage.begin(), m_gpuUsage.end(), 0.0) / m_gpuUsage.size();
		float maxGpuUsage = *std::max_element(m_gpuUsage.begin(), m_gpuUsage.end());
		float minGpuUsage = *std::min_element(m_gpuUsage.begin(), m_gpuUsage.end());
		float averageMemoryUsage = std::accumulate(m_memoryUsage.begin(), m_memoryUsage.end(), 0.0) / m_memoryUsage.size();
		float maxMemoryUsage = *std::max_element(m_memoryUsage.begin(), m_memoryUsage.end());
		float minMemoryUsage = *std::min_element(m_memoryUsage.begin(), m_memoryUsage.end());
		float averageGpuMemoryUsage = std::accumulate(m_gpuMemoryUsage.begin(), m_gpuMemoryUsage.end(), 0.0) / m_gpuMemoryUsage.size();
		float maxGpuMemoryUsage = *std::max_element(m_gpuMemoryUsage.begin(), m_gpuMemoryUsage.end());
		float minGpuMemoryUsage = *std::min_element(m_gpuMemoryUsage.begin(), m_gpuMemoryUsage.end());

		std::string filePath = gameController.getConfigRelativePath(outputPath);
		std::ofstream outFile(filePath);

		if (!outFile.is_open())
		{
			return;
		}

		outFile << "Renderer: " << rendererName << std::endl;
		outFile << "Objects count: " << objectsCount << std::endl;
		outFile << "Total number of vertices: " << totalNumberOfVertices << std::endl;
		outFile << "Creation time: " << m_creationTime << std::endl;
		outFile << "Average CPU usage: " << averageCPUUsage << std::endl;
		outFile << "Max CPU usage: " << maxCpuUsage << std::endl;
		outFile << "Min CPU usage: " << minCpuUsage << std::endl;
		outFile << "Average GPU usage: " << averageGPUUsage << std::endl;
		outFile << "Max GPU usage: " << maxGpuUsage << std::endl;
		outFile << "Min GPU usage: " << minGpuUsage << std::endl;
		outFile << "Average memory usage: " << averageMemoryUsage << std::endl;
		outFile << "Max memory usage: " << maxMemoryUsage << std::endl;
		outFile << "Min memory usage: " << minMemoryUsage << std::endl;
		outFile << "Average GPU memory usage: " << averageGpuMemoryUsage << std::endl;
		outFile << "Max GPU memory usage: " << maxGpuMemoryUsage << std::endl;
		outFile << "Min GPU memory usage: " << minGpuMemoryUsage << std::endl;
		outFile << "Average FPS: " << 1.0f / averageFrameTime << std::endl;
		outFile << "Average frame time: " << averageFrameTime << std::endl;
		outFile << "Median frame time: " << medianFrameTime << std::endl;
		outFile << "99th percentile frame time: " << percentile99 << std::endl;
		outFile << "1th percentile frame time: " << percentile1 << std::endl;
	}

	//////////////////////////////////////////////////////////////////////////

	int StatsSystem::getPriority() const
	{
		return 1;
	}

	//////////////////////////////////////////////////////////////////////////
}