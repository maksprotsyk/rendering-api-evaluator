#include "ExperimentSystemBase.h"

#include "Managers/GameController.h"
#include "Events/NativeInputEvents.h"
#include "Utils/DebugMacros.h"

namespace Engine::Systems
{
	//////////////////////////////////////////////////////////////////////////

	void ExperimentSystemBase::onStart()
	{
		ASSERT(m_config.contains("prefab"), "prefab field not found in experiment config");
		if (m_config.contains("prefab"))
		{
			m_prefabName = m_config["prefab"].get<std::string>();
		}

		ASSERT(m_config.contains("prefabCount"), "prefabCount field not found in experiment config");
		if (m_config.contains("prefabCount"))
		{
			m_prefabsCount = m_config["prefabCount"].get<size_t>();
		}

		ASSERT(m_config.contains("experimentTime"), "experientTime field not found in experiment config");
		if (m_config.contains("experimentTime"))
		{
			m_timeLeft = m_config["experimentTime"].get<float>();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void ExperimentSystemBase::onUpdate(float dt)
	{
		checkExperimentEnd(dt);
	}

	//////////////////////////////////////////////////////////////////////////

	void ExperimentSystemBase::checkExperimentEnd(float dt)
	{
		m_timeLeft -= dt;
		if (m_timeLeft <= 0)
		{
			GameController::get().getEventsManager().emit(Engine::Events::NativeExitRequested{});
		}
	}

	//////////////////////////////////////////////////////////////////////////
}