#include <sstream>
#include <iomanip>

#include "Global.h"
#include "Screen.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"
#include "Configuration.h"

#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "ScreenLoading.h"

#include "ScreenGameplay.h"
#include "ScreenGameplay7K.h"
#include "ScreenEdit.h"

#define SONGLIST_BASEY 120
#define SONGLIST_BASEX ScreenWidth*3/4

SoundSample *SelectSnd = NULL, *ClickSnd=NULL;
AudioStream	**Loops;
int LoopTotal;

ScreenSelectMusic::ScreenSelectMusic()
{
	Font = NULL;
	if (!SelectSnd)
	{
		LoopTotal = 0;
		SelectSnd = new SoundSample();
		SelectSnd->Open((FileManager::GetSkinPrefix() + "select.ogg").c_str());
		MixerAddSample(SelectSnd);

		ClickSnd = new SoundSample();
		ClickSnd->Open((FileManager::GetSkinPrefix() + "click.ogg").c_str());
		MixerAddSample(ClickSnd);
		
		LoopTotal = Configuration::GetSkinConfigf("LoopTotal");

		Loops = new AudioStream*[LoopTotal];
		for (int i = 0; i < LoopTotal; i++)
		{
			std::stringstream str;
			str << FileManager::GetSkinPrefix() << "loop" << i+1 << ".ogg";
			Loops[i] = new AudioStream();
			Loops[i]->Open(str.str().c_str());
			Loops[i]->Stop();
			Loops[i]->SetLoop(true);
			MixerAddStream(Loops[i]);
		}
	}

	SelectedMode = MODE_DOTCUR;

	OptionUpscroll = false;

	ListY = SONGLIST_BASEY;

	diff_index = 0;
}

void ScreenSelectMusic::MainThreadInitialization()
{
	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", Vec2(10, 20), Vec2(32, 32), Vec2(10,20), 32);
	}

	Font->SetAffectedByLightning(true);
	SelCursor.SetImage(ImageLoader::LoadSkin("songselect_cursor.png"));
	SelCursor.SetSize(20);
	SelCursor.SetPosition(SONGLIST_BASEX-SelCursor.GetWidth(), SONGLIST_BASEY);
	Background.SetImage(ImageLoader::LoadSkin(Configuration::GetSkinConfigs("SelectMusicBackground")));
	
	/* Logo */
	Logo.SetImage(ImageLoader::LoadSkin("logo.png"));
	Logo.SetSize(Configuration::GetSkinConfigf("Size", "Logo2"));
	Logo.Centered = Configuration::GetSkinConfigf("Centered", "Logo2") != 0;
	Logo.SetPosition(Configuration::GetSkinConfigf("X", "Logo2"), Configuration::GetSkinConfigf("Y", "Logo2"));
	Logo.AffectedByLightning = true;

	WindowFrame.SetLightMultiplier(1);
	Background.AffectedByLightning = true;
	PendingListY = 1;
}

void ScreenSelectMusic::LoadThreadInitialization()
{
#ifndef NDEBUG
	std::cout << "Getting dotCur song list..." << std::endl;
#endif
	FileManager::GetSongList(SongList);
#ifndef NDEBUG
	std::cout << "Getting dotCur keyboard song list..." << std::endl;
#endif
	FileManager::GetSongList7K(SongList7K);

#ifndef NDEBUG
	std::cout << "Done loading." << std::endl;
#endif
	
	Running = true;
	OldCursor = Cursor = 0;
	SwitchBackGuiPending = true;
	char* Manifest[] =
	{
		"songselect_cursor.png",
		(char*)Configuration::GetSkinConfigs("SelectMusicBackground").c_str(),
		"logo.png"
	};

	ImageLoader::LoadFromManifest(Manifest, 3, FileManager::GetSkinPrefix());

	Time = 0;
}

void ScreenSelectMusic::Cleanup()
{
	StopLoops();
}

