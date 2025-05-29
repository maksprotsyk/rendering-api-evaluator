#include "BasicUtils.h"

#include <Windows.h>
#include <fstream>
#include <sstream>

namespace Engine::Utils
{
    //////////////////////////////////////////////////////////////////////////

    std::wstring stringToWString(const std::string& str) 
    {
        if (str.empty()) return std::wstring();

        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);

        return wstr;
    }

    //////////////////////////////////////////////////////////////////////////

    std::string wstringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return std::string();

        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
        return str;
    }

    //////////////////////////////////////////////////////////////////////////

    std::vector<char> loadBytesFromFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) 
        {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        // Read the file content
        return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    //////////////////////////////////////////////////////////////////////////

    std::vector<std::string> splitString(const std::string& str, char delimiter)
    {
        std::vector<std::string> result;
        std::string::size_type start = 0;
        std::string::size_type end = 0;

        // Loop through the string to find each occurrence of the delimiter
        while ((end = str.find(delimiter, start)) != std::string::npos) {
            // Extract the substring and add it to the vector
            result.push_back(str.substr(start, end - start));
            start = end + 1;  // Move past the delimiter
        }

        // Add the last part of the string after the last delimiter
        result.push_back(str.substr(start));

        return result;
    }

    //////////////////////////////////////////////////////////////////////////

    std::string readFile(const std::string& filePath)
    {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + filePath);
        }

        std::ostringstream contents;
        contents << file.rdbuf();
        file.close();

        return contents.str();
    }

    //////////////////////////////////////////////////////////////////////////

    std::string shortenPath(const std::string& path, size_t maxLength)
    {
        if (path.length() <= maxLength)
        {
            return path;
        }

        size_t partLength = maxLength / 2 - 2;
        std::string shortened = path.substr(0, partLength) + "..." + path.substr(path.length() - partLength);
        return shortened;
    }

    //////////////////////////////////////////////////////////////////////////

    std::wstring openFileDialog(const std::wstring& formats)
    {
        WCHAR originalDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, originalDir);

        OPENFILENAMEW ofn;
        wchar_t szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = formats.data();
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        std::wstring res = L"";
        if (GetOpenFileNameW(&ofn) == TRUE)
        {
            res = ofn.lpstrFile;
        }
        SetCurrentDirectoryW(originalDir);
        return res;
    }

    //////////////////////////////////////////////////////////////////////////

    std::wstring saveFileDialog(const std::wstring& formats)
    {
        WCHAR originalDir[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, originalDir);

        OPENFILENAMEW ofn;
        wchar_t szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = formats.data();
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"txt";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        std::wstring res = L"";
        if (GetSaveFileNameW(&ofn) == TRUE)
        {
            res = ofn.lpstrFile;
        }
        SetCurrentDirectoryW(originalDir);
        return res;
    }

    //////////////////////////////////////////////////////////////////////////

}