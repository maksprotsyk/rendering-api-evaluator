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
	RenderingSystem::RenderingSystem(): _window(GameController::get().getWindow())
	{
	}

	void RenderingSystem::onStart()
	{
		if (m_config.contains("Renderer"))
		{
			const std::string& renderer = m_config["Renderer"];
			if (renderer == "DirectX")
			{
				_renderer = std::make_unique<Visual::DirectXRenderer>();
			}
			else if (renderer == "Vulkan")
			{
				_renderer = std::make_unique<Visual::VulkanRenderer>();
			}
			else if (renderer == "OpenGL")
			{
				_renderer = std::make_unique<Visual::OpenGLRenderer>();
			}
		}

		if (!_renderer)
		{
			_renderer = std::make_unique<Visual::DirectXRenderer>();
		}

		_renderer->init(_window);


		auto& compManager = GameController::get().getComponentsManager();
		auto& modelSet = compManager.getComponentSet<Components::Model>();
		auto& tagSet = compManager.getComponentSet<Components::Tag>();
		auto& transformSet = compManager.getComponentSet<Components::Transform>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);

			bool loadResult = _renderer->loadModel(model.path);
			ASSERT(loadResult, "Failed to load model: {}", model.path);
			if (!loadResult)
			{
				continue;
			}

			model.instance = _renderer->createModelInstance(model.path);
		}


		for (EntityID id : compManager.entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				_cameraId = id;
			}
		}

	}

	void RenderingSystem::onUpdate(float dt)
	{
		auto& compManager = GameController::get().getComponentsManager();
		const auto& cameraTransform = compManager.getComponentSet<Components::Transform>().getElement(_cameraId);
		_renderer->setCameraProperties(cameraTransform.position, cameraTransform.rotation);

		auto& modelSet = compManager.getComponentSet<Components::Model>();
		const auto& transformSet = compManager.getComponentSet<Components::Transform>();

		_renderer->clearBackground(0.0f, 0.2f, 0.4f, 1.0f);
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{

			Components::Model& model = modelSet.getElement(id);
			if (model.markedForDestroy)
			{
				_renderer->destroyModelInstance(*model.instance);
				modelSet.removeElement(id);
				continue;
			}
			
			if (!model.instance)
			{
				model.instance = _renderer->createModelInstance(model.path);
			}

			const Components::Transform& transform = transformSet.getElement(id);
			_renderer->draw(*model.instance, transform.position, transform.rotation, transform.scale);
		}
		_renderer->render();
	}

	void RenderingSystem::onStop()
	{
		_renderer->cleanUp();
	}

	int RenderingSystem::getPriority() const
	{
		return 0;
	}
	
}