#include <stdio.h>
#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "SongDC.h"
#include "Song7K.h"
#include "GameWindow.h"
#include "SongLoader.h"
#include "SongWheel.h"
#include "GraphObject2D.h"
#include "ImageLoader.h"
#include "BitmapFont.h"
#include "TruetypeFont.h"
#include "SongList.h"

#include "SongDatabase.h"

using namespace Game;

SongWheel::SongWheel()
{
	IsInitialized = false;
	mFont = NULL;
	PendingVerticalDisplacement = 0;
	mLoadMutex = NULL;
	mLoadThread = NULL;
	CurrentVerticalDisplacement = 0;
	VSRGModeActive = (Configuration::GetConfigf("VSRGEnabled") != 0);
	dotcurModeActive = (Configuration::GetConfigf("dotcurEnabled") != 0);
	DifficultyIndex = 0;

	ListRoot = NULL;
	CurrentList = NULL;

	IsHovering = false;
}

int SongWheel::GetDifficulty() const
{
	return DifficultyIndex;
}

SongWheel& SongWheel::GetInstance()
{
	static SongWheel *WheelInstance = new SongWheel();
	return *WheelInstance;
}

void SongWheel::Initialize(float Start, float End, SongDatabase* Database, bool IsGraphical)
{
	DB = Database;

	if (IsInitialized)
		return;

	ScrollSpeed = 90;
	SelectedItem = 0;
	CursorPos = 0;
	OldCursorPos = 0;
	Time = 0;
	DisplacementSpeed = 2;
	if (IsGraphical)
	{
		Item = new GraphObject2D;
		Item->SetImage(GameState::GetInstance().GetSkinImage("item.png"));
		ItemHeight = Item->GetHeight();
		Item->SetZ(16);

		ItemTextOffset = Vec2(Configuration::GetSkinConfigf("X", "ItemTextOffset"), Configuration::GetSkinConfigf("Y", "ItemTextOffset"));

		double FSize = Configuration::GetSkinConfigf("Size", "WheelFont");
		GString fname = Configuration::GetSkinConfigs("Font", "WheelFont");
        if (fname != "")
        {
            mTFont = new TruetypeFont(GameState::GetInstance().GetSkinFile(fname), FSize);
        }
	}

	IsInitialized = true;
	DifficultyIndex = 0;

	ReloadSongs();
}

class LoadThread
{
	boost::mutex* mLoadMutex;
	SongDatabase* DB;
	shared_ptr<SongList> ListRoot;
	bool VSRGActive;
	bool DCActive;
public:
	LoadThread(boost::mutex* m, SongDatabase* d, shared_ptr<SongList> r, bool va, bool da)
		: mLoadMutex(m),
		DB(d),
		ListRoot(r),
		VSRGActive(va),
		DCActive(da)
	{
	}

	void Load()
	{
		std::map<GString, GString> Directories;
		Configuration::GetConfigListS("SongDirectories", Directories, "Songs");

		SongLoader Loader (DB);

		Log::Printf("Started loading songs..\n");
		DB->StartTransaction();

		for (auto i = Directories.begin();
			i != Directories.end();
			i++)
		{
			ListRoot->AddNamedDirectory (*mLoadMutex, &Loader, i->second, i->first, VSRGActive, DCActive);
		}

		DB->EndTransaction();
		Log::Printf("Finished reloading songs.\n");
	}
};

void SongWheel::Join()
{
	if (mLoadThread)
	{
		mLoadThread->join();
		delete mLoadThread;
	}
}


void SongWheel::ReloadSongs()
{
	Join();

	ListRoot.reset();

	ListRoot = make_shared<SongList>();
	CurrentList = ListRoot.get();

	if (!mLoadMutex)
		mLoadMutex = new boost::mutex;

	LoadThread L(mLoadMutex, DB, ListRoot, VSRGModeActive, dotcurModeActive);
	mLoadThread = new boost::thread(&LoadThread::Load, L);
}

