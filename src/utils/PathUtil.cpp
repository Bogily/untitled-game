#include "PathUtil.h"

#include <filesystem>

#if defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace PathUtil
{
    void SetWorkingDirectoryToExecutable()
    {
#ifdef _WIN32
        char modulePath[MAX_PATH] = {};
        const DWORD len = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
        if (len == 0 || len >= MAX_PATH)
        {
            return;
        }

        std::error_code error;
        const std::filesystem::path exePath(modulePath);
        const std::filesystem::path exeDir = exePath.parent_path();
        if (!exeDir.empty())
        {
            std::filesystem::current_path(exeDir, error);
        }
#elif defined(__linux__)
        char modulePath[PATH_MAX] = {};
        const ssize_t len = readlink("/proc/self/exe", modulePath, sizeof(modulePath) - 1);
        if (len <= 0)
        {
            return;
        }

        modulePath[len] = '\0';

        std::error_code error;
        const std::filesystem::path exePath(modulePath);
        const std::filesystem::path exeDir = exePath.parent_path();
        if (!exeDir.empty())
        {
            std::filesystem::current_path(exeDir, error);
        }
#endif
    }
}
