#pragma once

#include <iostream>
#include <filesystem>
#include <string>


namespace shared 
{

// Remove directory
bool RemoveDirectoryRecursive(const std::string &dir);

// Create directory
bool CreateDirectory2(const std::string &dir);

// wide 2 string 
std::string ws2s(const std::wstring& wstr);

}