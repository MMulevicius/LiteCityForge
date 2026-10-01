#include "Core/AssetPaths.h"

#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace
{
    std::filesystem::path GetExecutablePath()
    {
#ifdef _WIN32
        std::wstring buffer(32768, L'\0');
        const DWORD length = GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));

        if (length == 0 || length >= buffer.size())
            throw std::runtime_error("Could not determine the executable path.");

        buffer.resize(length);
        return std::filesystem::path(buffer);

#elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);

        std::vector<char> buffer(size);
        if (_NSGetExecutablePath(buffer.data(), &size) != 0)
            throw std::runtime_error("Could not determine the executable path.");

        return std::filesystem::weakly_canonical(buffer.data());

#else
        std::vector<char> buffer(PATH_MAX);
        const ssize_t length =
            readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);

        if (length <= 0)
            throw std::runtime_error("Could not determine the executable path.");

        buffer.resize(static_cast<std::size_t>(length));
        return std::filesystem::path(buffer.data());
#endif
    }
}

namespace assets
{
    std::filesystem::path Root()
    {
        const auto root = GetExecutablePath().parent_path() / "assets";

        if (!std::filesystem::is_directory(root))
            throw std::runtime_error(
                "Runtime assets folder was not found: " + root.string());

        return root;
    }

    std::filesystem::path Path(const std::filesystem::path &relativePath)
    {
        const auto path = Root() / relativePath;

        if (!std::filesystem::exists(path))
            throw std::runtime_error("Asset was not found: " + path.string());

        return path;
    }
}