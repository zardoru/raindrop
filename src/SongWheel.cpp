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
#include <glm/gtc/matrix_transform.inl>

using namespace Game;

SongWheel::SongWheel()
{
	IsInitialized = false;
	PendingVerticalDisplacement = 0;
	mLoadMutex = nullptr;
	mLoadThread = nullptr;
	CurrentVerticalDisplacement = 0;
	VSRGModeActive = (Configuration::GetConfigf("VSRGEnabled") != 0);
	dotcurModeActive = (Configuration::GetConfigf("dotcurEnabled") != 0);
	DifficultyIndex = 0;

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

	ScrollSpeed = 90;
	SelectedItem = 0;
	SelectedListItem = 0;
	CursorPos = 0;
	OldCursorPos = 0;
	Time = 0;
	DisplacementSpeed = 2;

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
		std::map<GString, GString> Directories;
        std::map<GString, std::filesystem::path> Directories2;

		Configuration::GetConfigListS("SongDirectories", Directories, "Songs");
        Configuration::GetConfigListS("SongDirectories", Directories2, "Songs");

		SongLoader Loader (DB);

		Log::Printf("Started loading songs..\n");
		DB->StartTransaction();

		for (auto i = Directories.begin();
			i != Directories.end();
			++i)
		{
			ListRoot->AddNamedDirectory (*mLoadMutex, &Loader, i->second, i->first, VSRGActive, DCActive);
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

	ListRoot = nullptr;

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
	uint8 max_index = 0;
	if (!CurrentList->IsDirectory(SelectedItem))
	{
		DifficultyIndex--;
		if (CurrentList->GetSongEntry(SelectedItem)->Mode == MODE_VSRG)
		{
			auto Song = std::static_pointer_cast<VSRG::Song> (CurrentList->GetSongEntry(SelectedItem));
			max_index = Song->Difficulties.size() - 1;
		}
		else
		{
			auto Song = std::static_pointer_cast<dotcur::Song> (CurrentList->GetSongEntry(SelectedItem));
			max_index = Song->Difficulties.size() - 1;
		}
		
		DifficultyIndex = min(max_index, DifficultyIndex);
		OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
	}
	else
		DifficultyIndex = 0;

	return DifficultyIndex;
}

int SongWheel::NextDifficulty()
{
	if (!CurrentList->IsDirectory(SelectedItem))
	{
		DifficultyIndex++;
		if (CurrentList->GetSongEntry(SelectedItem)->Mode == MODE_VSRG)
		{
			auto Song = std::static_pointer_cast<VSRG::Song> (CurrentList->GetSongEntry(SelectedItem));
			if (DifficultyIndex >= Song->Difficulties.size())
				DifficultyIndex = 0;
		}
		else
		{
			auto Song = std::static_pointer_cast<dotcur::Song> (CurrentList->GetSongEntry(SelectedItem));
			if (DifficultyIndex >= Song->Difficulties.size())
				DifficultyIndex = 0;
		}

		OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
	}

	return DifficultyIndex;
}

bool SongWheel::InWheelBounds(Vec2 Pos)
{
	return Pos.x > TransformHorizontal(Pos.y) && Pos.x < TransformHorizontal(Pos.y) + ItemWidth;
}

void SongWheel::SetDifficulty(uint32 i)
{
	if (CurrentList && !CurrentList->IsDirectory(SelectedItem))
	{
		auto Song = std::static_pointer_cast<VSRG::Song> (GetSelectedSong());
		size_t maxIndex = Song->Difficulties.size();
		size_t oldDI = DifficultyIndex;

		if (maxIndex)
			DifficultyIndex = Clamp(i, uint32(0), uint32(maxIndex - 1));
		else
			DifficultyIndex = 0;

		if (DifficultyIndex != oldDI)
			OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
	}
}

bool SongWheel::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
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
			int32 boundIndex = GetCursorIndex();
			int32 Idx = GetListCursorIndex();
			if (boundIndex != CurrentList->GetNumEntries()) // There's entries!
			{
				if (InWheelBounds(mpos) || !isMouseInput)
				{
					if (OnItemClick)
						OnItemClick(Idx, boundIndex, CurrentList->GetEntryTitle(boundIndex), CurrentList->GetSongEntry(boundIndex));
					return true;
				}
			}
		}

		switch (key)
		{
		case 'Q':
			PendingVerticalDisplacement += 320;
			return true;
		case 'W':
			PendingVerticalDisplacement -= 320;
			return true;
		case 'E':
			GoUp();
			return true;
		}
	}

	return false;
}

