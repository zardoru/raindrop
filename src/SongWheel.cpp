#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "SongDC.h"
#include "Song7K.h"
#include "GameWindow.h"
#include "SongLoader.h"
#include "GraphicalString.h"
#include "Sprite.h"
#include "SongWheel.h"
#include "SongList.h"

#include "SongDatabase.h"
//#include <glm/gtc/matrix_transform.inl>

using namespace Game;

SongWheel::SongWheel()
{
    IsInitialized = false;
    mLoadMutex = nullptr;
    mLoadThread = nullptr;
    VSRGModeActive = (Configuration::GetConfigf("VSRGEnabled") != 0);
	dotcurModeActive = false;
    // dotcurModeActive = (Configuration::GetConfigf("dotcurEnabled") != 0);
    DifficultyIndex = 0;

	DisplayStartIndex = 0;
	DisplayItemCount = 0;

    ListRoot = nullptr;
    CurrentList = nullptr;
    LoadedSongsOnce = false;
    IsHovering = false;
}

int SongWheel::GetDifficulty() const
{
    return DifficultyIndex;
}

SongWheel& SongWheel::GetInstance()
{
    static auto WheelInstance = new SongWheel();
    return *WheelInstance;
}

void SongWheel::CleanItems()
{
    Strings.clear();
    Sprites.clear();
}


void SongWheel::Initialize(SongDatabase* Database)
{
    if (IsInitialized)
    {
        CleanItems();
        return;
    }

    SelectedBoundItem = 0;
    SelectedUnboundItem = 0;
    CursorPos = 0;
    OldCursorPos = 0;
    Time = 0;

    IsInitialized = true;
    DifficultyIndex = 0;

    LoadSongsOnce(Database);
}

class LoadThread
{
    std::mutex* mLoadMutex;
    SongDatabase* DB;
    std::shared_ptr<SongList> ListRoot;
    bool VSRGActive;
    bool DCActive;
    std::atomic<bool>& isLoading;
public:
    LoadThread(std::mutex* m, SongDatabase* d, std::shared_ptr<SongList> r, bool va, bool da, std::atomic<bool>& loadingstatus)
        : mLoadMutex(m),
        DB(d),
        ListRoot(r),
        VSRGActive(va),
        DCActive(da),
        isLoading(loadingstatus)
    {
        isLoading = true;
    }

    void Load()
    {
        std::map<std::string, std::string> Directories;

        Configuration::GetConfigListS("SongDirectories", Directories, "Songs");

        SongLoader Loader(DB);

        Log::Printf("Started loading songs..\n");
        DB->StartTransaction();

        for (auto i = Directories.begin();
        i != Directories.end();
            ++i)
        {
            ListRoot->AddNamedDirectory(*mLoadMutex, &Loader, i->second, i->first, VSRGActive, DCActive);
			SongWheel::GetInstance().ReapplyFilters();
        }

        DB->EndTransaction();
        Log::Printf("Finished reloading songs.\n");
        isLoading = false;
    }
};

void SongWheel::Join()
{
    if (mLoadThread)
    {
        mLoadThread->join();
        delete mLoadThread;
        mLoadThread = nullptr;
    }
}

void SongWheel::ReloadSongs(SongDatabase* Database)
{
    DB = Database;
    Join();

    ListRoot = std::make_shared<SongList>();
    CurrentList = ListRoot.get();

    if (!mLoadMutex)
        mLoadMutex = new std::mutex;

    LoadThread L(mLoadMutex, DB, ListRoot, VSRGModeActive, dotcurModeActive, mLoading);
    mLoadThread = new std::thread(&LoadThread::Load, L);
}

void SongWheel::LoadSongsOnce(SongDatabase* Database)
{
    if (!LoadedSongsOnce) LoadedSongsOnce = true;
    else return;
    ReloadSongs(Database);
}

int SongWheel::AddSprite(Sprite* Item)
{
    int size = Sprites.size() + 1;
    Sprites[size] = Item;
    return size;
}

int SongWheel::AddText(GraphicalString* Str)
{
    int size = Strings.size() + 1;
    Strings[size] = Str;
    return size;
}

