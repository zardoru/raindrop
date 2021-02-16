#pragma once

#include "Song.h"

namespace NoteLoaderSM
{
    void LoadObjectsFromFile(const std::filesystem::path &filename, rd::Song *Out);
}

namespace NoteLoaderSSC
{
    void LoadObjectsFromFile(const std::filesystem::path &filename, rd::Song *Out);
}

namespace NoteLoaderFTB
{
    void LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song *Out);
}

namespace NoteLoaderBMS
{
    void LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song *Out);
}

namespace NoteLoaderOM
{
    void LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song *Out);
}

const char *LoadOJNCover(const std::filesystem::path& filename, size_t &read);
namespace NoteLoaderOJN
{
    void LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song *Out);
}

namespace NoteLoaderBMSON
{
    void LoadObjectsFromFile(const std::filesystem::path& filename, rd::Song *Out);
}