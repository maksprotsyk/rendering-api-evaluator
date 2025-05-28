#pragma once

#include <string>

namespace Engine::Events
{

	struct RendererUpdate
	{
		std::string rendererName;
	};

	struct ConfigFileUpdate
	{
		std::string configPath;
	};
}

