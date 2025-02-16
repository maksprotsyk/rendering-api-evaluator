#pragma once

#include <string>

namespace Engine::Visual
{

    class IModelInstance
    {
    public:
        virtual const std::string& GetId() const = 0;
        virtual ~IModelInstance() = default;
    };

    class ModelInstanceBase : public IModelInstance
    {
    public:
        ModelInstanceBase(const std::string& id);
        const std::string& GetId() const override;

    private:
        std::string m_id;
    };

}