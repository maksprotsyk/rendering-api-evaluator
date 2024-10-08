#include "RenderingSystem.h"

#include <codecvt>

#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Components/JsonData.h"
#include "Utils/BasicUtils.h"


namespace Engine::Systems
{
	RenderingSystem::RenderingSystem(const Visual::Window& window): _window(window)
	{
	}
	void RenderingSystem::onStart()
	{
		auto& compManager = ComponentsManager::get();

		const auto& tagSet = compManager.getComponentSet<Components::Tag>();
		const auto& jsonDataSet = compManager.getComponentSet<Components::JsonData>();
		for (EntityID id: compManager.entitiesWithComponents<Components::JsonData, Components::Tag>())
		{
			const std::string& tag = tagSet.getElement(id).tag;
			if (tag == "Config")
			{
				const nlohmann::json& data = jsonDataSet.getElement(id).data;
				if (data.contains("Renderer"))
				{
					const std::string& renderer = data["Renderer"];
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
				break;
			}
		}

		if (!_renderer)
		{
			_renderer = std::make_unique<Visual::DirectXRenderer>();
		}

		_renderer->init(_window);


		auto& modelSet = compManager.getComponentSet<Components::Model>();
		auto& transformSet = compManager.getComponentSet<Components::Transform>();
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);

			model.model = _renderer->createModel();
			_renderer->loadModel(*model.model, model.path);

			const Components::Transform& transform = transformSet.getElement(id);
			_renderer->transformModel(*model.model, transform.position, transform.rotation, transform.scale);

			_renderer->createBuffersFromModel(*model.model);
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
		auto& compManager = ComponentsManager::get();
		const auto& cameraTransform = compManager.getComponentSet<Components::Transform>().getElement(_cameraId);
		_renderer->setCameraProperties(cameraTransform.position, cameraTransform.rotation);

		auto& modelSet = compManager.getComponentSet<Components::Model>();
		const auto& transformSet = compManager.getComponentSet<Components::Transform>();

		_renderer->clearBackground();
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);
			const Components::Transform& transform = transformSet.getElement(id);
			_renderer->transformModel(*model.model, transform.position, transform.rotation, transform.scale);
			_renderer->draw(*model.model);
		}
		_renderer->render();
	}

	void RenderingSystem::onStop()
	{

	}

	int RenderingSystem::getPriority() const
	{
		return 0;
	}
	
}