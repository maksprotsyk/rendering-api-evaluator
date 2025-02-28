#include "RenderingSystem.h"

#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Utils/BasicUtils.h"
#include "Utils/DebugMacros.h"
#include "Managers/GameController.h"

REGISTER_SYSTEM(Engine::Systems::RenderingSystem);

namespace Engine::Systems
{
	//////////////////////////////////////////////////////////////////////////

	RenderingSystem::RenderingSystem(): m_window(GameController::get().getWindow())
	{
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onStart()
	{
		if (m_config.contains("renderer"))
		{
			const std::string& renderer = m_config["renderer"];
			if (renderer == "DirectX")
			{
				m_renderer = std::make_unique<Visual::DirectXRenderer>();
			}
			else if (renderer == "Vulkan")
			{
				m_renderer = std::make_unique<Visual::VulkanRenderer>();
			}
			else if (renderer == "OpenGL")
			{
				m_renderer = std::make_unique<Visual::OpenGLRenderer>();
			}
		}

		if (!m_renderer)
		{
			m_renderer = std::make_unique<Visual::DirectXRenderer>();
		}

		m_renderer->init(m_window);


		auto& compManager = GameController::get().getComponentsManager();
		auto& modelSet = compManager.getComponentSet<Components::Model>();
		auto& tagSet = compManager.getComponentSet<Components::Tag>();
		auto& transformSet = compManager.getComponentSet<Components::Transform>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);

			bool loadResult = m_renderer->loadModel(model.path);
			ASSERT(loadResult, "Failed to load model: {}", model.path);
			if (!loadResult)
			{
				continue;
			}

			model.instance = m_renderer->createModelInstance(model.path);
		}


		for (EntityID id : compManager.entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				m_cameraId = id;
			}
		}

	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onUpdate(float dt)
	{
		auto& compManager = GameController::get().getComponentsManager();
		const auto& cameraTransform = compManager.getComponentSet<Components::Transform>().getElement(m_cameraId);
		m_renderer->setCameraProperties(cameraTransform.position, cameraTransform.rotation);

		auto& modelSet = compManager.getComponentSet<Components::Model>();
		const auto& transformSet = compManager.getComponentSet<Components::Transform>();

		m_renderer->clearBackground(0.0f, 0.2f, 0.4f, 1.0f);
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{

			Components::Model& model = modelSet.getElement(id);
			if (model.markedForDestroy)
			{
				m_renderer->destroyModelInstance(*model.instance);
				modelSet.removeElement(id);
				continue;
			}
			
			if (!model.instance)
			{
				model.instance = m_renderer->createModelInstance(model.path);
			}

			const Components::Transform& transform = transformSet.getElement(id);
			m_renderer->draw(*model.instance, transform.position, transform.rotation, transform.scale);
		}
		m_renderer->render();
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onStop()
	{
		m_renderer->cleanUp();
	}

	//////////////////////////////////////////////////////////////////////////

	int RenderingSystem::getPriority() const
	{
		return 0;
	}

	//////////////////////////////////////////////////////////////////////////
	
}