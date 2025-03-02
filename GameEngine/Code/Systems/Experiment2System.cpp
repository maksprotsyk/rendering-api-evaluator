#include "Experiment2System.h"

#include <numbers>

#include "Managers/GameController.h"
#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Utils/DebugMacros.h"
#include "Utils/Quaternion.h"

REGISTER_SYSTEM(Engine::Systems::Experiment2System);

namespace Engine::Systems
{
	//////////////////////////////////////////////////////////////////////////

	void Experiment2System::onStart()
	{
		ExperimentSystemBase::onStart();

		ASSERT(m_config.contains("distanceDelta"), "distanceDelta not found in config");
		if (m_config.contains("distanceDelta"))
		{
			m_distanceDelta = m_config["distanceDelta"].get<float>();
		}

		ASSERT(m_config.contains("elementsPerRow"), "elementsPerRow not found in config");
		if (m_config.contains("elementsPerRow"))
		{
			m_elementsPerRow = m_config["elementsPerRow"].get<size_t>();
		}

		GameController& gameController = GameController::get();
		ComponentsManager& compManager = gameController.getComponentsManager();
		Utils::SparseSet<Components::Transform, EntityID>& transformSet = compManager.getComponentSet<Components::Transform>();
		Utils::SparseSet<Components::Tag, EntityID>& tagSet = compManager.getComponentSet<Components::Tag>();
		
		float initialPosition = - (float)m_elementsPerRow / 2.0f * m_distanceDelta;
		size_t totalElements = 0;

		float currentZ = m_distanceDelta;
		while (totalElements < m_prefabsCount)
		{
			float currentY = initialPosition;
			for (size_t row = 0; row < m_elementsPerRow; row++)
			{
				float currentX = initialPosition + currentZ / 3.0f;
				for (size_t col = 0; col < m_elementsPerRow; col++)
				{
					EntityID id = gameController.createPrefab(m_prefabName);
					Components::Transform& transform = transformSet.getElement(id);
					transform.position.x = currentX;
					transform.position.y = currentY;
					transform.position.z = currentZ;
					tagSet.addElement(id, Components::Tag{ k_experimentObjectTag });

					totalElements++;
					if (totalElements >= m_prefabsCount)
					{
						break;
					}

					currentX += m_distanceDelta;
				}

				if (totalElements >= m_prefabsCount)
				{
					break;
				}

				currentY += m_distanceDelta;
			}

			currentZ += m_distanceDelta;
		}

	}

	//////////////////////////////////////////////////////////////////////////

	void Experiment2System::onStop()
	{

	}

	//////////////////////////////////////////////////////////////////////////

	int Experiment2System::getPriority() const
	{
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
}