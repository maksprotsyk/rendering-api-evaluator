#include "StatsSystem.h"
#include "Managers/ComponentsManager.h"
#include "Managers/EventsManager.h"
#include "Events/NativeInputEvents.h"
#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Components/FileLocation.h"

#include <iostream>
#include <fstream>

namespace Engine::Systems
{
	void StatsSystem::onStart()
	{

	}

	void StatsSystem::onUpdate(float dt)
	{
		updatesPerformed += 1;
		if (updatesPerformed < 2)
		{
			return;
		}
		if (updatesPerformed == 2)
		{
			creationTime = dt;
		}
		else
		{
			frameTimes.push_back(dt);
		}

		if (frameTimes.size() > 1000)
		{
			EventsManager::get().emit<Events::NativeExitRequested>({});
		}

	}

	void StatsSystem::onStop()
	{
		auto const& models = ComponentsManager::get().getComponentSet<Components::Model>();
		objectsCount = models.size();

		for (const auto& model : models.getElements())
		{
			totalNumberOfVertices += model.model->GetVertexCount();
		}

		auto& tagSet = ComponentsManager::get().getComponentSet<Components::Tag>();
		auto& fileSets = ComponentsManager::get().getComponentSet<Components::FileLocation>();
		std::string outputFilePath = "";

		for (EntityID id : tagSet.getIds())
		{
			if (tagSet.getElement(id).tag == "StatsConfiguration")
			{
				outputFilePath = fileSets.getElement(id).path;
			}
		}

		if (outputFilePath.empty())
		{
			return;
		}

		for (const Components::Tag& tag : tagSet.getElements())
		{
			if (tag.tag == "DirectX" || tag.tag == "Vulkan" || tag.tag == "OpenGL")
			{
				rendererName = tag.tag;
			}

		}

		if (rendererName.empty())
		{
			rendererName = "DirectX";
		}

		std::sort(frameTimes.begin(), frameTimes.end());
		float medianFrameTime = frameTimes[frameTimes.size() / 2];
		float averageFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
		
		size_t fivePercents = frameTimes.size() / 20;
		float percentile95 = frameTimes[frameTimes.size() - fivePercents];
		float percentile5 = frameTimes[fivePercents];


		std::ofstream outFile(outputFilePath);

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