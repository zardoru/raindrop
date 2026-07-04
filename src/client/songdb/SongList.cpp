#include <memory>
#include <functional>
#include <mutex>
#include <rmath.h>

#include <ChartGroup.h>
#include <text_and_file_util.h>
#include <cassert>
#include "SongList.h"

#include "SongLoader.h"

ListEntry::ListEntry() {
	Kind = Directory;
	SelectedIndex = 0;
}

SongList::SongList(SongList* Parent)
    : mParent(Parent)
	, IsInUse(false)
{
}

SongList::~SongList()
{
}

void SongList::Clear()
{
	mChildren.clear();
}

void SongList::SetInUse(bool inuse)
{
	IsInUse = inuse;
}

bool SongList::InUse()
{
	return IsInUse;
}

void SongList::ClearEmpty()
{
	for (auto it = mChildren.begin(); it != mChildren.end(); ) {
		bool increase = true;

		if (it->Kind == it->Directory) {
			auto list = std::static_pointer_cast<SongList>(it->Data);
			if (list->GetNumEntries() == 0 && !list->InUse()) {
				it = mChildren.erase(it);
				increase = false;
			}
		}

		if (increase)
			++it;
	}
}

void SongList::AddSong(std::shared_ptr<otoworm::ChartGroup> chart_group)
{
    ListEntry NewEntry;
    NewEntry.Kind = ListEntry::Song;
    NewEntry.Data = chart_group;

    mChildren.push_back(NewEntry);
}

void SongList::AddEntry(ListEntry entry)
{
	mChildren.push_back(entry);
}

const std::vector<ListEntry>& SongList::GetEntries()
{
	return mChildren;
}

void SongList::AddNamedDirectory(
	std::mutex &loadMutex, 
	SongLoader *Loader, 
	std::filesystem::path Dir, 
	std::string Name,
	OnLoadNotifyFunc OnSongLoaded)
{
    bool EntryWasPushed = false;
    auto* NewList = new SongList(this);

    ListEntry NewEntry;

    NewEntry.EntryName = Name;
    NewEntry.Kind = ListEntry::Directory;
    NewEntry.Data = std::shared_ptr<void>(NewList);

    std::vector<std::shared_ptr<otoworm::ChartGroup>> chart_groups;
    std::vector<std::string> Listing;

	// boost throws with nonexisting directories
	if (!std::filesystem::exists(Dir)) return;

	for (const auto& i : std::filesystem::directory_iterator (Dir))
    {
        if (i.path() == "." || i.path() == "..") continue;

		if (!std::filesystem::is_directory(i.path())) continue;

		Loader->LoadChartGroupsFromDir(i, chart_groups);

        if (!chart_groups.size()) // No songs, so, time to recursively search.
        {
            if (!EntryWasPushed)
            {
                std::unique_lock<std::mutex> lock(loadMutex);
                mChildren.push_back(NewEntry);
                EntryWasPushed = true;
            }

            NewList->AddDirectory(loadMutex, Loader, i, OnSongLoaded);

            {
                std::unique_lock<std::mutex> lock(loadMutex);
                if (!NewList->GetNumEntries() && !NewList->InUse())
                {
                    if (mChildren.size())
                        mChildren.erase(mChildren.end() - 1);
                    EntryWasPushed = false;
                }
            }
        }
        else
        {
            {
                std::unique_lock<std::mutex> lock(loadMutex);

                for (auto j = chart_groups.begin();
                j != chart_groups.end();
                    ++j)
                {
                    NewList->AddSong(*j);
                }

                chart_groups.clear();
            }

            if (!EntryWasPushed)
            {
                std::unique_lock<std::mutex> lock(loadMutex);
                mChildren.push_back(NewEntry);
                EntryWasPushed = true;
            }
        }

        if (EntryWasPushed) {
            if (OnSongLoaded)
                OnSongLoaded ();
        }
    }
}

void SongList::AddDirectory(std::mutex &loadMutex, SongLoader *Loader, std::filesystem::path Dir, OnLoadNotifyFunc OnSongLoaded)
{
    AddNamedDirectory(loadMutex, Loader, Dir, otoworm::locale::wstring_to_utf8(Dir.filename().wstring()), OnSongLoaded);
}

// if false, it's a song
bool SongList::IsDirectory(unsigned int Entry) const
{
    if (Entry >= mChildren.size()) return true;
    return mChildren[Entry].Kind == ListEntry::Directory;
}

