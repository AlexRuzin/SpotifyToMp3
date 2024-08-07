#pragma once

#include <iostream>
#include <filesystem>
#include <string>

namespace shared 
{

// Remove directory
bool RemoveDirectoryRecursive(const std::string &dir);

// Create directory
bool CreateDirectory(const std::string &dir);

}