#include "Parser.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace Engine::Utils
{

    nlohmann::json Parser::readJson(const std::string& path)
    {
        std::ifstream file(path);

        nlohmann::json json;
        file >> json;

        return json;
    }

    template<>
    void Parser::fillFromJson(int& obj, const nlohmann::json& data)
    {
        obj = data.get<int>();
    }
    
    template<>
    void Parser::fillFromJson(float& obj, const nlohmann::json& data)
    {
        obj = data.get<float>();
    }

    template<>
    void Parser::fillFromJson(double& obj, const nlohmann::json& data)
    {
        obj = data.get<double>();
    }

    template<>
    void Parser::fillFromJson(std::string& obj, const nlohmann::json& data)
    {
        obj = data.get<std::string>();
    }

    template<>
    void Parser::fillFromJson(bool& obj, const nlohmann::json& data)
    {
        obj = data.get<bool>();
    }

    template<>
    void Parser::fillFromJson(nlohmann::json& obj, const nlohmann::json& data)
    {
		obj = data;
    }

}