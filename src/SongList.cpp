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

void SongList::AddDirectory(boost::mutex &loadMutex, SongLoader *Loader, Directory Dir, bool VSRGActive, bool DotcurActive)
{
	bool EntryWasPushed = false;
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
		if (*i == "." || *i == "..") continue;
		
		if (VSRGActive)
			Loader->LoadSong7KFromDir(Dir / *i, Songs7K);
		
		if (DotcurActive)
			Loader->LoadSongDCFromDir(Dir / *i, SongsDC);

		if (!SongsDC.size() && !Songs7K.size()) // No songs, so, time to recursively search.
		{
			if (!EntryWasPushed)
			{
				loadMutex.lock();
				mChildren.push_back	(NewEntry);
				loadMutex.unlock();
				EntryWasPushed = true;
			}

			NewList->AddDirectory(loadMutex, Loader, Dir / *i, VSRGActive, DotcurActive);

			loadMutex.lock();
			if (!NewList->GetNumEntries())
			{
				if (mChildren.size())
					mChildren.erase(mChildren.end()-1);
			}
			loadMutex.unlock();
		}
		else
		{
			size_t tSize = Songs7K.size() + SongsDC.size();

			if (Songs7K.size())
			{
				loadMutex.lock();
				for (std::vector<VSRG::Song*>::iterator j = Songs7K.begin();
					j != Songs7K.end();
					j++)
				{
					NewList->AddSong(*j);
				}

				Songs7K.clear();
				loadMutex.unlock();
			}

			if (SongsDC.size())
			{
				loadMutex.lock();
				for (std::vector<dotcur::Song*>::iterator j = SongsDC.begin();
					j != SongsDC.end();
					j++)
				{
					NewList->AddSong(*j);
				}

				SongsDC.clear();
				loadMutex.unlock();
			}

			if (tSize > 0) // There's a song in here. 
			{
				if (!EntryWasPushed)
				{
					loadMutex.lock();
					mChildren.push_back	(NewEntry);
					loadMutex.unlock();
					EntryWasPushed = true;
				}
			}
		}
	}

	if (!EntryWasPushed)
	{
		delete NewList;
	}	
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
