#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include "Utils/BasicUtils.h"

namespace Engine::Utils
{
    template<typename Class, typename T>
    struct PropertyImpl
    {
        constexpr PropertyImpl(T Class::* _member, const char* _name): member{_member}, name{_name} {}

        using Type = T;

        T Class::* member;
        const char* name;
    };

    template<typename Class, typename T>
    constexpr auto property(T Class::* member, const char* name)
    {
        return PropertyImpl<Class, T>{member, name};
    }

    class Parser
    {
    public:

        static nlohmann::json readJson(const std::string& path);

        template<typename T>
        static void fillFromJson(T& obj, const nlohmann::json& data);

        template<typename ItemType>
        static void fillFromJson(std::vector<ItemType>& obj, const nlohmann::json& data);

        template<>
        static void fillFromJson(int& obj, const nlohmann::json& data);

        template<>
        static void fillFromJson(float& obj, const nlohmann::json& data);

        template<>
        static void fillFromJson(double& obj, const nlohmann::json& data);

        template<>
        static void fillFromJson(std::string& obj, const nlohmann::json& data);

        template<>
        static void fillFromJson(bool& obj, const nlohmann::json& data);

        template<>
        static void fillFromJson(nlohmann::json& obj, const nlohmann::json& data);
    };



}

namespace Engine::Utils
{
    template<typename T>
    void Parser::fillFromJson(T& obj, const nlohmann::json& data)
    {
        constexpr auto nbProperties = std::tuple_size<decltype(T::properties)>::value;

        forSequence(std::make_index_sequence<nbProperties>{}, [&](auto i) 
            {
                constexpr auto prop = std::get<i>(T::properties);
                if (!data.contains(prop.name))
                {
                    return;
                }
                using Type = typename decltype(prop)::Type;

                auto& propertyRef = obj.*(prop.member);
                fillFromJson(propertyRef, data[prop.name]);
            });
            
    }

    template<typename ItemType>
    void Parser::fillFromJson(std::vector<ItemType>& obj, const nlohmann::json& data)
    {
        for (const nlohmann::json& elem : data)
        {
            obj.emplace_back();
            fillFromJson(obj.back(), elem);
        }
    }
}


#define PROPERTY(CLASS, MEMBER) Engine::Utils::property(&CLASS::MEMBER, #MEMBER)

#define NAMED_PROPERTY(CLASS, MEMBER, NAME) Engine::Utils::property(&CLASS::MEMBER, NAME)

#define SERIALIZABLE(...) constexpr static auto properties = std::make_tuple(__VA_ARGS__);
