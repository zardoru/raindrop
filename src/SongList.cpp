#include "GameGlobal.h"
#include "Song.h"
#include "SongList.h"
#include "FileManager.h"

#include "Song7K.h"
#include "SongDC.h"

SongList::SongList (SongList* Parent)
	: mParent(Parent)
{
}

SongList::~SongList()
{
	for (std::vector<ListEntry>::iterator i = mChildren.begin();
		i != mChildren.end();
		i++)
	{
		if (i->Kind == ListEntry::Directory)
		{
			SongList* toDelete = static_cast<SongList*>(i->Data);
			delete toDelete;
		}else
		{
			Game::Song* toDelete = static_cast<Game::Song*>(i->Data);

			// Virtual destructor, so we can safely delete.
			delete toDelete;
		}
	}
}

void SongList::AddSong(Game::Song* Song)
{
	ListEntry NewEntry;
	NewEntry.EntryName = Song->SongName;
	NewEntry.Kind = ListEntry::Song;
	NewEntry.Data = Song;

	mChildren.push_back(NewEntry);
}

void SongList::AddDirectory(Directory Dir, bool VSRGActive, bool DotcurActive)
{
	SongList* NewList = new SongList(this);

	ListEntry NewEntry;
	NewEntry.EntryName = Dir.path();
	NewEntry.Kind = ListEntry::Directory;
	NewEntry.Data = NewList;

	if (VSRGActive)
	{
		std::vector<VSRG::Song*> Songs;
		FileManager::GetSongList7K(Songs, Dir);		
		for (std::vector<VSRG::Song*>::iterator i = Songs.begin();
			i != Songs.end();
			i++)
		{
			NewList->AddSong(*i);
		}
	}

	if (DotcurActive)
	{
		std::vector<dotcur::Song*> Songs;
		FileManager::GetSongListDC(Songs, Dir);
		for (std::vector<dotcur::Song*>::iterator i = Songs.begin();
			i != Songs.end();
			i++)
		{
			NewList->AddSong(*i);
		}		
	}

	mChildren.push_back(NewEntry);
}

void SongList::AddVirtualDirectory(String NewEntryName, Game::Song* List, int Count)
{
	SongList* NewList = new SongList(this);

	ListEntry NewEntry;
	NewEntry.EntryName = NewEntryName;
	NewEntry.Kind = ListEntry::Directory;
	NewEntry.Data = NewList;

	for (int i = 0; i < Count; i++)
		NewList->AddSong(&List[Count]);

	mChildren.push_back(NewEntry);
}

// if false, it's a song
bool SongList::IsDirectory(unsigned int Entry)
{
	return mChildren[Entry].Kind == ListEntry::Directory;
}

SongList* SongList::GetListEntry(unsigned int Entry)
{
	assert(IsDirectory(Entry));
	return static_cast<SongList*> (mChildren[Entry].Data);
}

Game::Song* SongList::GetSongEntry(unsigned int Entry)
{
	assert(!IsDirectory(Entry));
	return static_cast<Game::Song*> (mChildren[Entry].Data);
}

String SongList::GetEntryTitle(unsigned int Entry)
{
	return mChildren[Entry].EntryName;
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