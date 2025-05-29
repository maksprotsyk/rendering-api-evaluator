#pragma once

#include <string>

#include "Visual/Window.h"
#include "Events/StatsEvents.h"
#include "Managers/EventsManager.h"

namespace Engine::Visual
{

	class UIController
	{
	public:
		UIController(std::vector<std::string> rendererNames);
		void init();
		void setRenderer(const std::string& rendererName);
		void render(float dt);
		void cleanUp();
	private:

		inline static const float k_warningTime = 5.0f;
		inline static const float k_maxRecordingTime = 60.0f;

	private:
		void drawStat(const std::string& label, const std::string& unit, float value, size_t precision, size_t totalWidth = 43);
		void onWarningRequested(const std::string& message);
		void renderPerformance();
		void renderSettings();
		void renderStatsRecorder();

	private:
		const Visual::Window& m_window;
		EventsManager& m_eventsManager;
		std::vector<std::string> m_rendererNames;
		std::string m_rendererName;
		std::string m_outputFile;
		bool m_isRecording;
		float m_recordingTime;
		StatsData m_statsData{};

		EventListenerID m_statsUpdateListenerId = -1;
		EventListenerID m_warningListenerId = -1;

		float m_warningTimer;
		std::string m_warningMessage;
	};

}