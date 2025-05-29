#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <algorithm>

namespace Engine::Utils
{

	std::wstring stringToWString(const std::string& str);

	std::string wstringToString(const std::wstring& wstr);

	std::vector<char> loadBytesFromFile(const std::string& filename);

	std::vector<std::string> splitString(const std::string& str, char delimiter);

	std::string readFile(const std::string& filename);

	template<typename T>
	std::string getTypeName();

	template <typename T>
	std::vector<std::string> getTypeNames();

	template <typename First, typename Second, typename ...Rest>
	std::vector<std::string> getTypeNames();

	template <typename T, T... S, typename F>
	constexpr void forSequence(std::integer_sequence<T, S...>, F&& f);

	template <typename Key, typename Value>
	std::vector<Key> getKeys(std::unordered_map<Key, Value> items);

	std::string shortenPath(const std::string& path, size_t maxLength);
	std::wstring openFileDialog(const std::wstring& formats);
	std::wstring saveFileDialog(const std::wstring& formats);

}

#include "BasicUtils.inl"

