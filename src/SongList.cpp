#include "pch.h"

#include "GameGlobal.h"
#include "Song.h"
#include "SongList.h"

#include "Song7K.h"
#include "SongDC.h"
#include "SongLoader.h"

SongList::SongList(SongList* Parent)
    : mParent(Parent)
{
}

SongList::~SongList()
{
}

void SongList::AddSong(std::shared_ptr<Game::Song> Song)
{
    ListEntry NewEntry;
    NewEntry.Kind = ListEntry::Song;
    NewEntry.Data = Song;

    mChildren.push_back(NewEntry);
}

void SongList::AddNamedDirectory(std::mutex &loadMutex, SongLoader *Loader, std::filesystem::path Dir, std::string Name, bool VSRGActive, bool DotcurActive)
{
    bool EntryWasPushed = false;
    SongList* NewList = new SongList(this);

    ListEntry NewEntry;

    NewEntry.EntryName = Name;
    NewEntry.Kind = ListEntry::Directory;
    NewEntry.Data = std::shared_ptr<void>(NewList);

    std::vector<VSRG::Song*> Songs7K;
    std::vector<dotcur::Song*> SongsDC;
    std::vector<std::string> Listing;

	for (auto i : std::filesystem::directory_iterator (Dir))
    {
        if (i == "." || i == "..") continue;

		if (!std::filesystem::is_directory(i.path())) continue;

        if (VSRGActive)
            Loader->LoadSong7KFromDir(i, Songs7K);

        if (DotcurActive)
            Loader->LoadSongDCFromDir(i.path().string(), SongsDC);

        if (!SongsDC.size() && !Songs7K.size()) // No songs, so, time to recursively search.
        {
            if (!EntryWasPushed)
            {
                std::unique_lock<std::mutex> lock(loadMutex);
                mChildren.push_back(NewEntry);
                EntryWasPushed = true;
            }

            NewList->AddDirectory(loadMutex, Loader, i, VSRGActive, DotcurActive);

            {
                std::unique_lock<std::mutex> lock(loadMutex);
                if (!NewList->GetNumEntries())
                {
                    if (mChildren.size())
                        mChildren.erase(mChildren.end() - 1);
                    EntryWasPushed = false;
                }
            }
        }
        else
        {
            size_t tSize = Songs7K.size() + SongsDC.size();

            if (Songs7K.size())
            {
                std::unique_lock<std::mutex> lock(loadMutex);

                for (auto j = Songs7K.begin();
                j != Songs7K.end();
                    ++j)
                {
                    NewList->AddSong(std::shared_ptr<Game::Song>(*j));
                }

                Songs7K.clear();
            }

            if (SongsDC.size())
            {
                std::unique_lock<std::mutex> lock(loadMutex);

                for (auto j = SongsDC.begin();
                j != SongsDC.end();
                    j++)
                {
                    NewList->AddSong(std::shared_ptr<Game::Song>(*j));
                }

                SongsDC.clear();
            }

            if (tSize > 0) // There's a song in here.
            {
                if (!EntryWasPushed)
                {
                    std::unique_lock<std::mutex> lock(loadMutex);
                    mChildren.push_back(NewEntry);
                    EntryWasPushed = true;
                }
            }
        }
    }
}

void SongList::AddDirectory(std::mutex &loadMutex, SongLoader *Loader, std::filesystem::path Dir, bool VSRGActive, bool DotcurActive)
{
    AddNamedDirectory(loadMutex, Loader, Dir, Utility::Narrow(Dir.filename()), VSRGActive, DotcurActive);
}

void SongList::AddVirtualDirectory(std::string NewEntryName, Game::Song* List, int Count)
{
    SongList* NewList = new SongList(this);

    ListEntry NewEntry;
    NewEntry.EntryName = NewEntryName;
    NewEntry.Kind = ListEntry::Directory;
    NewEntry.Data = std::shared_ptr <void>(NewList);

    for (int i = 0; i < Count; i++)
        NewList->AddSong(std::shared_ptr<Game::Song>(&List[Count]));

    mChildren.push_back(NewEntry);
}

// if false, it's a song
bool SongList::IsDirectory(unsigned int Entry)
{
    if (Entry >= mChildren.size()) return true;
    return mChildren[Entry].Kind == ListEntry::Directory;
}

std::shared_ptr<SongList> SongList::GetListEntry(unsigned int Entry)
{
    assert(IsDirectory(Entry));
    return std::static_pointer_cast<SongList> (mChildren[Entry].Data);
}

std::shared_ptr<Game::Song> SongList::GetSongEntry(unsigned int Entry)
{
    if (!IsDirectory(Entry))
        return std::static_pointer_cast<Game::Song> (mChildren[Entry].Data);
    else
        return nullptr;
}

std::string SongList::GetEntryTitle(unsigned int Entry)
{
    if (Entry >= mChildren.size())
        return "";

    if (mChildren[Entry].Kind == ListEntry::Directory)
        return mChildren[Entry].EntryName;
    else
    {
        std::shared_ptr<Game::Song> Song = std::static_pointer_cast<Game::Song>(mChildren[Entry].Data);
        return Song->SongName;
    }
}

unsigned int SongList::GetNumEntries()
{
    return mChildren.size();
}

bool SongList::HasParentDirectory()
{
    return mParent != NULL;
}

SongList* SongList::GetParentDirectory()
{
    return mParent;
}