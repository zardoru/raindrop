/* description: .dcf (dotcur format) reader*/

#ifndef NOTELOADER_H_
#define NOTELOADER_H_

#include <vector>
#include "GameObject.h"
#include "Song.h"

namespace NoteLoader 
{
	// user responsability to clean this one up.
	SongDC *LoadObjectsFromFile(String filename, String prefix = "");
};

#endif // NOTELOADER_H_