bool ScreenSelectMusic::Run(double Delta)
{
	if (RunNested(Delta))
		return true;
	else
	{
		if (SwitchBackGuiPending)
		{
			WindowFrame.isGuiInputEnabled = true;
			SwitchBackGuiPending = false;
			if (LoopTotal)
			{
				int rn = rand() % LoopTotal;
				Loops[rn]->SeekTime(0);
				Loops[rn]->Play();
			}
		}
	}

	Time += Delta;
	Logo.AddRotation(12 * Delta);

	SelCursor.Alpha = (sin(Time*6)+1)/4 + 0.5;
	WindowFrame.SetLightMultiplier(sin(Time) * 0.2 + 1);

	Background.Render();

	if (PendingListY)
	{
		uint32 Size;

		if (SelectedMode == MODE_DOTCUR)
			Size = SongList.size();
		else
			Size = SongList7K.size();

		float ListDelta = PendingListY * Delta * 2;
		float NewListY = ListY + ListDelta;
		float NewLowerBound = NewListY + Size * 20;
		float LowerBound = Size * 20;
		
		if (!IntervalsIntersect(0, ScreenHeight, NewListY, NewLowerBound))
		{
			PendingListY = 0;
		}else
		{
			ListY = NewListY;
			PendingListY -= PendingListY * Delta * 2;
		}
	}

	Vec2 mpos = WindowFrame.GetRelativeMPos();

	if (mpos.y > ListY && mpos.x > SONGLIST_BASEX)
	{
		float posy = mpos.y;
		posy -= ListY;
		posy -= (int)posy % 20;
		posy = (posy / 20);
		Cursor = posy;
	}

	UpdateCursor();

	int Cur = 0;

	if (SelectedMode == MODE_DOTCUR)
	{
		for (std::vector<SongDC*>::iterator i = SongList.begin(); i != SongList.end(); i++)
		{
			Font->DisplayText((*i)->SongName.c_str(), Vec2(SONGLIST_BASEX, Cur*20 + ListY));
			Cur++;
		}
	}else if (SelectedMode == MODE_7K)
	{
		for (std::vector<Song7K*>::iterator i = SongList7K.begin(); i != SongList7K.end(); i++)
		{
			std::string text = (*i)->SongName;
			Font->DisplayText(text.c_str(), Vec2(SONGLIST_BASEX, Cur*20 + ListY));
			Cur++;
		}
	}

	Font->DisplayText("song select", Vec2(ScreenWidth/2-55, 0));
	Font->DisplayText("press space to confirm", Vec2(ScreenWidth/2-110, 20));

	String modeString;

	if (SelectedMode == MODE_DOTCUR)
		modeString = "mode: dotcur";
	else
	{
		if (OptionUpscroll)
			modeString = "mode: 7K (upscroll)";
		else
			modeString = "mode: 7K (downscroll)";
	}

	Font->DisplayText(modeString.c_str(), Vec2(ScreenWidth/2-modeString.length() * 5, 40));

	/* TODO: Reduce these to functions or something */
	if (SongList.size() && SelectedMode == MODE_DOTCUR)
	{
		char infoStream[1024];

		if (SongList.at(Cursor)->Difficulties.size())
		{
			int Min = SongList.at(Cursor)->Difficulties[0]->Duration / 60;
			int Sec = (int)SongList.at(Cursor)->Difficulties[0]->Duration % 60;

			sprintf(infoStream, "song author: %s\n"
						  	    "difficulties: %d\n"
					            "duration: %d:%02d\n",
								SongList.at(Cursor)->SongAuthor.c_str(),
								SongList.at(Cursor)->Difficulties.size(),
								Min, Sec
								);

			Font->DisplayText(infoStream, Vec2(ScreenWidth/6, 120));
		}else Font->DisplayText("unavailable (edit only)", Vec2(ScreenWidth/6, 120));

		
	}else if (SongList7K.size() && SelectedMode == MODE_7K)
	{
		
		char infoStream[1024];

		if (diff_index < SongList7K.at(Cursor)->Difficulties.size())
		{
			int Min = SongList7K.at(Cursor)->Difficulties[diff_index]->Duration / 60;
			int Sec = (int)SongList7K.at(Cursor)->Difficulties[diff_index]->Duration % 60;

			sprintf(infoStream, "song author: %s\n"
						  	    "difficulties: %d\n"
					            "duration: %d:%02d\n"
								"difficulty: %s\n",
								SongList7K.at(Cursor)->SongAuthor.c_str(),
								SongList7K.at(Cursor)->Difficulties.size(),
								Min, Sec,
								SongList7K.at(Cursor)->Difficulties[diff_index]->Name.c_str()
								);

			Font->DisplayText(infoStream, Vec2(ScreenWidth/6, 120));
		}
	}


	SelCursor.Render();
	Logo.Render();
	return Running;
}	

