#ifndef NLSM_H_
#define NLSM_H_

#include "Song.h"

namespace NoteLoaderSM
{
	Song7K* LoadObjectsFromFile(String filename, String prefix = "");
}

#endif