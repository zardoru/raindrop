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

void SongWheel::Initialize(float Start, float End, SongDatabase* Database)
{
	DB = Database;

	if (IsInitialized)
		return;

	ListRoot = NULL;
	CurrentList = NULL;

	ScrollSpeed = 90;
	SelectedItem = 0;
	CursorPos = 0;
	OldCursorPos = 0;
	Time = 0;
	DisplacementSpeed = 2;

	Item = new GraphObject2D;
	Item->SetImage(GameState::GetInstance().GetSkinImage("item.png"));
	ItemHeight = Item->GetHeight();
	Item->SetZ(16);

	ItemTextOffset = Vec2(Configuration::GetSkinConfigf("X", "ItemTextOffset"), Configuration::GetSkinConfigf("Y", "ItemTextOffset"));

	IsInitialized = true;
	DifficultyIndex = 0;

	//mFont = new BitmapFont();
	//mFont->LoadSkinFontImage("font-wheel.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);

	double FSize = Configuration::GetSkinConfigf("Size", "WheelFont");
	GString fname = Configuration::GetSkinConfigs("Font", "WheelFont");
	mTFont = new TruetypeFont(GameState::GetInstance().GetSkinFile(fname), FSize);
	ReloadSongs();
}

class LoadThread
{
	boost::mutex* mLoadMutex;
	SongDatabase* DB;
	SongList* ListRoot;
	bool VSRGActive;
	bool DCActive;
public:
	LoadThread(boost::mutex* m, SongDatabase* d, SongList* r, bool va, bool da)
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

		for (std::map<GString, GString>::iterator i = Directories.begin();
			i != Directories.end();
			i++)
		{
			ListRoot->AddNamedDirectory (*mLoadMutex, &Loader, i->second, i->first, VSRGActive, DCActive);
		}

		DB->EndTransaction();
		Log::Printf("Finished reloading songs.\n");
	}
};

void SongWheel::ReloadSongs()
{
	if (mLoadThread)
	{
		mLoadThread->join();
		delete mLoadThread;
	}

	delete ListRoot;

	ListRoot = new SongList();
	CurrentList = ListRoot;

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
			max_index = ((VSRG::Song*)CurrentList->GetSongEntry(SelectedItem))->Difficulties.size() - 1;
		}
		else
		{
			max_index = ((dotcur::Song*)CurrentList->GetSongEntry(SelectedItem))->Difficulties.size() - 1;
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
			if (DifficultyIndex >= ((VSRG::Song*)CurrentList->GetSongEntry(SelectedItem))->Difficulties.size())
				DifficultyIndex = 0;
		}
		else
		{
			if (DifficultyIndex >= ((dotcur::Song*)CurrentList->GetSongEntry(SelectedItem))->Difficulties.size())
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
	if (!CurrentList->IsDirectory(SelectedItem))
	{
		size_t maxIndex = ((VSRG::Song*)CurrentList->GetSongEntry(SelectedItem))->Difficulties.size();

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
						CurrentList = CurrentList->GetListEntry(Idx);
						if (OnDirectoryChange) OnDirectoryChange();
					}
				}
				return true;
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

Game::Song* SongWheel::GetSelectedSong()
{
	return CurrentList->GetSongEntry(SelectedItem);
}

void SongWheel::Update(float Delta)
{
	uint32 Size = GetNumItems();

	Time += Delta;

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
		DifficultyIndex = 0;
		if (OnItemHover)
		{
			Game::Song* Notify = GetSelectedSong();
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
		Game::Song* Song = NULL;
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

	Game::Song *ToDisplay;

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

			if (!CurrentList->IsDirectory(RealIndex))
			{
				ToDisplay = CurrentList->GetSongEntry(RealIndex);
				DisplayItem(RealIndex, Position);
			}
			else
			{
				DisplayItem(RealIndex, Position);
			}
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
		GameState::GetInstance().SetSelectedSong(GetSelectedSong());
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