int SongWheel::GetCursorIndex() const
{
    size_t Size;
    Size = GetNumItems();

    if (Size)
    {
        int ret = CursorPos % (int)Size;
        while (ret < 0)
            ret += Size;
        return ret;
    }

    return 0;
}

int SongWheel::PrevDifficulty()
{
    size_t max_index = 0;
    if (!FilteredCurrentList.IsDirectory(SelectedBoundItem))
    {
        DifficultyIndex--;
        if (FilteredCurrentList.GetSongEntry(SelectedBoundItem)->Mode == MODE_VSRG)
        {
            auto Song = std::static_pointer_cast<VSRG::Song> (FilteredCurrentList.GetSongEntry(SelectedBoundItem));
            max_index = Song->Difficulties.size() - 1;
        }
        else
        {
            auto Song = std::static_pointer_cast<dotcur::Song> (FilteredCurrentList.GetSongEntry(SelectedBoundItem));
            max_index = Song->Difficulties.size() - 1;
        }

        DifficultyIndex = std::min(max_index, DifficultyIndex);
        OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
    }
    else
        DifficultyIndex = 0;

    return DifficultyIndex;
}

int SongWheel::NextDifficulty()
{
    if (!FilteredCurrentList.IsDirectory(SelectedBoundItem))
    {
        DifficultyIndex++;
        if (FilteredCurrentList.GetSongEntry(SelectedBoundItem)->Mode == MODE_VSRG)
        {
            auto Song = std::static_pointer_cast<VSRG::Song> (FilteredCurrentList.GetSongEntry(SelectedBoundItem));
            if (DifficultyIndex >= Song->Difficulties.size())
                DifficultyIndex = 0;
        }
        else
        {
            auto Song = std::static_pointer_cast<dotcur::Song> (FilteredCurrentList.GetSongEntry(SelectedBoundItem));
            if (DifficultyIndex >= Song->Difficulties.size())
                DifficultyIndex = 0;
        }

        OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
    }

    return DifficultyIndex;
}

bool SongWheel::InWheelBounds(Vec2 Pos)
{
	return IndexAtPoint(Pos.x, Pos.y) != -1;
}

AABBd Game::SongWheel::ItemBoxAt(float t)
{
	Vec2 Position(TransformHorizontal(t), TransformVertical(t));
	Vec2 Size(TransformWidth(t), TransformHeight(t));

	// az: No + operator, lol?
	Vec2 BottomRightCorner = Position;
	BottomRightCorner += Size;

	return AABBd(Position.x, Position.y, BottomRightCorner.x, BottomRightCorner.y);
}

void SongWheel::SetDifficulty(uint32_t i)
{
    if (!FilteredCurrentList.IsDirectory(SelectedBoundItem))
    {
        auto Song = std::static_pointer_cast<VSRG::Song> (GetSelectedSong());
        size_t maxIndex = Song->Difficulties.size();
        size_t oldDI = DifficultyIndex;

        if (maxIndex)
            DifficultyIndex = Clamp(i, uint32_t(0), uint32_t(maxIndex - 1));
        else
            DifficultyIndex = 0;

        if (DifficultyIndex != oldDI)
            OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
    }
}

bool SongWheel::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
{
    if (code == KE_PRESS)
    {
        switch (BindingsManager::TranslateKey(key))
        {
        default:
            break;
        case KT_Up:
            CursorPos--;
            return true;
        case KT_Down:
            CursorPos++;
            return true;
        case KT_Select:
            Vec2 mpos = GameState::GetWindow()->GetRelativeMPos();
            auto boundIndex = GetCursorIndex();
            auto Idx = GetListCursorIndex();
            if (boundIndex != FilteredCurrentList.GetNumEntries()) // There's entries!
            {
                if (InWheelBounds(mpos) || !isMouseInput)
                {
                    if (OnItemClick)
                        OnItemClick(Idx, boundIndex, 
							FilteredCurrentList.GetEntryTitle(boundIndex), 
							FilteredCurrentList.GetSongEntry(boundIndex));
                    return true;
                }
            }
        }

        switch (key)
        {
        case 'E':
            GoUp();
            return true;
        }
    }

    return false;
}

