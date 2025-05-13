#include "UIController.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

#include "Managers/GameController.h"
#include "Events/UIEvents.h"
#include "Events/StatsEvents.h"

namespace Engine::Visual
{

    ////////////////////////////////////////////////////////////////////////

	static std::wstring OpenFileDialog(const std::wstring& formats)
	{
		OPENFILENAMEW ofn;
		wchar_t szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = formats.data();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameW(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return L"";
	}

	//////////////////////////////////////////////////////////////////////////

	static std::wstring SaveFileDialog(const std::wstring& formats)
	{
		OPENFILENAMEW ofn;
		wchar_t szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr; 
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = formats.data();
		ofn.nFilterIndex = 1;
		ofn.lpstrDefExt = L"txt";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (GetSaveFileNameW(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}
		return L"";
	}


	UIController::UIController(std::vector<std::string> rendererNames)
		: m_window(GameController::get().getWindow())
		, m_rendererName("")
		, m_outputFile("")
		, m_rendererNames(std::move(rendererNames))
	{
		GameController::get().getEventsManager().subscribe<Events::StatsUpdate>(
			[this](const Events::StatsUpdate& i_statsEvent)
			{
				m_statsData = i_statsEvent.stats;
			}
		);

	}

	////////////////////////////////////////////////////////////////////////

	void UIController::init()
    {
		GameController::get().getEventsManager().emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{false});
		m_isRecoring = false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(m_window.getHandle());
    
    }

	////////////////////////////////////////////////////////////////////////

	void UIController::setRenderer(const std::string& rendererName)
	{
		m_rendererName = rendererName;
	}

    ////////////////////////////////////////////////////////////////////////

    void UIController::render()
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
						GameController::get().getEventsManager().emit(Events::RendererUpdate{ m_rendererNames[i] });
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::Separator();

			ImGui::Text("FPS: %.1f", m_statsData.avgFPS);
			ImGui::Text("Frame Time: %.3f ms", 1000.0f * m_statsData.avgFrameTime);
			ImGui::Text("99th Percentile Frame Time: %.3f ms", 1000.0f * m_statsData.frameTimePercentile99);
			ImGui::Text("RAM Usage: %.2f MB", m_statsData.memoryUsage);
			ImGui::Text("VRAM Usage: %.2f MB", m_statsData.gpuMemoryUsage);
			ImGui::Text("CPU Usage: %.2f%%", m_statsData.cpuUsage);
			ImGui::Text("GPU Usage: %.2f%%", m_statsData.gpuUsage);

			ImGui::Separator();

			static char file1[256] = "";
			static char file2[256] = "";

			if (ImGui::Button("Select stats output file"))
			{
				std::string res = Utils::wstringToString(SaveFileDialog(TEXT(".txt")));
				if (!res.empty())
				{
					m_outputFile = res;
					GameController::get().getEventsManager().emit<Events::StatsOutputFileUpdate>(Events::StatsOutputFileUpdate{ m_outputFile });
				}
			}

			ImGui::Text("Output file: %s", m_outputFile.c_str());
			if (ImGui::Button("Start/Stop recording"))
			{
				m_isRecoring = !m_isRecoring;
				GameController::get().getEventsManager().emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{ m_isRecoring });
			}
			if (m_isRecoring)
			{
				ImGui::Text("Recording: ON");
			}
			else
			{
				ImGui::Text("Recording: OFF");
			}


			//ImGui::InputText("Texture File", file2, IM_ARRAYSIZE(file2));
			//ImGui::SameLine();
			//if (ImGui::Button("Browse##2")) {
				// Call your file dialog here
			//}
		}

		ImGui::End();
		ImGui::Render();
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::cleanUp()
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    ////////////////////////////////////////////////////////////////////////
}