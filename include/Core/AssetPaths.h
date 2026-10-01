#pragma once

#include <filesystem>

namespace assets
{
    std::filesystem::path Root();
    std::filesystem::path Path(const std::filesystem::path &relativePath);
}