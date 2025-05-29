#pragma once

#include <Windows.h>

namespace Engine
{
	struct StatsData
	{
		float avgFPS;
		float avgFrameTime;
		float memoryUsage;
		float gpuMemoryUsage;
		float cpuUsage;
		float gpuUsage;
		float frameTimePercentile99;
	};
}

namespace Engine::Events
{

	struct StatsUpdate
	{
		StatsData stats;
	};

	struct StatsRecordingUpdate
	{
		bool recordData;
		std::string rendererName;
	};

	struct StatsOutputFileUpdate
	{
		std::string outputPath;
	};

}