void ScreenSelectMusic::StopLoops()
{
	for (int i = 0; i < LoopTotal; i++)
	{
		Loops[i]->SeekTime(0);
		Loops[i]->Stop();
	}
}

void ScreenSelectMusic::UpdateCursor()
{
	int Size;

	if (SelectedMode == MODE_DOTCUR)
		Size = SongList.size();
	else
		Size = SongList7K.size();

	if (Cursor < 0)
	{
		Cursor = Size-1;
	}

	if (Cursor >= Size)
		Cursor = 0;

	if (OldCursor != Cursor)
	{
		OldCursor = Cursor;
		diff_index = 0;
		ClickSnd->Play();
	}

	SelCursor.SetPosition(SONGLIST_BASEX - SelCursor.GetWidth() - sin(Time*2) * sin(Time*2) * 10, Cursor * SelCursor.GetHeight() + ListY);
}

void ScreenSelectMusic::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (Next)
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	if (code == KE_Press)
	{
		ScreenGameplay *_gNext = NULL;
		ScreenGameplay7K *_g7Next = NULL;

		ScreenEdit *_eNext = NULL;
		ScreenLoading *_LNext = NULL;

		switch (BindingsManager::TranslateKey(key))
		{
		case KT_GoToEditMode: // Edit mode!
			
			_eNext = new ScreenEdit(this);
			_LNext = new ScreenLoading(this, _eNext);
			_eNext->Init(SongList.at(Cursor));
			_LNext->Init();

			Next = _LNext;
			SwitchBackGuiPending = true;
			StopLoops();
			break;
		case KT_Up:
			Cursor--;
			UpdateCursor();
			break;
		case KT_Down:
			Cursor++;
			UpdateCursor();
			break;
		case KT_Left:
			diff_index--;
			if (diff_index < 0) diff_index = 0;
			break;
		case KT_Right:
			diff_index++;
			if (SelectedMode == MODE_7K)
			{
				if (diff_index >= SongList7K.at(Cursor)->Difficulties.size())
					diff_index = 0;
			}else
			{
				if (diff_index >= SongList.at(Cursor)->Difficulties.size())
					diff_index = 0;
			}
			break;
		case KT_Select:
			if (!isMouseInput || (WindowFrame.GetRelativeMPos().x > SONGLIST_BASEX && WindowFrame.GetRelativeMPos().y > ListY))
			{
				SelectSnd->Play();

				if (SelectedMode == MODE_DOTCUR && SongList.size())
				{
					if (diff_index < SongList.at(Cursor)->Difficulties.size())
					{
						_gNext = new ScreenGameplay(this);
						_LNext = new ScreenLoading(this, _gNext);


						_gNext->Init(SongList.at(Cursor), diff_index);

						_LNext->Init();
					}else
						return;
				}else if (SongList7K.size())
				{
					if (diff_index < SongList7K.at(Cursor)->Difficulties.size())
					{
						_g7Next = new ScreenGameplay7K();
						_LNext = new ScreenLoading(this, _g7Next);

						_g7Next->Init(SongList7K.at(Cursor), diff_index, OptionUpscroll);

						_LNext->Init();
					}else
						return;
				}

				if (_LNext)
				{
					Next = _LNext;
					SwitchBackGuiPending = true;
					WindowFrame.isGuiInputEnabled = false;
					StopLoops();
				}
			}
				break;
		case KT_Escape:
			Running = false;
			break;
		case KT_FractionDec:
			OptionUpscroll = !OptionUpscroll;
			break;
		case KT_BSPC:
			if (SelectedMode == MODE_DOTCUR)
				SelectedMode = MODE_7K;
			else
				SelectedMode = MODE_DOTCUR;
			Cursor = 0;
		}

		switch (key)
		{
		case 'Q':
			PendingListY += 320;
			break;
		case 'W':
			PendingListY -= 320;
			break;
		}
	}
}

void ScreenSelectMusic::HandleScrollInput(double xOff, double yOff)
{
	PendingListY += yOff * 80;
}
