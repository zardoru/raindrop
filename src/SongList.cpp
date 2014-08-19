#include "GameGlobal.h"
#include "Song.h"
#include "SongList.h"

#include "Song7K.h"
#include "SongDC.h"
#include "SongLoader.h"

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

void SongList::AddDirectory(SongLoader *Loader, Directory Dir, bool VSRGActive, bool DotcurActive)
{
	SongList* NewList = new SongList(this);

	ListEntry NewEntry;

	NewEntry.EntryName = Dir.path().substr(Dir.path().find_last_of('/')+1);
	NewEntry.Kind = ListEntry::Directory;
	NewEntry.Data = NewList;

	std::vector<VSRG::Song*> Songs7K;
	std::vector<dotcur::Song*> SongsDC;
	std::vector<String> Listing;

	Dir.ListDirectory(Listing, Directory::FS_DIR);
	
	for (std::vector<String>::iterator i = Listing.begin();
		i != Listing.end();
		i++)
	{
		if (VSRGActive)
			Loader->LoadSong7KFromDir(Dir / *i, Songs7K);
		
		if (DotcurActive)
			Loader->LoadSongDCFromDir(Dir / *i, SongsDC);

		if (!SongsDC.size() && !Songs7K.size())
			NewList->AddDirectory(Loader, Dir / *i, VSRGActive, DotcurActive);

		if (Songs7K.size())
		{
			for (std::vector<VSRG::Song*>::iterator j = Songs7K.begin();
				j != Songs7K.end();
				j++)
			{
				NewList->AddSong(*j);
			}

			Songs7K.clear();
		}

		if (SongsDC.size())
		{
			for (std::vector<dotcur::Song*>::iterator j = SongsDC.begin();
				j != SongsDC.end();
				j++)
			{
				NewList->AddSong(*j);
			}

			SongsDC.clear();
		}
	}

	if (NewList->GetNumEntries())
	{
		if (NewList->GetNumEntries() > 1)
			mChildren.push_back(NewEntry);
		else
		{
			// Move the entries back here, as they are too small to warrant having a list of a single item.
			for (std::vector<ListEntry>::iterator i = NewList->mChildren.begin(); i != NewList->mChildren.end(); i++)
			{
				mChildren.push_back(*i);

				if (i->Kind == ListEntry::Directory)
				{
					SongList* L = static_cast<SongList*> (i->Data);
					L->mParent = this;
				}

				i = NewList->mChildren.erase(i);

				if (i == NewList->mChildren.end())
					break;
			}

			delete NewList;
		}
	}
	else
		delete NewList;
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