void SongWheel::GoUp()
{
    std::unique_lock<std::mutex> lock (*mLoadMutex);

	if (CurrentList->HasParentDirectory())
	{
		CurrentList = CurrentList->GetParentDirectory();
		OnDirectoryChange();
	}
}

bool SongWheel::HandleScrollInput(const double dx, const double dy)
{
	return true;
}

std::shared_ptr<Game::Song> SongWheel::GetSelectedSong()
{
	return CurrentList->GetSongEntry(SelectedItem);
}

void SongWheel::Update(float Delta)
{
	uint32 Size = GetNumItems();

	Time += Delta;

	if (!IsLoading() && mLoadThread)
	{
		mLoadThread->join();
		delete mLoadThread;
		mLoadThread = nullptr;
	}

	if (!CurrentList)
		return;
	
	if (TransformPendingDisplacement) // We have a pending displacement transformation function
	{
		float Delta = TransformPendingDisplacement(PendingVerticalDisplacement);
		CurrentVerticalDisplacement += Delta;
		PendingVerticalDisplacement -= Delta;
	}
	else if (PendingVerticalDisplacement) // We don't, so we use the default hardcoded method
	{
		float ListDelta = PendingVerticalDisplacement * Delta * DisplacementSpeed;
		float NewListY = CurrentVerticalDisplacement + ListDelta;

		CurrentVerticalDisplacement = NewListY;
		PendingVerticalDisplacement -= ListDelta;
	}

	if (TransformListY)
		shownListY = TransformListY(CurrentVerticalDisplacement);
	else
		shownListY = CurrentVerticalDisplacement;

	Vec2 mpos = GameState::GetInstance().GetWindow()->GetRelativeMPos();
	float Transformed = TransformHorizontal(mpos.y);
	if (InWheelBounds(mpos))
	{
		IsHovering = true;
		CursorPos = IndexAtPoint(mpos.y);
	}
	else
	{
		if (IsHovering)
		{
			IsHovering = false;
			if (OnItemHoverLeave)
				OnItemHoverLeave(GetCursorIndex(), GetListCursorIndex(),
									CurrentList->GetEntryTitle(GetCursorIndex()), nullptr);
		}
	}

	// Hey we've got a new cursor position, update.
	if (OldCursorPos != CursorPos)
	{
		OldCursorPos = CursorPos;
		DifficultyIndex = 0;
		if (OnItemHover)
		{
            std::shared_ptr<Game::Song> Notify = GetSelectedSong();
			OnItemHover(GetCursorIndex(), GetListCursorIndex(),
						CurrentList->GetEntryTitle(GetCursorIndex()), Notify);
		}
	}
}

void SongWheel::DisplayItem(int32 ListItem, int32 ListPosition, Vec2 Position)
{
	if (Position.y > -ItemHeight && Position.y < ScreenHeight)
	{
		bool IsSelected = false;
        std::shared_ptr<Song> Song = nullptr;
		GString Text;
		if (ListItem != -1)
		{
			Song = CurrentList->GetSongEntry(ListItem);
			Text = CurrentList->GetEntryTitle(ListItem);
			IsSelected = (ListPosition == SelectedListItem);
		}

		for (auto Item = Sprites.begin(); Item != Sprites.end(); ++Item) {
			Item->second->SetPosition(Position);
					  
			if (TransformItem)
				TransformItem(Item->first, Song, IsSelected, ListPosition);

			// Render the objects.
			Item->second->Render();
		}

		for (auto Item = Strings.begin(); Item != Strings.end(); ++Item) {
			Item->second->SetPosition(Position);

			if (TransformString)
				TransformString(Item->first, Song, IsSelected, ListPosition, Text);

			Item->second->Render();
		}
	}
}

