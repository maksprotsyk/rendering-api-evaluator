#include "ISystem.h"

namespace Engine::Systems
{
	void ISystem::setConfig(const nlohmann::json& config)
	{
		m_config = config;
	}
}