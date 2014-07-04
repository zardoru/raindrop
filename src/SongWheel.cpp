#include "GameGlobal.h"
#include "SongDC.h"
#include "Song7K.h"
#include "FileManager.h"
#include "GameWindow.h"
#include "GameState.h"
#include "SongWheel.h"
#include "GraphObject2D.h"
#include "ImageLoader.h"
#include "BitmapFont.h"
#include "SongList.h"

using namespace Game;

SongWheel::SongWheel()
{
	Transform = NULL;
	OnSongChange = NULL;
	IsInitialized = false;
	mFont = NULL;
	PendingVerticalDisplacement = 0;
	CurrentVerticalDisplacement = 0;
}

SongWheel& SongWheel::GetInstance()
{
	static SongWheel *WheelInstance = new SongWheel();
	return *WheelInstance;
}

void SongWheel::Initialize(float Start, float End, bool IsDotcurActive, bool IsVSRGActive, ListTransformFunction FuncTransform, SongNotification FuncNotify, SongNotification FuncNotifySelect)
{
	dotcurModeActive = IsDotcurActive;
	VSRGModeActive = IsVSRGActive;

	OnSongChange = FuncNotify;
	OnSongSelect = FuncNotifySelect;
	Transform = FuncTransform;

	if (IsInitialized)
		return;

	ListRoot = NULL;
	CurrentList = NULL;

	RangeStart = Start;
	RangeEnd = End;

	CursorPos = 0;
	OldCursorPos = 0;
	Time = 0;
	DisplacementSpeed = 2;
	SelCursor = new GraphObject2D;
	SelCursor->SetImage(ImageLoader::LoadSkin("songselect_cursor.png"));
	SelCursor->SetSize(20);

	IsInitialized = true;
	DifficultyIndex = 0;

	mFont = new BitmapFont();
	mFont->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);

	ItemHeight = 20;
}

void SongWheel::ReloadSongs()
{
	delete ListRoot;

	ListRoot = new SongList();

	std::vector<String> Directories;
	Configuration::GetConfigListS("SongDirectories", Directories);

	for (std::vector<String>::iterator i = Directories.begin(); 
		i != Directories.end();
		i++)
	{
		ListRoot->AddDirectory (*i, VSRGModeActive, dotcurModeActive);
	}
	
	CurrentList = ListRoot;
	GameState::Printf("Finished reloading songs.\n");
}

bool SongWheel::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	uint8 max_index = 0;

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
		case KT_Left:
			if (CursorPos < CurrentList->GetNumEntries() && !CurrentList->IsDirectory(CursorPos))
			{
				DifficultyIndex--;
				max_index = 0;
				if (CurrentList->GetSongEntry(CursorPos)->Mode == MODE_7K)
				{
					max_index = ((VSRG::Song*)CurrentList->GetSongEntry(CursorPos))->Difficulties.size()-1;
				}else
				{
					max_index = ((dotcur::Song*)CurrentList->GetSongEntry(CursorPos))->Difficulties.size()-1;
				}
			}

			DifficultyIndex = min(max_index, DifficultyIndex);
			return true;
		case KT_Right:
			if (CursorPos < CurrentList->GetNumEntries() && !CurrentList->IsDirectory(CursorPos))
			{
				DifficultyIndex++;
				if (CurrentList->GetSongEntry(CursorPos)->Mode == MODE_7K)
				{
					if (DifficultyIndex >= ((VSRG::Song*)CurrentList->GetSongEntry(CursorPos))->Difficulties.size())
						DifficultyIndex = 0;
				}else
				{
					if (DifficultyIndex >= ((dotcur::Song*)CurrentList->GetSongEntry(CursorPos))->Difficulties.size())
						DifficultyIndex = 0;
				}
			}
			return true;
		case KT_Select:
			Vec2 mpos = GameState::GetWindow()->GetRelativeMPos();
			if (CursorPos < CurrentList->GetNumEntries() && 
				(!isMouseInput || mpos.x > Transform(mpos.y) && mpos.y > CurrentVerticalDisplacement))
			{
				if (!CurrentList->IsDirectory(CursorPos))
					OnSongSelect(CurrentList->GetSongEntry(CursorPos), DifficultyIndex);
				else
					CurrentList = CurrentList->GetListEntry(CursorPos);

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
			if (CurrentList->HasParentDirectory())
				CurrentList = CurrentList->GetParentDirectory();
			return true;
		}
	}

	return false;
}

bool SongWheel::HandleScrollInput(const double dx, const double dy)
{
	PendingVerticalDisplacement += dy * 90;
	return true;
}

Game::Song* SongWheel::GetSelectedSong()
{
	if (CursorPos >= CurrentList->GetNumEntries())
		return NULL;

	if (CurrentList->IsDirectory(CursorPos))
		return NULL;
	else return CurrentList->GetSongEntry(CursorPos);
}

