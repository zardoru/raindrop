#pragma once

class SongLoader;

struct ListEntry
{
	enum {
		Directory,
		Song
	} Kind;
    std::shared_ptr<void> Data;
	GString EntryName; 
};

class SongList
{
	SongList* mParent;
	std::vector<ListEntry> mChildren;

public:
	SongList (SongList *Parent = nullptr);
	~SongList();

	void AddNamedDirectory(std::mutex &loadMutex, SongLoader *Loader, Directory Dir, GString Name, bool VSRGActive, bool DotcurActive);
	void AddDirectory(std::mutex &loadMutex, SongLoader *Loader, Directory Dir, bool VSRGActive, bool DotcurActive);
	void AddVirtualDirectory(GString NewEntryName, Game::Song* List, int Count);
	void AddSong(std::shared_ptr<Game::Song> Song);

	// if false, it's a song
	bool IsDirectory(unsigned int Entry);
	std::shared_ptr<SongList> GetListEntry(unsigned int Entry);
	std::shared_ptr<Game::Song> GetSongEntry(unsigned int Entry);

	GString GetEntryTitle(unsigned int Entry);
	unsigned int GetNumEntries();

	void SortAlphabetically();
	bool HasParentDirectory();
	SongList* GetParentDirectory();
};