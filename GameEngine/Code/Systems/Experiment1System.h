#pragma once

#include "ExperimentSystemBase.h"
#include "Managers/EntitiesManager.h"

namespace Engine::Systems
{
	class Experiment1System: public ExperimentSystemBase
	{
	public:
		void onStart() override;
		void onUpdate(float dt) override;
		void onStop() override;
		int getPriority() const override;

	private:
		void rotateObjects(float dt);
		void moveCamera(float dt);

	private:
		float m_rotationSpeed = 2.0f;
		float m_cameraSpeed = 1.0f;
		float m_cameraMaxDistance = 10.0f;
		std::vector<float> m_radiuses = { 5 };
		std::vector<EntityID> m_clockwiseObjects;
		std::vector<EntityID> m_counterClockwiseObjects;
		EntityID m_cameraId;
		bool m_cameraMoveForwards = true;
		float m_originalCameraPosition = 0.0f;
	};
}