#include "Experiment1System.h"

#include <numbers>

#include "Managers/GameController.h"
#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Utils/DebugMacros.h"
#include "Utils/Quaternion.h"

REGISTER_SYSTEM(Engine::Systems::Experiment1System);

namespace Engine::Systems
{
	//////////////////////////////////////////////////////////////////////////

	void Experiment1System::onStart()
	{
		ExperimentSystemBase::onStart();

		ASSERT(m_config.contains("rotationSpeed"), "rotationSpeed not found in config");
		if (m_config.contains("rotationSpeed"))
		{
			m_rotationSpeed = m_config["rotationSpeed"].get<float>();
		}


		if (m_config.contains("radiuses"))
		{
			m_radiuses = m_config["radiuses"].get<std::vector<float>>();
		}

		GameController& gameController = GameController::get();
		ComponentsManager& compManager = gameController.getComponentsManager();
		Utils::SparseSet<Components::Transform, EntityID>& transformSet = compManager.getComponentSet<Components::Transform>();
		Utils::SparseSet<Components::Tag, EntityID>& tagSet = compManager.getComponentSet<Components::Tag>();
		
		float pi = std::numbers::pi_v<float>;
		float angleStep = 2 * pi / m_prefabsCount;
		float currentAngle = 0.0f;

		bool moveClockwise = true;
		for (float radius : m_radiuses)
		{
			std::vector<EntityID>& objects = moveClockwise ? m_clockwiseObjects : m_counterClockwiseObjects;
			for (size_t i = 0; i < m_prefabsCount; i++)
			{
				EntityID id = gameController.createPrefab(m_prefabName);
				Components::Transform& transform = transformSet.getElement(id);
				transform.position.x = radius * std::cos(currentAngle);
				transform.position.z = radius * std::sin(currentAngle);
				currentAngle += angleStep;

				tagSet.addElement(id, Components::Tag{ k_experimentObjectTag });
				objects.push_back(id);
			}
			moveClockwise = !moveClockwise;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void Experiment1System::onUpdate(float dt)
	{
		ExperimentSystemBase::onUpdate(dt);
		rotateObjects(dt);
	}

	//////////////////////////////////////////////////////////////////////////

	void Experiment1System::onStop()
	{

	}

	//////////////////////////////////////////////////////////////////////////

	int Experiment1System::getPriority() const
	{
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////

	void Experiment1System::rotateObjects(float dt)
	{
		ComponentsManager& compManager = GameController::get().getComponentsManager();
		Utils::SparseSet<Components::Transform, EntityID>& transform = compManager.getComponentSet<Components::Transform>();

		for (EntityID id : m_clockwiseObjects)
		{
			transform.getElement(id).position.rotateArroundVector(Utils::Vector3(0, 1, 0), m_rotationSpeed * dt);
		}

		for (EntityID id : m_counterClockwiseObjects)
		{
			transform.getElement(id).position.rotateArroundVector(Utils::Vector3(0, 1, 0), -m_rotationSpeed * dt);
		}
	}

	//////////////////////////////////////////////////////////////////////////
}