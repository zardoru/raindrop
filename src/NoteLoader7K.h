#ifndef NLSM_H_
#define NLSM_H_

#include "Song.h"

namespace NoteLoaderSM
{
	VSRG::Song* LoadObjectsFromFile(String filename, String prefix);
}

namespace NoteLoaderFTB
{
	void LoadMetadata(String filename, String prefix, VSRG::Song *Out);
	void LoadObjectsFromFile(String filename, String prefix, VSRG::Song *Out);
}

namespace NoteLoaderBMS
{
	void LoadObjectsFromFile(String filename, String prefix, VSRG::Song *Out);
}

namespace NoteLoaderOM
{
	void LoadObjectsFromFile(String filename, String prefix, VSRG::Song *Out);
}

#endif