uint32 SongWheel::GetCursorIndex()
{
	size_t Size;
	Size = GetNumItems();

	if (Size)
	{
		int ret = CursorPos % (int)Size;
		while (ret < 0)
			ret += Size;
		return (uint32)ret;
	}
	else
		return 0;
}

int SongWheel::PrevDifficulty()
{
	uint8 max_index = 0;
	if (!CurrentList->IsDirectory(SelectedItem))
	{
		DifficultyIndex--;
		max_index = 0;
		if (CurrentList->GetSongEntry(SelectedItem)->Mode == MODE_VSRG)
		{
			shared_ptr<VSRG::Song> Song = static_pointer_cast<VSRG::Song> (CurrentList->GetSongEntry(SelectedItem));
			max_index = Song->Difficulties.size() - 1;
		}
		else
		{
			shared_ptr<dotcur::Song> Song = static_pointer_cast<dotcur::Song> (CurrentList->GetSongEntry(SelectedItem));
			max_index = Song->Difficulties.size() - 1;
		}
	}

	DifficultyIndex = min(max_index, DifficultyIndex);
	return DifficultyIndex;
}

int SongWheel::NextDifficulty()
{
	if (!CurrentList->IsDirectory(SelectedItem))
	{
		DifficultyIndex++;
		if (CurrentList->GetSongEntry(SelectedItem)->Mode == MODE_VSRG)
		{
			shared_ptr<VSRG::Song> Song = static_pointer_cast<VSRG::Song> (CurrentList->GetSongEntry(SelectedItem));
			if (DifficultyIndex >= Song->Difficulties.size())
				DifficultyIndex = 0;
		}
		else
		{
			shared_ptr<dotcur::Song> Song = static_pointer_cast<dotcur::Song> (CurrentList->GetSongEntry(SelectedItem));
			if (DifficultyIndex >= Song->Difficulties.size())
				DifficultyIndex = 0;
		}
	}

	return DifficultyIndex;
}

bool SongWheel::InWheelBounds(Vec2 Pos)
{
	return Pos.x > TransformHorizontal(Pos.y) && Pos.x < TransformHorizontal(Pos.y) + Item->GetWidth();
}

void SongWheel::SetDifficulty(uint32 i)
{
	if (CurrentList && !CurrentList->IsDirectory(SelectedItem))
	{
		shared_ptr<VSRG::Song> Song = static_pointer_cast<VSRG::Song> (CurrentList->GetSongEntry(SelectedItem));
		size_t maxIndex = Song->Difficulties.size();

		if (maxIndex)
			DifficultyIndex = Clamp(i, (uint32)0, (uint32)(maxIndex - 1));
		else
			DifficultyIndex = 0;
	}
}

