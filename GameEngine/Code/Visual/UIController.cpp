#include "UIController.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

#include "Managers/GameController.h"
#include "Events/UIEvents.h"
#include "Events/StatsEvents.h"

namespace Engine::Visual
{

    //////////////////////////////////////////////////////////////////////////

	UIController::UIController(std::vector<std::string> rendererNames)
		: m_window(GameController::get().getWindow())
        , m_eventsManager(GameController::get().getEventsManager())
		, m_rendererName("")
		, m_outputFile("")
		, m_rendererNames(std::move(rendererNames))
	{

	}

	////////////////////////////////////////////////////////////////////////

	void UIController::init()
    {
        m_statsUpdateListenerId = m_eventsManager.subscribe<Events::StatsUpdate>(
            [this](const Events::StatsUpdate& i_statsEvent)
            {
                m_statsData = i_statsEvent.stats;
            }
        );

		m_warningListenerId = m_eventsManager.subscribe<Events::SendWarning>([this](const Events::SendWarning& i_event) {onWarningRequested(i_event.message);});

        m_eventsManager.emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{false});
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
        ImGui::Begin("Performance Monitor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginTabBar("MainTabBar"))
        {
            if (ImGui::BeginTabItem("Performance"))
            {
                renderPerformance();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Recording Management"))
            {
                renderStatsRecorder();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Settings"))
            {
                renderSettings();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }


        ImGui::End();

        if (m_isRecording)
        {
            m_recordingTime += dt;
        }
        else
        {
            m_recordingTime = 0.0f;
        }

        if (m_recordingTime > k_maxRecordingTime && m_isRecording)
        {
            onWarningRequested("Recording time exceeded the maximum limit of 60 seconds. Stopping recording.");
            m_isRecording = !m_isRecording;
            m_eventsManager.emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{ m_isRecording, m_rendererName });
        }
        
        if (m_warningTimer >= 0)
        {
			m_warningTimer -= dt;

            ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
            ImVec2 textSize = ImGui::CalcTextSize(m_warningMessage.c_str());


            ImVec2 windowPos = ImVec2((viewportSize.x - textSize.x) * 0.5f, (viewportSize.y - textSize.y) * 0.5f);

            ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.75f);

            ImGui::Begin(
                "##WarningPopup", 
                nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoInputs
            );

            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), m_warningMessage.c_str());

            ImGui::End();
        }

		ImGui::Render();
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::cleanUp()
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

		m_eventsManager.unsubscribe<Events::StatsUpdate>(m_statsUpdateListenerId);
		m_eventsManager.unsubscribe<Events::SendWarning>(m_warningListenerId);
    }


    ////////////////////////////////////////////////////////////////////////

    void UIController::onWarningRequested(const std::string& message)
    {
		m_warningMessage = message;
		m_warningTimer = k_warningTime;
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::renderPerformance()
    {
        drawStat("FPS:", "", m_statsData.avgFPS, 1);
        drawStat("Frame Time:", " ms", 1000.0f * m_statsData.avgFrameTime, 3);
        drawStat("99th Percentile Frame Time:", " ms", 1000.0f * m_statsData.frameTimePercentile99, 3);
        drawStat("RAM Usage:", " MB", m_statsData.memoryUsage, 2);
        drawStat("VRAM Usage:", " MB", m_statsData.gpuMemoryUsage, 2);
        drawStat("CPU Usage:", "%%", m_statsData.cpuUsage, 2);
        drawStat("GPU Usage:", "%%", m_statsData.gpuUsage, 2);
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::renderSettings()
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
                    m_eventsManager.emit(Events::RendererUpdate{ m_rendererNames[i] });
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
            std::string newConfigFile = Utils::wstringToString(Utils::openFileDialog(TEXT(".json")));
            if (!newConfigFile.empty())
            {
                m_eventsManager.emit<Events::ConfigFileUpdate>(Events::ConfigFileUpdate{ newConfigFile });
            }
            else
            {
                onWarningRequested("You haven't selected a valid config file");
            }
        }

        std::string configPath = GameController::get().getConfigPath();
        ImGui::Text("Config file: %s", Utils::shortenPath(configPath, 30).c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", configPath.c_str());
            ImGui::EndTooltip();
        }
        ImGui::EndGroup();
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::renderStatsRecorder()
    {
        float width = ImGui::GetContentRegionAvail().x;

        ImGui::BeginGroup();

        if (ImGui::Button("Select output file", ImVec2(width, 20)))
        {
            std::string res = Utils::wstringToString(Utils::saveFileDialog(TEXT(".txt")));
            if (!res.empty())
            {
                if (m_isRecording)
                {
                    onWarningRequested("You can't change the output file while recording is active. Please stop recording first.");
                }
                else
                {
                    m_outputFile = res;
                    m_eventsManager.emit<Events::StatsOutputFileUpdate>(Events::StatsOutputFileUpdate{ m_outputFile });
                }
            }
            else
            {
                onWarningRequested("You haven't selected a valid output file");
            }
        }

        std::string outputFilePath = m_outputFile.empty() ? "NOT SELECTED" : Utils::shortenPath(m_outputFile, 30);

        ImGui::Text("Output file: %s", outputFilePath.c_str());
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", m_outputFile.c_str());
            ImGui::EndTooltip();
        }

        ImGui::EndGroup();

        ImGui::Separator();

        ImGui::BeginGroup();

        if (ImGui::Button(m_isRecording ? "Stop recording" : "Start recording", ImVec2(width, 20)))
        {
            if (m_outputFile.empty())
            {
                onWarningRequested("You haven't selected an output file. Please select one to start recording.");
            }
            else
            {
                m_isRecording = !m_isRecording;
                m_eventsManager.emit<Events::StatsRecordingUpdate>(Events::StatsRecordingUpdate{ m_isRecording, m_rendererName });
            }
        }

        ImGui::Text("Recording time: %.2f s", m_recordingTime);
        ImGui::EndGroup();
    }

    ////////////////////////////////////////////////////////////////////////

    void UIController::drawStat(const std::string& label, const std::string& unit, float value, size_t precision, size_t totalWidth)
    {
        std::string formatStr = std::format("{{:>{}.{}f}}", totalWidth - (int)label.length() - (int)unit.length(), precision);
        std::string numberStr = std::vformat(formatStr, std::make_format_args(value));
        std::string fullStr = std::format("{}{}{}", label, numberStr, unit);
        ImGui::Text("%s", fullStr.c_str());
        
    }

    ////////////////////////////////////////////////////////////////////////
}