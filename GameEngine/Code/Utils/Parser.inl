#pragma once

#include "Parser.h"

namespace Engine::Utils
{

    template<typename Class, typename T>
    constexpr auto property(T Class::* member, const char* name)
    {
        return PropertyImpl<Class, T>{member, name};
    }

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