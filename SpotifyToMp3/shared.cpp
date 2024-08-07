#include <iostream>
#include <filesystem>
#include <string>
#include <codecvt>

#include "shared.h"

#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif //_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

using namespace shared;

bool shared::RemoveDirectoryRecursive(const std::string &dir)
{
    if (!std::filesystem::exists(dir)) {
        return false;
    }

    if (!std::filesystem::is_directory(dir)) {
        return false;
    }

    std::filesystem::remove_all(dir);

    return true;
}

bool shared::CreateDirectory2(const std::string &dir)
{
    if (std::filesystem::exists(dir)) {
        return false;
    }

    return std::filesystem::create_directory(dir);
}

std::string shared::ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}
