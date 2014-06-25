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
	if (IsInitialized)
		return;

	RangeStart = Start;
	RangeEnd = End;

	dotcurModeActive = IsDotcurActive;
	VSRGModeActive = IsVSRGActive;

	OnSongChange = FuncNotify;
	OnSongSelect = FuncNotifySelect;
	Transform = FuncTransform;

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
	for (std::vector<dotcur::Song*>::iterator i = SongList.begin(); 
		i != SongList.end();
		i++)
	{
		delete *i;
	}

	for (std::vector<VSRG::Song*>::iterator i = SongList7K.begin(); 
		i != SongList7K.end();
		i++)
	{
		delete *i;
	}

	SongList.clear();
	SongList7K.clear();

	if (dotcurModeActive)
	{
		GameState::Printf("Getting dotCur song list...\n");;
		FileManager::GetSongList(SongList);
	}

	if (VSRGModeActive)
	{
		GameState::Printf("Getting VSRG song list...\n");
		FileManager::GetSongList7K(SongList7K);
	}
	
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
			DifficultyIndex--;
			max_index = 0;
			if (CurrentMode == MODE_7K)
			{
				if (CursorPos < SongList7K.size())
					max_index = SongList7K.at(CursorPos)->Difficulties.size()-1;
			}
			else
			{
				if (CursorPos < SongList.size())
					max_index = SongList.at(CursorPos)->Difficulties.size()-1;
			}

			DifficultyIndex = min(max_index, DifficultyIndex);
			return true;
		case KT_Right:
			DifficultyIndex++;
			if (CurrentMode == MODE_7K)
			{
				if (CursorPos < SongList7K.size())
					if (DifficultyIndex >= SongList7K.at(CursorPos)->Difficulties.size())
						DifficultyIndex = 0;
			}else
			{
				if (CursorPos < SongList.size())
					if (DifficultyIndex >= SongList.at(CursorPos)->Difficulties.size())
						DifficultyIndex = 0;
			}
			return true;
		case KT_Select:
			Vec2 mpos = GameState::GetWindow()->GetRelativeMPos();
			if (!isMouseInput || mpos.x > Transform(mpos.y) && mpos.y > CurrentVerticalDisplacement)
			{
				if (CurrentMode == MODE_DOTCUR && SongList.size())
				{
					if (DifficultyIndex < SongList.at(CursorPos)->Difficulties.size())
						OnSongSelect(SongList.at(CursorPos), DifficultyIndex);
					
				}else if (SongList7K.size())
				{
					if (DifficultyIndex < SongList7K.at(CursorPos)->Difficulties.size())
						OnSongSelect(SongList7K.at(CursorPos), DifficultyIndex);
				}
			}
			return true;
		}

		switch (key)
		{
		case 'Q':
			PendingVerticalDisplacement += 320;
			return true;
		case 'W':
			PendingVerticalDisplacement -= 320;
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
	if (CurrentMode == MODE_DOTCUR)
		return SongList.at(CursorPos);
	else
		return SongList7K.at(CursorPos);
}

void SongWheel::ChangeMode (const ModeType NewMode)
{
	CurrentMode = NewMode;
}

void SongWheel::Update(float Delta)
{
	Time += Delta;
	SelCursor->Alpha = (sin(Time*6)+1)/4 + 0.5;

	uint32 Size;

	if (CurrentMode == MODE_DOTCUR)
		Size = SongList.size();
	else
		Size = SongList7K.size();

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
	int Max = 0;

	if (CurrentMode == MODE_DOTCUR)
		Max = SongList.size();
	else
		Max = SongList7K.size();

	Game::Song *ToDisplay;

	for (Cur = 0; Cur < Max; Cur++)
	{
		if (CurrentMode == MODE_DOTCUR)
			ToDisplay = SongList.at(Cur);
		else
			ToDisplay = SongList7K.at(Cur);

		float yTransform = Cur*ItemHeight + CurrentVerticalDisplacement;
		float xTransform = Transform(yTransform);

		DisplayItem(ToDisplay->SongName, Vec2(xTransform, yTransform));
	}

	if (SongList.size() && CurrentMode == MODE_DOTCUR)
	{
		char infoStream[1024];

		if (SongList.at(CursorPos)->Difficulties.size())
		{
			int Min = SongList.at(CursorPos)->Difficulties[0]->Duration / 60;
			int Sec = (int)SongList.at(CursorPos)->Difficulties[0]->Duration % 60;

			sprintf(infoStream, "song author: %s\n"
						  	    "difficulties: %d\n"
					            "duration: %d:%02d\n",
								SongList.at(CursorPos)->SongAuthor.c_str(),
								SongList.at(CursorPos)->Difficulties.size(),
								Min, Sec
								);

			mFont->DisplayText(infoStream, Vec2(ScreenWidth/6, 120));
		}else mFont->DisplayText("unavailable (edit only)", Vec2(ScreenWidth/6, 120));

		
	}else if (SongList7K.size() && CurrentMode == MODE_7K)
	{
		
		char infoStream[1024];

		if (CursorPos < SongList7K.size() && DifficultyIndex < SongList7K.at(CursorPos)->Difficulties.size())
		{
			int Min = SongList7K.at(CursorPos)->Difficulties[DifficultyIndex]->Duration / 60;
			int Sec = (int)SongList7K.at(CursorPos)->Difficulties[DifficultyIndex]->Duration % 60;
			float nps = SongList7K.at(CursorPos)->Difficulties[DifficultyIndex]->TotalObjects / SongList7K.at(CursorPos)->Difficulties[DifficultyIndex]->Duration;

			sprintf(infoStream, "song author: %s\n"
						  	    "difficulties: %d\n"
					            "duration: %d:%02d\n"
								"difficulty: %s (%d keys)\n"
								"avg. nps: %f\n",
								SongList7K.at(CursorPos)->SongAuthor.c_str(),
								SongList7K.at(CursorPos)->Difficulties.size(),
								Min, Sec,
								SongList7K.at(CursorPos)->Difficulties[DifficultyIndex]->Name.c_str(), SongList7K.at(CursorPos)->Difficulties[DifficultyIndex]->Channels,
								nps
								);

			mFont->DisplayText(infoStream, Vec2(ScreenWidth/6, 120));
		}
	}

	SelCursor->Render();
}