void SongWheel::GoUp()
{
    std::unique_lock<std::mutex> lock(*mLoadMutex);

    if (CurrentList->HasParentDirectory())
    {
		CurrentList->SetInUse(false);
        CurrentList = CurrentList->GetParentDirectory();
		CurrentList->ClearEmpty();
		ReapplyFilters();
        OnDirectoryChange();
		OnSongTentativeSelect(GetSelectedSong(), 0);
    }
}

bool SongWheel::HandleScrollInput(const double dx, const double dy)
{
    return true;
}

std::shared_ptr<Game::Song> SongWheel::GetSelectedSong()
{
    return FilteredCurrentList.GetSongEntry(SelectedBoundItem);
}

void SongWheel::Update(float Delta)
{
    uint32_t Size = GetNumItems();

    Time += Delta;

    if (!IsLoading() && mLoadThread)
    {
        mLoadThread->join();
        delete mLoadThread;
        mLoadThread = nullptr;
    }

    if (!CurrentList)
        return;

    Vec2 mpos = GameState::GetInstance().GetWindow()->GetRelativeMPos();
    if (InWheelBounds(mpos))
    {
        IsHovering = true;
        CursorPos = IndexAtPoint(mpos.x, mpos.y);
    }
    else
    {
        if (IsHovering)
        {
            IsHovering = false;
            if (OnItemHoverLeave)
                OnItemHoverLeave(GetCursorIndex(), GetListCursorIndex(),
                FilteredCurrentList.GetEntryTitle(GetCursorIndex()), nullptr);
        }
    }

    // Hey we've got a new cursor position, update.
    if (OldCursorPos != CursorPos)
    {
        OldCursorPos = CursorPos;
        if (OnItemHover)
        {
            std::shared_ptr<Game::Song> Notify = GetSelectedSong();
            OnItemHover(GetCursorIndex(), GetListCursorIndex(),
                FilteredCurrentList.GetEntryTitle(GetCursorIndex()), Notify);
        }
    }
}

void SongWheel::DisplayItem(int32_t ListItem, int32_t ListPosition, float itemFraction)
{
	AABBd screen_box (0.0, 0.0, ScreenWidth, ScreenHeight);
	AABBd item_box = ItemBoxAt(itemFraction);
	Vec2 pos(item_box.X1, item_box.Y1);
	// Vec2 size (item_box.width(), item_box.height()); 

    if (screen_box.Intersects(item_box))
    {
        bool IsSelected = false;
        std::shared_ptr<Song> Song = nullptr;
        std::string Text;

        if (ListItem != -1)
        {
            Song = FilteredCurrentList.GetSongEntry(ListItem);
            Text = FilteredCurrentList.GetEntryTitle(ListItem);
            IsSelected = (ListPosition == SelectedUnboundItem);
        }

        for (auto Item = Sprites.begin(); Item != Sprites.end(); ++Item)
        {			
            Item->second->SetPosition(item_box.X1, item_box.Y1);

            if (TransformItem)
                TransformItem(Item->first, Song, IsSelected, ListPosition);

            // Render the objects.
            Item->second->Render();
        }

        for (auto Item = Strings.begin(); Item != Strings.end(); ++Item)
        {
            Item->second->SetPosition(pos);	

            if (TransformString)
                TransformString(Item->first, Song, IsSelected, ListPosition, Text);

            Item->second->Render();
        }
    }
}


void SongWheel::Render()
{
    int Index = GetCursorIndex();
    std::unique_lock<std::mutex> lock(*mLoadMutex);
    int Cur = 0;
    int Max = FilteredCurrentList.GetNumEntries();

	// I only really need the top index.
	int DisplayEndIndex = DisplayStartIndex + DisplayItemCount + 1;

	float TotalEntries = DisplayEndIndex - DisplayStartIndex;
    for (Cur = DisplayStartIndex; Cur <= DisplayEndIndex; Cur++)
    {
		// Cur*ItemHeight + shownListY

		float t = (Cur - DisplayStartIndex) / TotalEntries;
        if (Max)
        {
            int RealIndex = Cur % Max;

            while (RealIndex < 0) // Loop over..
                RealIndex += Max;

            DisplayItem(RealIndex, Cur, t);
        }
        else
            DisplayItem(-1, Cur, t);
    }
}

