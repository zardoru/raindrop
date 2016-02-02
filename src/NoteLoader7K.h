#pragma once

#include "Song.h"

namespace NoteLoaderSM
{
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
}

namespace NoteLoaderSSC
{
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
}

namespace NoteLoaderFTB
{
	void LoadMetadata(GString filename, GString prefix, VSRG::Song *Out);
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
}

namespace NoteLoaderBMS
{
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
	void LoadObjectsFromFile(const std::filesystem::path&, VSRG::Song *Out);
}

namespace NoteLoaderOM
{
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
}

const char *LoadOJNCover(GString filename, size_t &read);
namespace NoteLoaderOJN
{
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
}

namespace NoteLoaderBMSON
{
	void LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out);
}