void SongWheel::Update(float Delta)
{
	Time += Delta;
	SelCursor->Alpha = (sin(Time*6)+1)/4 + 0.5;

	uint32 Size = CurrentList->GetNumEntries();

	if (PendingVerticalDisplacement)
	{
		float ListDelta = PendingVerticalDisplacement * Delta * DisplacementSpeed;
		float NewListY = CurrentVerticalDisplacement + ListDelta;
		float NewLowerBound = NewListY + Size * ItemHeight;
		float LowerBound = Size * ItemHeight;
		
		if (!IntervalsIntersect(0, ScreenHeight, NewListY, NewLowerBound))
		{
			CurrentVerticalDisplacement = 0;
		}else
		{
			CurrentVerticalDisplacement = NewListY;
			PendingVerticalDisplacement -= ListDelta;
		}
	}

	Vec2 mpos = GameState::GetInstance().GetWindow()->GetRelativeMPos();

	if (mpos.y > CurrentVerticalDisplacement && mpos.x > Transform(mpos.y))
	{
		float posy = mpos.y;
		posy -= CurrentVerticalDisplacement;
		posy -= (int)posy % (int)ItemHeight;
		posy = (posy / ItemHeight);
		CursorPos = posy;
	}

	// Update the cursor.
	if (CursorPos < 0)
		CursorPos = Size-1;
	else if (CursorPos >= Size)
		CursorPos = 0;

	// Hey we've got a new cursor position, update.
	if (OldCursorPos != CursorPos)
	{
		OldCursorPos = CursorPos;
		DifficultyIndex = 0;
		if (OnSongChange)
		{
			if (CursorPos < Size)
			{
				Game::Song* Notify = GetSelectedSong();

				OnSongChange(Notify, DifficultyIndex);
			}
		}
	}

	// Set its position.
	float Y = CursorPos * SelCursor->GetHeight() + CurrentVerticalDisplacement;
	float sinTSquare = sin(Time*2);

	sinTSquare *= sinTSquare;

	float X = Transform(Y) - SelCursor->GetWidth() -  sinTSquare * 10;
	SelCursor->SetPosition(X, Y);
}

void SongWheel::DisplayItem(String Text, Vec2 Position)
{
	if (Position.y > -ItemHeight && Position.y < ScreenHeight)
		mFont->DisplayText(Text.c_str(), Position);
}

void SongWheel::Render()
{
	int Cur = 0;
	int Max = CurrentList->GetNumEntries();

	Game::Song *ToDisplay;

	for (Cur = 0; Cur < Max; Cur++)
	{
		float yTransform = Cur*ItemHeight + CurrentVerticalDisplacement;
		float xTransform = Transform(yTransform);
		Vec2 Position = Vec2(xTransform, yTransform);

		if (!CurrentList->IsDirectory(Cur))
		{
			ToDisplay = CurrentList->GetSongEntry(Cur);
			DisplayItem(ToDisplay->SongName, Position);
		}else
		{
			DisplayItem(CurrentList->GetEntryTitle(Cur), Position);
		}
	}

	if (!CurrentList->GetNumEntries())
		return;

	if (CurrentList->IsDirectory(CursorPos))
	{
	}else
	{
		Vec2 InfoPosition = Vec2(ScreenWidth/6, 120);
		if (CurrentList->GetSongEntry(CursorPos)->Mode == MODE_DOTCUR)
		{
			char infoStream[1024];

			dotcur::Song* Entry = static_cast<dotcur::Song*> (CurrentList->GetSongEntry(CursorPos));
			if (Entry->Difficulties.size())
			{
				int Min = Entry->Difficulties[0]->Duration / 60;
				int Sec = (int)Entry->Difficulties[0]->Duration % 60;

				sprintf(infoStream, "song author: %s\n"
					"difficulties: %d\n"
					"duration: %d:%02d\n",
					Entry->SongAuthor.c_str(),
					Entry->Difficulties.size(),
					Min, Sec);

				mFont->DisplayText(infoStream, Vec2(ScreenWidth/6, 120));
			}else mFont->DisplayText("unavailable (edit only)", InfoPosition);


		}else if (CurrentList->GetSongEntry(CursorPos)->Mode == MODE_7K)
		{

			char infoStream[1024];

			VSRG::Song* Entry = static_cast<VSRG::Song*>(CurrentList->GetSongEntry(CursorPos));

			if (DifficultyIndex < Entry->Difficulties.size())
			{
				int Min = Entry->Difficulties[DifficultyIndex]->Duration / 60;
				int Sec = (int)Entry->Difficulties[DifficultyIndex]->Duration % 60;
				float nps = Entry->Difficulties[DifficultyIndex]->TotalObjects / Entry->Difficulties[DifficultyIndex]->Duration;

				sprintf(infoStream, "song author: %s\n"
					"difficulties: %d\n"
					"duration: %d:%02d\n"
					"difficulty: %s (%d keys)\n"
					"avg. nps: %f\n",
					Entry->SongAuthor.c_str(),
					Entry->Difficulties.size(),
					Min, Sec,
					Entry->Difficulties[DifficultyIndex]->Name.c_str(), Entry->Difficulties[DifficultyIndex]->Channels,
					nps);

				mFont->DisplayText(infoStream, InfoPosition);
			}
		}

	}
	SelCursor->Render();

}
