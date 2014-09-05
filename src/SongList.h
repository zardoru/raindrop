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
	String EntryName; 
};

class SongList
{
	SongList *mParent;
	std::vector<ListEntry> mChildren;

public:
	SongList (SongList* Parent = NULL);
	~SongList();

	void AddDirectory(boost::mutex &loadMutex, SongLoader *Loader, Directory Dir, bool VSRGActive, bool DotcurActive);
	void AddVirtualDirectory(String NewEntryName, Game::Song* List, int Count);
	void AddSong(Game::Song* Song);

	// if false, it's a song
	bool IsDirectory(unsigned int Entry);
	SongList* GetListEntry(unsigned int Entry);
	Game::Song* GetSongEntry(unsigned int Entry);

	String GetEntryTitle(unsigned int Entry);
	unsigned int GetNumEntries();

	void SortAlphabetically();
	bool HasParentDirectory();
	SongList* GetParentDirectory();
};

#endif