#ifndef SONGLIST_H_
#define SONGLIST_H_

#include <boost/thread.hpp>

class SongLoader;

struct ListEntry
{
	enum {
		Directory,
		Song
	} Kind;
	void* Data;
	GString EntryName; 
};

class SongList
{
	SongList *mParent;
	std::vector<ListEntry> mChildren;

public:
	SongList (SongList* Parent = NULL);
	~SongList();

	void AddNamedDirectory(boost::mutex &loadMutex, SongLoader *Loader, Directory Dir, GString Name, bool VSRGActive, bool DotcurActive);
	void AddDirectory(boost::mutex &loadMutex, SongLoader *Loader, Directory Dir, bool VSRGActive, bool DotcurActive);
	void AddVirtualDirectory(GString NewEntryName, Game::Song* List, int Count);
	void AddSong(Game::Song* Song);

	// if false, it's a song
	bool IsDirectory(unsigned int Entry);
	SongList* GetListEntry(unsigned int Entry);
	Game::Song* GetSongEntry(unsigned int Entry);

	GString GetEntryTitle(unsigned int Entry);
	unsigned int GetNumEntries();

	void SortAlphabetically();
	bool HasParentDirectory();
	SongList* GetParentDirectory();
};

#endif