std::shared_ptr<SongList> SongList::GetListEntry(unsigned int Entry)
{
    assert(IsDirectory(Entry));
    return std::static_pointer_cast<SongList> (mChildren[Entry].Data);
}

std::shared_ptr<otoworm::ChartGroup> SongList::GetSongEntry(unsigned int Entry)
{
    if (!IsDirectory(Entry))
        return std::static_pointer_cast<otoworm::ChartGroup> (mChildren[Entry].Data);
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
        std::shared_ptr<otoworm::ChartGroup> song = std::static_pointer_cast<otoworm::ChartGroup>(mChildren[Entry].Data);
        if (song)
            return song->title;
        else
            return "<no song>";
    }
}

unsigned int SongList::GetNumEntries() const
{
    return mChildren.size();
}

bool SongList::HasParentDirectory()
{
    return mParent != nullptr;
}

SongList* SongList::GetParentDirectory()
{
    return mParent;
}

void SongList::SortByFn(std::function<bool(const ListEntry&, const ListEntry&)> fn)
{
	std::stable_sort(mChildren.begin(), mChildren.end(), [&](const ListEntry&A, const ListEntry&B)
	{
		if (A.Kind == ListEntry::Directory && B.Kind != A.Kind)
		{
			return true;
		}

		if (A.Kind != ListEntry::Directory && B.Kind != A.Kind)
		{
			return false;
		}

		if (A.Kind == B.Kind && A.Kind == ListEntry::Directory)
			return A.EntryName < B.EntryName;

		return fn(A, B);
	});
};

void SongList::SortBy(ESortCriteria criteria)
{
	switch (criteria)
	{
	case SORT_TITLE:
		SortByFn([](const ListEntry&A, const ListEntry&B)
		{
			auto a = std::static_pointer_cast<otoworm::ChartGroup>(A.Data);
			auto b = std::static_pointer_cast<otoworm::ChartGroup>(B.Data);
			return a->title < b->title;
		});
		break;
	case SORT_AUTHOR:
		SortByFn([](const ListEntry&A, const ListEntry&B)
		{
			auto a = std::static_pointer_cast<otoworm::ChartGroup>(A.Data);
			auto b = std::static_pointer_cast<otoworm::ChartGroup>(B.Data);
			return a->artist < b->artist;
		});
		break;
	case SORT_LENGTH:
		SortByFn([](const ListEntry&A, const ListEntry&B)
		{
			auto dur = [](std::shared_ptr<otoworm::ChartGroup> a)
			{
				auto chart = a->get_chart(0);
				if (chart) return chart->duration;
				
				return 0.0;
			};

			auto a = std::static_pointer_cast<otoworm::ChartGroup>(A.Data);
			auto b = std::static_pointer_cast<otoworm::ChartGroup>(B.Data);
			float lena = dur(a);
			float lenb = dur(b);
		
			return lena < lenb;
		});
		break;
	case SORT_MINLEVEL:
		SortByFn([](const ListEntry&A, const ListEntry&B)
		{
			auto nps = [](std::shared_ptr<otoworm::ChartGroup> a)
			{
				long long minnps = 10000000;
				for (auto chart : a->charts) {
					minnps = std::min(minnps, chart->level);
				}

				return minnps;
			};

			auto a = std::static_pointer_cast<otoworm::ChartGroup>(A.Data);
			auto b = std::static_pointer_cast<otoworm::ChartGroup>(B.Data);
			float npsa = nps(a);
			float npsb = nps(b);
		
			return npsa < npsb;
		});
		break;
	case SORT_MAXLEVEL:
		SortByFn([](const ListEntry&A, const ListEntry&B)
		{
			auto nps = [](std::shared_ptr<otoworm::ChartGroup> a)
			{
				long long maxnps = -10000000;
				for (auto chart : a->charts) {
					maxnps = std::max(maxnps, chart->level);
				}

				return maxnps;
			};

			auto a = std::static_pointer_cast<otoworm::ChartGroup>(A.Data);
			auto b = std::static_pointer_cast<otoworm::ChartGroup>(B.Data);
			float npsa = nps(a);
			float npsb = nps(b);
		
			return npsa < npsb;
		});
		break;
	default:
		break;
	}

	// recursively sort
	for (auto &&ch: mChildren)
	{
		if (ch.Kind == ListEntry::Directory)
		{
			auto list = std::static_pointer_cast<SongList>(ch.Data);
			list->SortBy(criteria);
		}
	}
}
