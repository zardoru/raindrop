/* description: .dcf (dotcur format) reader*/

#ifndef NOTELOADER_H_
#define NOTELOADER_H_

#include <vector>
#include "GameObject.h"
#include "Song.h"

class NoteLoader 
{
public:
	// user responsability to clean this one up.
	static Song *LoadObjectsFromFile(String filename, String prefix = "");
};

#endif // NOTELOADER_H_
