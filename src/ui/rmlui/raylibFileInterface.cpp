#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include "raylibFileInterface.h"

inline bool StartsWith(const std::string &text, const std::string &prefix)
{
    return prefix.size() <= text.size() && std::equal(prefix.begin(), prefix.end(), text.begin());
}

inline void RemoveText(std::string &text, const std::string &toRemove)
{
    auto pos = std::string::npos;
    while ((pos = text.find(toRemove)) != std::string::npos)
        text.erase(pos, toRemove.length());
}

Rml::FileHandle RaylibFileInterface::Open(const Rml::String &path)
{
    auto stream = new std::fstream(ParsePath(path), std::ios::in | std::ios::binary);
    return reinterpret_cast<Rml::FileHandle>(stream);
}

void RaylibFileInterface::Close(Rml::FileHandle file)
{
    auto *fs = reinterpret_cast<std::fstream *>(static_cast<uintptr_t>(file));
    fs->close();
    delete fs;
}

size_t RaylibFileInterface::Read(void *buffer, std::size_t size, Rml::FileHandle file)
{
    auto *fs = reinterpret_cast<std::fstream *>(static_cast<uintptr_t>(file));
    fs->read(static_cast<char *>(buffer), static_cast<std::streamsize>(size));
    return static_cast<size_t>(fs->gcount());
}

bool RaylibFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
    auto *fs = reinterpret_cast<std::fstream *>(static_cast<uintptr_t>(file));
    std::ios::seekdir dir = std::ios::beg;
    if (origin == SEEK_CUR)
        dir = std::ios::cur;
    else if (origin == SEEK_END)
        dir = std::ios::end;
    fs->seekg(offset, dir);
    return fs->fail();
}

size_t RaylibFileInterface::Tell(Rml::FileHandle file)
{
    auto *fs = reinterpret_cast<std::fstream *>(static_cast<uintptr_t>(file));
    return static_cast<size_t>(fs->tellg());
}

std::string RaylibFileInterface::ParsePath(const std::string &path)
{
    std::string filePath = path;
    RemoveText(filePath, "..");
    while (StartsWith(filePath, "/"))
        filePath = filePath.substr(1);
    while (StartsWith(filePath, "assets/"))
        filePath = filePath.substr(7);

    const auto base = std::filesystem::current_path() / "assets";
    const auto full = base / std::filesystem::path(filePath);
    return full.string();
}
