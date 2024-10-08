#pragma once

#include <string>

#include "Window.h"
#include "Utils/Vector.h"

namespace Engine::Visual
{

    class IRenderer
    {
    public:
        struct AbstractModel
        {
            virtual size_t GetVertexCount() const = 0;
            virtual ~AbstractModel() = default;
        };

        virtual void init(const Window& window) = 0;
        virtual void clearBackground() = 0;
        virtual void draw(const AbstractModel& model) = 0;
        virtual void render() = 0;

        virtual std::unique_ptr<AbstractModel> createModel() = 0;
        virtual void createBuffersFromModel(AbstractModel& model) = 0;
        virtual void loadModel(AbstractModel& model, const std::string& filename) = 0;
        virtual void transformModel(
            AbstractModel& model,
            const Utils::Vector3& position,
            const Utils::Vector3& rotation,
            const Utils::Vector3& scale
        ) = 0;

        virtual void setCameraProperties(const Utils::Vector3& position, const Utils::Vector3& rotation) = 0;

        virtual ~IRenderer() = default;
    };

}