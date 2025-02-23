#pragma once

#include <string>
#include <vector>
#include <typeinfo>

namespace Engine::Utils
{

	std::wstring stringToWString(const std::string& str);

	std::string wstringToString(const std::wstring& wstr);

	std::vector<char> loadBytesFromFile(const std::string& filename);

	std::vector<std::string> splitString(const std::string& str, char delimiter);

	std::string readFile(const std::string& filename);

	template<typename T>
	std::string getTypeName()
	{
		std::string name = typeid(T).name();
		return name.substr(6);
	}

	template <typename T>
	std::vector<std::string> getTypeNames()
	{
		return { getTypeName<T>() };
	}

	template <typename First, typename Second, typename ...Rest>
	std::vector<std::string> getTypeNames()
	{
		std::vector<std::string> names = getTypeNames<Second, Rest...>();
		names.push_back(getTypeName<First>());
		return names;
	}

	template <typename T, T... S, typename F>
	constexpr void forSequence(std::integer_sequence<T, S...>, F&& f)
	{
		(static_cast<void>(f(std::integral_constant<T, S>{})), ...);
	}

	template <typename Key, typename Value>
	std::vector<Key> getKeys(std::unordered_map<Key, Value> items)
	{
		std::vector<Key> keys(items.size());
		std::transform(items.begin(), items.end(), keys.begin(), [](const auto& p) { return p.first; });
		return keys;
	}


	
}

