#pragma once

#include "Song.h"

namespace NoteLoaderSM
{
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}

namespace NoteLoaderSSC
{
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}

namespace NoteLoaderFTB
{
    void LoadMetadata(std::string filename, std::string prefix, VSRG::Song *Out);
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}

namespace NoteLoaderBMS
{
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}

namespace NoteLoaderOM
{
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}

const char *LoadOJNCover(std::filesystem::path filename, size_t &read);
namespace NoteLoaderOJN
{
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}

namespace NoteLoaderBMSON
{
    void LoadObjectsFromFile(std::filesystem::path filename, VSRG::Song *Out);
}