bool SongWheel::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (code == KE_Press)
	{
		switch (BindingsManager::TranslateKey(key))
		{

		case KT_Up:
			CursorPos--;
			return true;
		case KT_Down:
			CursorPos++;
			return true;
		case KT_Select:
			Vec2 mpos = GameState::GetWindow()->GetRelativeMPos();
			uint32 Idx = GetCursorIndex();
			if (Idx < CurrentList->GetNumEntries())
			{
				if (InWheelBounds(mpos) || !isMouseInput)
				{
					if (OnItemClick)
						OnItemClick(Idx, CurrentList->GetEntryTitle(Idx), CurrentList->GetSongEntry(Idx));

					if (CurrentList->IsDirectory(Idx))
					{
						CurrentList = CurrentList->GetListEntry(Idx).get();

						if (OnDirectoryChange) OnDirectoryChange();

						if (GetSelectedSong()) {
							GameState::GetInstance().SetSelectedSong(GetSelectedSong().get());
							GameState::GetInstance().SetDifficultyIndex(0);
							DifficultyIndex = 0;
							OnSongTentativeSelect(GetSelectedSong(), 0);
						}
					}
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
	boost::mutex::scoped_lock lock (*mLoadMutex);

	if (CurrentList->HasParentDirectory())
	{
		CurrentList = CurrentList->GetParentDirectory();
		OnDirectoryChange();
	}
}

bool SongWheel::HandleScrollInput(const double dx, const double dy)
{
	PendingVerticalDisplacement += dy * ScrollSpeed;
	return true;
}

shared_ptr<Game::Song> SongWheel::GetSelectedSong()
{
	return CurrentList->GetSongEntry(SelectedItem);
}

void SongWheel::Update(float Delta)
{
	uint32 Size = GetNumItems();

	Time += Delta;

	if (!IsLoading() && mLoadThread)
	{
		delete mLoadThread; mLoadThread = NULL;
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
				OnItemHoverLeave(GetCursorIndex(), CurrentList->GetEntryTitle(GetCursorIndex()), NULL);
		}
	}

	// Hey we've got a new cursor position, update.
	if (OldCursorPos != CursorPos)
	{
		OldCursorPos = CursorPos;
		if (OnItemHover)
		{
			shared_ptr<Game::Song> Notify = GetSelectedSong();
			OnItemHover(GetCursorIndex(), CurrentList->GetEntryTitle(GetCursorIndex()), Notify);
		}
	}
}

void SongWheel::DisplayItem(int32 ListItem, Vec2 Position)
{
	if (Position.y > -ItemHeight && Position.y < ScreenHeight)
	{
		bool IsDirectory = true;
		bool IsSelected = false;
		shared_ptr<Game::Song> Song = NULL;
		GString Text;

		Item->SetPosition(Position);

		if (ListItem != -1)
		{
			Song = CurrentList->GetSongEntry(ListItem);
			Text = CurrentList->GetEntryTitle(ListItem);
			IsSelected = (ListItem == SelectedItem);
		}

		if (TransformItem)
			TransformItem(Item, Song, IsSelected); // third arg: 'IsSelected'

		// Render the objects.
		Item->Render();
		
		if (Text.length())
			mTFont->Render(Text, Position + ItemTextOffset);
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
	boost::mutex::scoped_lock lock (*mLoadMutex);
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

			DisplayItem(RealIndex, Position);
		}else
			DisplayItem(-1, Position);
	}
}

int32 SongWheel::GetSelectedItem()
{
	return SelectedItem;
}

void SongWheel::SetSelectedItem(uint32 Item)
{
	if (!CurrentList)
		return;

	if (CurrentList->GetNumEntries() <= Item)
		return;

	if (SelectedItem != Item)
	{
		SelectedItem = Item;
		GameState::GetInstance().SetSelectedSong(GetSelectedSong().get());
		OnSongTentativeSelect(GetSelectedSong(), DifficultyIndex);
	}
	else if (!CurrentList->IsDirectory(Item))
		OnSongConfirm(CurrentList->GetSongEntry(Item), DifficultyIndex);
}

int32 SongWheel::IndexAtPoint(float Y)
{
	float posy = Y;
	posy -= shownListY;
	posy -= (int)posy % (int)ItemHeight;
	posy = (posy / ItemHeight);
	return floor(posy);
}

uint32 SongWheel::NormalizedIndexAtPoint(float Y)
{
	int32 Idx = IndexAtPoint(Y);
	if (!Idx || !GetNumItems() || (Idx % GetNumItems() == 0) ) return 0;

	while (Idx > 0)
		Idx -= GetNumItems();
	while (Idx < 0)
		Idx += GetNumItems();
	return Idx;
}

int32 SongWheel::GetNumItems()
{
	if (!CurrentList)
		return 0;
	else
	{
		return CurrentList->GetNumEntries();
	}
}

void SongWheel::SetCursorIndex(int32 Index)
{
	CursorPos = Index;
}

float SongWheel::GetItemHeight()
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
		if (!mLoadThread->timed_join(boost::posix_time::milliseconds(0))) // Not complete
			return true;
		else
		{
			return false;
		}
	}
	else return false;
}