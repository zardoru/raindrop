/* description: .dcf (raindrop format) reader*/

#ifndef NOTELOADER_H_
#define NOTELOADER_H_

namespace NoteLoader 
{
	// user responsability to clean this one up.
	dotcur::Song *LoadObjectsFromFile(String filename, String prefix = "");
};

#endif // NOTELOADER_H_
