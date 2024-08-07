#include <iostream>
#include <filesystem>
#include <string>

#include "shared.h"

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

bool shared::CreateDirectory(const std::string &dir)
{
    if (std::filesystem::exists(dir)) {
        return false;
    }

    return std::filesystem::create_directory(dir);
}

