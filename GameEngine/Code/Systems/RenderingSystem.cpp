#include "RenderingSystem.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

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
		m_rendererCreators["DirectX"] = []() { return std::make_unique<Visual::DirectXRenderer>(); };
		m_rendererCreators["Vulkan"] = []() { return std::make_unique<Visual::VulkanRenderer>(); };
		m_rendererCreators["OpenGL"] = []() { return std::make_unique<Visual::OpenGLRenderer>(); };

		m_rendererNames.reserve(m_rendererCreators.size());
		for (const auto& pair : m_rendererCreators)
		{
			m_rendererNames.push_back(pair.first);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onStart()
	{

#ifdef _SHOWUI
		initUI();
#endif
		if (m_config.contains("renderer"))
		{
			setRenderer(m_config["renderer"]);
		}
		else
		{
			setRenderer("");
		}

		m_nextRendererName = m_rendererName;

		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
		auto& tagSet = compManager.getComponentSet<Components::Tag>();

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
		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
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
				bool loadResult = m_renderer->loadModel(gameController.getConfigRelativePath(model.path));
				ASSERT(loadResult, "Failed to load model: {}", gameController.getConfigRelativePath(model.path));
				if (!loadResult)
				{
					continue;
				}
				model.instance = m_renderer->createModelInstance(gameController.getConfigRelativePath(model.path));
			}

			const Components::Transform& transform = transformSet.getElement(id);
			m_renderer->draw(*model.instance, transform.position, transform.rotation, transform.scale);
		}

#ifdef _SHOWUI
		m_renderer->startUIRender();
		renderUI();
		m_renderer->endUIRender();
#endif

		m_renderer->render();

		if (m_nextRendererName != m_rendererName)
		{
			removeRenderer();
			setRenderer(m_nextRendererName);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onStop()
	{
		removeRenderer();
#ifdef _SHOWUI
		cleanUpUI();
#endif
	}

	//////////////////////////////////////////////////////////////////////////

	int RenderingSystem::getPriority() const
	{
		return 10;
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::initUI()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(m_window.getHandle());
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::renderUI()
	{
		ImGui_ImplWin32_NewFrame(); // or ImGui_ImplGLFW_NewFrame();
		ImGui::NewFrame();

		// Choose corner to attach (e.g., top-left)
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos; // Start of usable region
		ImVec2 work_size = viewport->WorkSize;

		ImVec2 window_pos = ImVec2(work_pos.x + 10, work_pos.y + 10); // Top-left with padding
		ImVec2 window_pos_pivot = ImVec2(0.0f, 0.0f);

		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::SetNextWindowBgAlpha(0.75f); // Optional: transparent bg

		if (ImGui::Begin("Performance Monitor", nullptr,
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
		{
			if (ImGui::BeginCombo("Renderer API", m_rendererName.c_str()))
			{
				for (int i = 0; i < m_rendererNames.size(); i++)
				{
					bool is_selected = (m_rendererNames[i] == m_rendererName);
					if (ImGui::Selectable(m_rendererNames[i].c_str(), is_selected))
					{
						m_nextRendererName = m_rendererNames[i];
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();

			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Text("Frame Time: %.2f ms", 1000.0f / ImGui::GetIO().Framerate);
			ImGui::Text("Memory Usage: %.2f MB", 123.45); // Replace with real value

			ImGui::Separator();

			static char file1[256] = "";
			static char file2[256] = "";

			ImGui::InputText("Model File", file1, IM_ARRAYSIZE(file1));
			ImGui::SameLine();
			if (ImGui::Button("Browse##1")) {
				// Call your file dialog here
			}

			ImGui::InputText("Texture File", file2, IM_ARRAYSIZE(file2));
			ImGui::SameLine();
			if (ImGui::Button("Browse##2")) {
				// Call your file dialog here
			}
		}

		ImGui::End();
		ImGui::Render();
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::cleanUpUI()
	{
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::removeRenderer()
	{
		auto& compManager = GameController::get().getComponentsManager();

		auto& modelSet = compManager.getComponentSet<Components::Model>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);
			if (model.instance)
			{
				m_renderer->destroyModelInstance(*model.instance);
				model.instance = nullptr;
			}
		}

		m_renderer->cleanUp();
		m_renderer = nullptr;
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::setRenderer(const std::string& rendererName)
	{
		const auto& itr = m_rendererCreators.find(rendererName);
		if (itr != m_rendererCreators.end())
		{
			m_renderer = itr->second();
			m_rendererName = rendererName;
		}

		if (!m_renderer)
		{
			const auto& itr = m_rendererCreators.begin();
			m_renderer = itr->second();
			m_rendererName = itr->first;
		}

		m_renderer->init(m_window);

		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
		auto& modelSet = compManager.getComponentSet<Components::Model>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);

			bool loadResult = m_renderer->loadModel(gameController.getConfigRelativePath(model.path));
			ASSERT(loadResult, "Failed to load model: {}", gameController.getConfigRelativePath(model.path));
			if (!loadResult)
			{
				continue;
			}

			model.instance = m_renderer->createModelInstance(gameController.getConfigRelativePath(model.path));
		}
	}

	//////////////////////////////////////////////////////////////////////////
	
}