void SongWheel::CalculateIndices()
{
	int maxItems = ScreenHeight / ItemHeight; // This is how many items we want to draw.

	// I only really need the top index.
	float V = floor(-shownListY / ItemHeight);
	StartIndex = V;
	EndIndex = StartIndex + maxItems + 1;
}

void SongWheel::Render()
{
	int Index = GetCursorIndex();
    std::unique_lock<std::mutex> lock (*mLoadMutex);
	int Cur = 0;
	int Max = CurrentList->GetNumEntries();

	CalculateIndices();

	for (Cur = StartIndex; Cur <= EndIndex; Cur++)
	{
		float yTransform = Cur*ItemHeight + shownListY;
		float xTransform = TransformHorizontal(yTransform);
		Vec2 Position = Vec2(xTransform, yTransform);

		if (Max)
		{
			int RealIndex = Cur % Max;

			while (RealIndex < 0) // Loop over..
				RealIndex += Max;

			DisplayItem(RealIndex, Cur, Position);
		}else
			DisplayItem(-1, Cur, Position);
	}
}

int32 SongWheel::GetSelectedItem() const
{
	return SelectedListItem;
}

void SongWheel::SetItemHeight(float Height)
{
	ItemHeight = Height;
}

void SongWheel::SetItemWidth(float width)
{
	ItemWidth = width;
}

float SongWheel::GetItemWidth() const
{
	return ItemWidth;
}

void SongWheel::SetSelectedItem(int32 Item)
{
	if (!CurrentList)
		return;

	if (CurrentList->GetNumEntries() == 0)
		return;

	int32 OldListIndex = SelectedListItem;
	SelectedListItem = Item;

	// Get a bound item index
	while (Item < 0) Item += CurrentList->GetNumEntries();
	Item %= CurrentList->GetNumEntries();

	// Set bound item index to this.
	SelectedItem = Item;
	GameState::GetInstance().SetSelectedSong(GetSelectedSong());
	OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
}

int32 SongWheel::IndexAtPoint(float Y)
{
	float posy = Y;
	posy -= shownListY;
	// posy -= abs(fmod(posy, ItemHeight));
	posy = (posy / ItemHeight);
	return floor(posy);
}

uint32 SongWheel::NormalizedIndexAtPoint(float Y)
{
	int32 Idx = IndexAtPoint(Y);
	if (!Idx || !GetNumItems() || (Idx % GetNumItems() == 0) ) return 0;

	while (Idx > GetNumItems())
		Idx -= GetNumItems();
	while (Idx < 0)
		Idx += GetNumItems();
	return Idx;
}

int32 SongWheel::GetNumItems() const
{
	if (!CurrentList)
		return 0;
	else
	{
		return CurrentList->GetNumEntries();
	}
}

bool SongWheel::IsItemDirectory(int32 Item)
{
	if (CurrentList->GetNumEntries())
	{
		while (Item < 0) Item += CurrentList->GetNumEntries();
		Item %= CurrentList->GetNumEntries();
		return CurrentList->IsDirectory(Item);
	}

	return false;
}

void SongWheel::SetCursorIndex(int Index)
{
	CursorPos = Index;
}

void SongWheel::ConfirmSelection()
{
	if (!CurrentList->IsDirectory(SelectedItem))
	{
		OnSongConfirm(GetSelectedSong(), DifficultyIndex);
	} else
	{
		CurrentList = CurrentList->GetListEntry(SelectedItem).get();
		SetSelectedItem(SelectedListItem); // Update our selected item to new bounderies.
		OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
	}
}

int SongWheel::GetListCursorIndex() const
{
	return CursorPos;
}

float SongWheel::GetItemHeight() const
{
	return ItemHeight;
}

float SongWheel::GetListY() const
{
	return CurrentVerticalDisplacement;
}

void SongWheel::SetListY(float List)
{
	CurrentVerticalDisplacement = List;
}

float SongWheel::GetDeltaY() const
{
	return PendingVerticalDisplacement;
}

void SongWheel::SetDeltaY(float PD)
{
	PendingVerticalDisplacement = PD;
}

float SongWheel::GetTransformedY() const
{
	return shownListY;
}

bool SongWheel::IsLoading()
{
	if (mLoadThread)
	{
		return mLoading;
	}
	else return false;
}