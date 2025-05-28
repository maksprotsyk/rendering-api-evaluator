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
		const Visual::Window& getWindow() const;
		void setConfig(const std::string& configPath);
		std::string getConfigRelativePath(const std::string& path) const;
		std::string getConfigPath() const;

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

		EntityID createPrefab(const std::string& prefabName);


	private:
		GameController() = default;
		
		void createEntity(const nlohmann::json& entityJson);
		void initPrefabs();
		void initEntities();
		void initSystems();

	private:
		static constexpr const char* k_prefabsField = "Prefabs";
		static constexpr const char* k_entitiesField = "Entities";
		static constexpr const char* k_systemsField = "Systems";
		static constexpr const char* k_nameField = "Name";
		static constexpr const char* k_prefabField = "Prefab";
		static constexpr const char* k_componentsField = "Components";

		static std::unique_ptr<GameController> m_instance;

		Visual::Window m_window;
		nlohmann::json m_config;
		std::string m_configPath;
		std::unordered_map<std::string, nlohmann::json> m_prefabs;

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

	template <typename System>
	class SystemRegisterer
	{
	public:
		explicit SystemRegisterer();
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
	template<typename System>
	SystemRegisterer<System>::SystemRegisterer()
	{
		GameController::get().getSystemsManager().registerSystem<System>();
	}
}

#define REGISTER_COMPONENT(T) \
static const Engine::ComponentRegisterer<T> reg = Engine::ComponentRegisterer<T>();

#define REGISTER_SERIALIZABLE_COMPONENT(T) \
static const Engine::SerializableComponentRegisterer<T, T> reg = Engine::SerializableComponentRegisterer<T, T>();

#define REGISTER_SERIALIZABLE_COMPONENT_S(T, S) \
static const Engine::SerializableComponentRegisterer<T, S> reg = Engine::SerializableComponentRegisterer<T, S>();

#define REGISTER_SYSTEM(T) \
static const Engine::SystemRegisterer<T> reg = Engine::SystemRegisterer<T>();