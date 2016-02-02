#pragma once

#include "Song.h"

namespace NoteLoaderSM
{
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
}

namespace NoteLoaderSSC
{
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
}

namespace NoteLoaderFTB
{
	void LoadMetadata(std::string filename, std::string prefix, VSRG::Song *Out);
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
}

namespace NoteLoaderBMS
{
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
	void LoadObjectsFromFile(const std::filesystem::path&, VSRG::Song *Out);
}

namespace NoteLoaderOM
{
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
}

const char *LoadOJNCover(std::string filename, size_t &read);
namespace NoteLoaderOJN
{
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
}

namespace NoteLoaderBMSON
{
	void LoadObjectsFromFile(std::string filename, std::string prefix, VSRG::Song *Out);
}