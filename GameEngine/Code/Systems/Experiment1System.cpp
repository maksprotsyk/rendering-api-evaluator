#include "Experiment1System.h"

#include <numbers>
#include <algorithm>

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

		ASSERT(m_config.contains("radiuses"), "radiuses not found in config");
		if (m_config.contains("radiuses"))
		{
			m_radiuses = m_config["radiuses"].get<std::vector<float>>();
		}

		ASSERT(m_config.contains("cameraSpeed"), "cameraSpeed not found in config");
		if (m_config.contains("cameraSpeed"))
		{
			m_cameraSpeed = m_config["cameraSpeed"].get<float>();
		}

		ASSERT(m_config.contains("cameraMaxDistance"), "cameraMaxDistance not found in config");
		if (m_config.contains("cameraMaxDistance"))
		{
			m_cameraMaxDistance = m_config["cameraMaxDistance"].get<float>();
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
				transform.position.y = radius * std::sin(currentAngle);
				currentAngle += angleStep;

				tagSet.addElement(id, Components::Tag{ k_experimentObjectTag });
				objects.push_back(id);
			}
			moveClockwise = !moveClockwise;
		}

		for (EntityID id : compManager.entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				m_cameraId = id;
			}
		}

		Components::Transform& transform = transformSet.getElement(m_cameraId);
		m_originalCameraPosition = transform.position.z;
		m_cameraMoveForwards = true;
	}

	//////////////////////////////////////////////////////////////////////////

	void Experiment1System::onUpdate(float dt)
	{
		ExperimentSystemBase::onUpdate(dt);
		rotateObjects(dt);
		moveCamera(dt);
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
			transform.getElement(id).position.rotateArroundVector(Utils::Vector3(0, 0, 1), m_rotationSpeed * dt);
		}

		for (EntityID id : m_counterClockwiseObjects)
		{
			transform.getElement(id).position.rotateArroundVector(Utils::Vector3(0, 0, 1), -m_rotationSpeed * dt);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void Experiment1System::moveCamera(float dt)
	{
		ComponentsManager& compManager = GameController::get().getComponentsManager();
		Utils::SparseSet<Components::Transform, EntityID>& transform = compManager.getComponentSet<Components::Transform>();
		Components::Transform& cameraTransform = transform.getElement(m_cameraId);
		if (m_cameraMoveForwards)
		{
			cameraTransform.position.z = std::fminf(cameraTransform.position.z + m_cameraSpeed * dt, m_cameraMaxDistance + m_originalCameraPosition);
		}
		else
		{
			cameraTransform.position.z = std::fmaxf(cameraTransform.position.z - m_cameraSpeed * dt, -m_cameraMaxDistance + m_originalCameraPosition);
		}

		if (cameraTransform.position.z >= m_cameraMaxDistance + m_originalCameraPosition)
		{
			m_cameraMoveForwards = false;
		}
		else if (cameraTransform.position.z <= -m_cameraMaxDistance + m_originalCameraPosition)
		{
			m_cameraMoveForwards = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
}