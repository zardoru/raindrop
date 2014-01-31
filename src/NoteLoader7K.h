#ifndef NLSM_H_
#define NLSM_H_

#include "Song.h"

namespace NoteLoaderSM
{
	Song7K* LoadObjectsFromFile(String filename, String prefix);
}

namespace NoteLoaderFTB
{
	void LoadMetadata(String filename, String prefix, Song7K *Out);
	void LoadObjectsFromFile(String filename, String prefix, Song7K *Out);
}

namespace NoteLoaderBMS
{
	void LoadObjectsFromFile(String filename, String prefix, Song7K *Out);
}

#endif