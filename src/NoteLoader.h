/* description: .dcf (dotcur format) reader*/

#ifndef NOTELOADER_H_
#define NOTELOADER_H_

#include <vector>
#include "GameObject.h"
#include "Song.h"

// todo: return song
class NoteLoader 
{
public:
	// user responsability to clean this one up.
	static Song LoadObjectsFromFile(std::string filename, std::string prefix = "");
};

#endif // NOTELOADER_H_
