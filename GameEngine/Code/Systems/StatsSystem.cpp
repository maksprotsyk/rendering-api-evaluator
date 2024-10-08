#include "StatsSystem.h"
#include "Managers/ComponentsManager.h"
#include "Managers/EventsManager.h"
#include "Events/NativeInputEvents.h"
#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Components/JsonData.h"

#include <iostream>
#include <fstream>

namespace Engine::Systems
{
	void StatsSystem::onStart()
	{
		firstUpdate = true;
	}

	void StatsSystem::onUpdate(float dt)
	{
		if (firstUpdate)
		{
			creationTime = dt;
			firstUpdate = false;
			return;
		}

		frameTimes.push_back(dt);

		if (frameTimes.size() >= 1000)
		{
			EventsManager::get().emit<Events::NativeExitRequested>({});
		}

	}

	void StatsSystem::onStop()
	{
		auto& compManager = ComponentsManager::get();

		const auto& tagSet = compManager.getComponentSet<Components::Tag>();
		const auto& jsonDataSet = compManager.getComponentSet<Components::JsonData>();

		std::string rendererName = "DirectX";
		std::string outputPath = "";
		for (EntityID id : compManager.entitiesWithComponents<Components::JsonData, Components::Tag>())
		{
			const std::string& tag = tagSet.getElement(id).tag;
			if (tag == "Config")
			{
				const nlohmann::json& data = jsonDataSet.getElement(id).data;
				if (data.contains("Renderer"))
				{
					rendererName = data["Renderer"];
				}
				if (data.contains("OutputStatsFile"))
				{
					outputPath = data["OutputStatsFile"];
				}
				break;
			}
		}

		if (outputPath.empty())
		{
			return;
		}
		
		auto const& models = compManager.getComponentSet<Components::Model>();
		size_t objectsCount = models.size();
		size_t totalNumberOfVertices = 0;
		for (const auto& model : models.getElements())
		{
			totalNumberOfVertices += model.model->GetVertexCount();
		}

		std::sort(frameTimes.begin(), frameTimes.end());
		float medianFrameTime = frameTimes[frameTimes.size() / 2];
		float averageFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
		
		size_t fivePercents = frameTimes.size() / 20;
		float percentile95 = frameTimes[frameTimes.size() - fivePercents];
		float percentile5 = frameTimes[fivePercents];


		std::ofstream outFile(outputPath);

		if (!outFile.is_open()) {
			return;
		}

		outFile << "Renderer: " << rendererName << std::endl;
		outFile << "Objects count: " << objectsCount << std::endl;
		outFile << "Total number of vertices: " << totalNumberOfVertices << std::endl;
		outFile << "Creation time: " << creationTime << std::endl;
		outFile << "Median frame time: " << medianFrameTime << std::endl;
		outFile << "Average frame time: " << averageFrameTime << std::endl;
		outFile << "95th percentile frame time: " << percentile95 << std::endl;
		outFile << "5th percentile frame time: " << percentile5 << std::endl;
	}

	int StatsSystem::getPriority() const
	{
		return 1;
	}
}