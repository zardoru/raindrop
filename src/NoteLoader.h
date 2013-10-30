/* description: .dcf (dotcur format) reader*/

#ifndef NOTELOADER_H_
#define NOTELOADER_H_

#include <vector>
#include "GameObject.h"
#include "Song.h"

namespace NoteLoader 
{
	void LoadNotes(Song* Out, SongInternal::Difficulty* Difficulty, String line);
	void LoadBPMs(Song* Out, SongInternal::Difficulty* Difficulty, String line);
	// user responsability to clean this one up.
	Song *LoadObjectsFromFile(String filename, String prefix = "");
};

#endif // NOTELOADER_H_
