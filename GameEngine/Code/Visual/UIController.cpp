#include "UIController.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

#include "Managers/GameController.h"
#include "Events/UIEvents.h"
#include "Events/StatsEvents.h"

namespace Engine::Visual
{

    ////////////////////////////////////////////////////////////////////////

    std::string shortenPath(const std::string& path, size_t maxLength)
    {
        if (path.length() <= maxLength)
        {
            return path;
        }

        size_t partLength = maxLength / 2 - 2;
        std::string shortened = path.substr(0, partLength) + "..." + path.substr(path.length() - partLength);
        return shortened;
    }

    //////////////////////////////////////////////////////////////////////////

	static std::wstring OpenFileDialog(const std::wstring& formats)
	{
        WCHAR originalDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, originalDir);

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

        std::wstring res = L"";
		if (GetOpenFileNameW(&ofn) == TRUE)
		{
            res = ofn.lpstrFile;
		}
        SetCurrentDirectoryW(originalDir);
		return res;
	}

	//////////////////////////////////////////////////////////////////////////

	static std::wstring SaveFileDialog(const std::wstring& formats)
	{
        WCHAR originalDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, originalDir);

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
        std::wstring res = L"";
		if (GetSaveFileNameW(&ofn) == TRUE)
		{
            res = ofn.lpstrFile;
		}
        SetCurrentDirectoryW(originalDir);
		return res;
	}

    //////////////////////////////////////////////////////////////////////////

	UIController::UIController(std::vector<std::string> rendererNames)
		: m_window(GameController::get().getWindow())
		, m_rendererName("")
		, m_outputFile("")
		, m_rendererNames(std::move(rendererNames))
	{

	}

	////////////////////////////////////////////////////////////////////////

	void UIController::init()
    {
        m_statsUpdateListenerId = GameController::get().getEventsManager().subscribe<Events::StatsUpdate>(
            [this](const Events::StatsUpdate& i_statsEvent)
            {
                m_statsData = i_statsEvent.stats;
            }
        );

		GameController::get().getEventsManager().emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{false});
		m_isRecording = false;

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

    void UIController::render(float dt)
    {
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
        ImGui::Begin("Performance Monitor", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

        if (ImGui::BeginTabBar("MainTabBar"))
        {
            // Performance Tab
            if (ImGui::BeginTabItem("Performance"))
            {
                ImGui::Text("FPS: %.1f", m_statsData.avgFPS);
                ImGui::Text("Frame Time: %.3f ms", 1000.0f * m_statsData.avgFrameTime);
                ImGui::Text("99th Percentile Frame Time: %.3f ms", 1000.0f * m_statsData.frameTimePercentile99);
                ImGui::Text("RAM Usage: %.2f MB", m_statsData.memoryUsage);
                ImGui::Text("VRAM Usage: %.2f MB", m_statsData.gpuMemoryUsage);
                ImGui::Text("CPU Usage: %.2f%%", m_statsData.cpuUsage);
                ImGui::Text("GPU Usage: %.2f%%", m_statsData.gpuUsage);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Recording Management"))
            {
                float width = ImGui::GetContentRegionAvail().x;

                ImGui::BeginGroup();

                if (ImGui::Button("Select output file", ImVec2(width, 20)))
                {
                    std::string res = Utils::wstringToString(SaveFileDialog(TEXT(".txt")));
                    if (!res.empty())
                    {
                        m_outputFile = res;
                        GameController::get().getEventsManager().emit<Events::StatsOutputFileUpdate>(Events::StatsOutputFileUpdate{ m_outputFile });
                    }
                }

                ImGui::Text("Output file: %s", shortenPath(m_outputFile, 30).c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", m_outputFile.c_str());
                    ImGui::EndTooltip();
                }

                ImGui::EndGroup();

                ImGui::Separator();

				ImGui::BeginGroup();

                if (ImGui::Button(m_isRecording ? "Stop recording": "Start recording", ImVec2(width, 20)))
                {
                    m_isRecording = !m_isRecording;
                    GameController::get().getEventsManager().emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{ m_isRecording });
                }

                if (m_isRecording)
                {
                    m_recordingTime += dt;
                }
                else
                {
					m_recordingTime = 0.0f;
                }

                ImGui::Text("Recording time: %.2f s", m_recordingTime);
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings"))
            {
                float width = ImGui::GetContentRegionAvail().x;

                ImGui::BeginGroup();
                if (ImGui::BeginCombo("Rendering API", m_rendererName.c_str()))
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
                ImGui::EndGroup();

                ImGui::Separator();

                ImGui::BeginGroup();
                if (ImGui::Button("Select config file", ImVec2(width, 20)))
                {
                    std::string newConfigFile = Utils::wstringToString(OpenFileDialog(TEXT(".json")));
                    if (!newConfigFile.empty())
                    {
						GameController::get().getEventsManager().emit<Events::ConfigFileUpdate>(Events::ConfigFileUpdate{ newConfigFile });
                    }
                }
				std::string configPath = GameController::get().getConfigPath();
                ImGui::Text("Config file: %s", shortenPath(configPath, 30).c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", configPath.c_str());
                    ImGui::EndTooltip();
                }
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }


        ImGui::End();
		ImGui::Render();
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::cleanUp()
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

		GameController::get().getEventsManager().unsubscribe<Events::StatsUpdate>(m_statsUpdateListenerId);
    }

    ////////////////////////////////////////////////////////////////////////
}