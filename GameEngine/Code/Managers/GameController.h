#pragma once

#include "nlohmann/json.hpp"

#include "EventsManager.h"
#include "ComponentsManager.h"
#include "SystemsManager.h"
#include "EntitiesManager.h"

#include "Visual/Window.h"

namespace Engine
{
	class GameController
	{
	public:
		static GameController& get();

		void setWindow(const Visual::Window& window);
		void setConfig(const std::string& configPath);
		const std::string& getConfigRelativePath(const std::string& path) const;

		void init();
		void run();
		void clear();

		EventsManager& getEventsManager();
		ComponentsManager& getComponentsManager();
		SystemsManager& getSystemsManager();
		EntitiesManager& getEntitiesManager();

		const EventsManager& getEventsManager() const;
		const ComponentsManager& getComponentsManager() const;
		const SystemsManager& getSystemsManager() const;
		const EntitiesManager& getEntitiesManager() const;

	private:
		GameController() = default;
		void initEntities();
		void initSystems();

	private:
		static std::unique_ptr<GameController> m_instance;

		Visual::Window m_window;
		nlohmann::json m_config;
		std::string m_configPath;

		EventsManager m_eventsManager;
		ComponentsManager m_componentsManager;
		SystemsManager m_systemsManager;
		EntitiesManager m_entitiesManager;

	};


	template <typename Component>
	class ComponentRegisterer
	{
	public:
		explicit ComponentRegisterer();
	};

	template <typename Component, typename Serializer>
	class SerializableComponentRegisterer
	{
	public:
		explicit SerializableComponentRegisterer();
	};

	template <typename Component>
	ComponentRegisterer<Component>::ComponentRegisterer()
	{
		GameController::get().getComponentsManager().registerComponent<Component>();
	}

	template <typename Component, typename Serializer>
	SerializableComponentRegisterer<Component, Serializer>::SerializableComponentRegisterer()
	{
		GameController::get().getComponentsManager().registerComponent<Component, Serializer>();
	}
}

#define REGISTER_COMPONENT(T) \
static const Engine::ComponentRegisterer<T> reg = Engine::ComponentRegisterer<T>();

#define REGISTER_SERIALIZABLE_COMPONENT(T) \
static const Engine::SerializableComponentRegisterer<T, T> reg = Engine::SerializableComponentRegisterer<T, T>();

#define REGISTER_SERIALIZABLE_COMPONENT_S(T, S) \
static const Engine::SerializableComponentRegisterer<T, S> reg = Engine::SerializableComponentRegisterer<T, S>();