int32_t SongWheel::GetSelectedItem() const
{
    return SelectedUnboundItem;
}


void SongWheel::SetSelectedItem(int32_t Item)
{
    if (!CurrentList)
        return;

    SelectedUnboundItem = Item;

    // Get a bound item index
	if (GetNumItems())
		Item = modulo(Item, GetNumItems());
	else
		Item = 0;
    
    // Set bound item index to this.
    SelectedBoundItem = Item;
    GameState::GetInstance().SetSelectedSong(GetSelectedSong());
    OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
}

int32_t SongWheel::IndexAtPoint(float X, float Y)
{
	for (int cur = DisplayStartIndex;
		cur != DisplayStartIndex + DisplayItemCount + 1;
		cur++) {
		float t = float(cur - DisplayStartIndex) / float(DisplayItemCount + 1);

		if (ItemBoxAt(t).IsInBox(X, Y))
			return cur;
	}

	return -1;
}

uint32_t SongWheel::NormalizedIndexAtPoint(float X, float Y)
{
    auto Idx = modulo(IndexAtPoint(X, Y), GetNumItems());
    return Idx;
}

int32_t SongWheel::GetNumItems() const
{
    if (!CurrentList)
        return 0;
    else
    {
        return FilteredCurrentList.GetNumEntries();
    }
}

bool SongWheel::IsItemDirectory(int32_t Item) const
{
    if (FilteredCurrentList.GetNumEntries())
    {
        while (Item < 0) Item += GetNumItems();
        Item %= GetNumItems();
        return FilteredCurrentList.IsDirectory(Item);
    }

    return false;
}

void SongWheel::SetCursorIndex(int Index)
{
    CursorPos = Index;
}

void SongWheel::ConfirmSelection()
{
    if (!FilteredCurrentList.IsDirectory(SelectedBoundItem))
    {
        OnSongConfirm(GetSelectedSong(), DifficultyIndex);
    }
    else
    {
        CurrentList = CurrentList->GetListEntry(SelectedBoundItem).get();
		CurrentList->SetInUse(true);

		ReapplyFilters();
        SetSelectedItem(SelectedUnboundItem); // Update our selected item to new bounderies.
        OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
    }
}

int SongWheel::GetListCursorIndex() const
{
    return CursorPos;
}

bool SongWheel::IsLoading()
{
    if (mLoadThread)
    {
        return mLoading;
    }
    else return false;
}

void SongWheel::SortBy(ESortCriteria criteria)
{
	std::unique_lock<std::mutex> lock(*mLoadMutex);
	ListRoot->SortBy(criteria);
	ReapplyFilters();
}

void Game::SongWheel::ReapplyFilters()
{
	if (!CurrentList) return;

	FilteredCurrentList.Clear();
	for (auto entry : CurrentList->GetEntries()) {
		bool add = true;

		if (entry.Kind == ListEntry::Song) {
			auto song = std::static_pointer_cast<Game::Song>(entry.Data);
			if (!GameState::GetInstance().IsSongUnlocked(song.get()))
				continue;
		}

		// all filters pass?
		// no filters means next block is skipped, so always adds
		for (auto can_add : ActiveFilters) {
			// this one doesn't
			if (!can_add(&entry)) {
				add = false;
				break;
			}
		}

		// all filters passed!
		if (add)
			FilteredCurrentList.AddEntry(entry);
	}
}

void Game::SongWheel::ResetFilters()
{
	ActiveFilters.clear();
	ReapplyFilters();
}

void Game::SongWheel::SelectBy(FuncFilterCriteria criteria)
{
	ActiveFilters.push_back(criteria);
	ReapplyFilters();
}
