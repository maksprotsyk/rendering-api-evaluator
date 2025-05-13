#pragma once

#include <string>

#include "Visual/Window.h"
#include "Events/StatsEvents.h"

namespace Engine::Visual
{

	class UIController
	{
	public:
		UIController(std::vector<std::string> rendererNames);
		void init();
		void setRenderer(const std::string& rendererName);
		void render();
		void cleanUp();
	private:
		const Visual::Window& m_window;
		std::vector<std::string> m_rendererNames;
		std::string m_rendererName;
		std::string m_outputFile;
		bool m_isRecoring;
		StatsData m_statsData{};
	};

}