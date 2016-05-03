#pragma once

class SongLoader;

struct ListEntry
{
    enum
    {
        Directory,
        Song
    } Kind;
    std::shared_ptr<void> Data;
    std::string EntryName;
};

enum ESortCriteria
{
	SORT_UNKNOWN,
	SORT_TITLE,
	SORT_AUTHOR,
	SORT_LENGTH,
	SORT_MAXNPS,
	SORT_MINNPS,
	SORT_COUNT
};

class SongList
{
    SongList* mParent;
    std::vector<ListEntry> mChildren;

	void SortByFn(std::function<bool(const ListEntry&, const ListEntry&)> fn);

public:
    SongList(SongList *Parent = nullptr);
    ~SongList();

    void AddNamedDirectory(std::mutex &loadMutex, SongLoader *Loader, std::filesystem::path Dir, std::string Name, bool VSRGActive, bool DotcurActive);
    void AddDirectory(std::mutex &loadMutex, SongLoader *Loader, std::filesystem::path Dir, bool VSRGActive, bool DotcurActive);
    void AddVirtualDirectory(std::string NewEntryName, Game::Song* List, int Count);
    void AddSong(std::shared_ptr<Game::Song> Song);

    // if false, it's a song
    bool IsDirectory(unsigned int Entry);
    std::shared_ptr<SongList> GetListEntry(unsigned int Entry);
    std::shared_ptr<Game::Song> GetSongEntry(unsigned int Entry);

    std::string GetEntryTitle(unsigned int Entry);
    unsigned int GetNumEntries();

    void SortAlphabetically();
    bool HasParentDirectory();
    SongList* GetParentDirectory();
	void SortBy(ESortCriteria criteria);
};