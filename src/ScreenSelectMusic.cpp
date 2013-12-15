#include <sstream>
#include <iomanip>

#include "Global.h"
#include "Screen.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include "FileManager.h"

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
VorbisStream	*Loops[6];

ScreenSelectMusic::ScreenSelectMusic()
{
	Font = NULL;
	if (!SelectSnd)
	{
		SelectSnd = new SoundSample((FileManager::GetSkinPrefix() + "select.ogg").c_str());
		MixerAddSample(SelectSnd);

		ClickSnd = new SoundSample((FileManager::GetSkinPrefix() + "click.ogg").c_str());
		MixerAddSample(ClickSnd);

		for (int i = 0; i < 6; i++)
		{
			std::stringstream str;
			str << FileManager::GetSkinPrefix() << "loop" << i+1 << ".ogg";
			Loops[i] = new VorbisStream(str.str().c_str());
			Loops[i]->Stop();
			Loops[i]->setLoop(true);
			MixerAddStream(Loops[i]);
		}
	}
	SelectedMode = MODE_DOTCUR;
}

void ScreenSelectMusic::MainThreadInitialization()
{
	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", glm::vec2(10, 20), glm::vec2(32, 32), glm::vec2(10,20), 32);
	}

	Font->SetAffectedByLightning(true);
	SelCursor.SetImage(ImageLoader::LoadSkin("songselect_cursor.png"));
	SelCursor.SetSize(20);
	SelCursor.SetPosition(SONGLIST_BASEX-SelCursor.GetWidth(), SONGLIST_BASEY);
	Background.SetImage(ImageLoader::LoadSkin("MenuBackground.png"));
	Logo.SetImage(ImageLoader::LoadSkin("logo.png"));
	Logo.SetSize(480);
	Logo.Centered = true;
	Logo.SetPosition(Logo.GetWidth()/4, ScreenHeight - Logo.GetHeight()/4);
	Logo.AffectedByLightning = true;
	WindowFrame.SetLightMultiplier(1);
	Background.AffectedByLightning = true;
}

void ScreenSelectMusic::LoadThreadInitialization()
{
	FileManager::GetSongList(SongList);
	FileManager::GetSongList7K(SongList7K);

	Running = true;
	OldCursor = Cursor = 0;
	SwitchBackGuiPending = true;

	char* Manifest[] =
	{
		"songselect_cursor.png",
		"MenuBackground.png",
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
			int rn = rand() % 6;
			Loops[rn]->seek(0);
			Loops[rn]->Start();
		}
	}

	Time += Delta;
	Logo.AddRotation(12 * Delta);

	SelCursor.Alpha = (sin(Time*6)+1)/4 + 0.5;
	WindowFrame.SetLightMultiplier(sin(Time) * 0.2 + 1);

	Background.Render();

		glm::vec2 mpos = WindowFrame.GetRelativeMPos();

	if (mpos.x > SONGLIST_BASEX)
	{
		float posy = mpos.y;
		posy -= 120;
		posy -= (int)posy % 20;
		posy = (posy / 20);
		Cursor = posy;
		UpdateCursor();
	}


	int Cur = 0;

	if (SelectedMode == MODE_DOTCUR)
	{
		for (std::vector<SongDC*>::iterator i = SongList.begin(); i != SongList.end(); i++)
		{
			Font->DisplayText((*i)->SongName.c_str(), glm::vec2(SONGLIST_BASEX, Cur*20 + SONGLIST_BASEY));
			Cur++;
		}
	}else if (SelectedMode == MODE_7K)
	{
		for (std::vector<Song7K*>::iterator i = SongList7K.begin(); i != SongList7K.end(); i++)
		{
			Font->DisplayText((*i)->SongName.c_str(), glm::vec2(SONGLIST_BASEX, Cur*20 + SONGLIST_BASEY));
			Cur++;
		}
	}

	Font->DisplayText("song select", glm::vec2(ScreenWidth/2-55, 0));
	Font->DisplayText("press space to confirm", glm::vec2(ScreenWidth/2-110, 20));

	String modeString;

	if (SelectedMode == MODE_DOTCUR)
		modeString = "mode: dotcur";
	else
		modeString = "mode: 7K";

	Font->DisplayText(modeString.c_str(), glm::vec2(ScreenWidth/2-modeString.length() * 5, 40));

	/* TODO: Reduce these to functions or something */
	if (SongList.size() && SelectedMode == MODE_DOTCUR)
	{
		std::stringstream ss;
		ss << "song author: " << SongList.at(Cursor)->SongAuthor<< "\n"
			<< "difficulties: " << SongList.at(Cursor)->Difficulties.size() << "\n";
		if (SongList.at(Cursor)->Difficulties.size())
		{
			int Min = SongList.at(Cursor)->Difficulties[0]->Duration / 60;
			int Sec = (int)SongList.at(Cursor)->Difficulties[0]->Duration % 60;
			ss	<< "duration: " << Min << ":" << std::setw(2) << std::setfill('0') << Sec;
		}

		Font->DisplayText(ss.str().c_str(), glm::vec2(ScreenWidth/6, 120));
	}else if (SongList7K.size() && SelectedMode == MODE_7K)
	{
		std::stringstream ss;
		ss << "song author: " << SongList7K.at(Cursor)->SongAuthor<< "\n"
			<< "difficulties: " << SongList7K.at(Cursor)->Difficulties.size() << "\n";
		if (SongList.at(Cursor)->Difficulties.size())
		{
			int Min = SongList7K.at(Cursor)->Difficulties[0]->Duration / 60;
			int Sec = (int)SongList7K.at(Cursor)->Difficulties[0]->Duration % 60;
			ss	<< "duration: " << Min << ":" << std::setw(2) << std::setfill('0') << Sec;
		}

		Font->DisplayText(ss.str().c_str(), glm::vec2(ScreenWidth/6, 120));
	}


	SelCursor.Render();
	Logo.Render();
	return Running;
}	

void ScreenSelectMusic::StopLoops()
{
	for (int i = 0; i < 6; i++)
	{
		Loops[i]->seek(0);
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
		ClickSnd->Reset();
	}

	SelCursor.SetPosition(SONGLIST_BASEX - SelCursor.GetWidth(), Cursor * SelCursor.GetHeight() + SONGLIST_BASEY);
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

		ScreenEdit *_eNext;
		ScreenLoading *_LNext;

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
		case KT_Select:
			if (!isMouseInput || (WindowFrame.GetRelativeMPos().x > SONGLIST_BASEX && WindowFrame.GetRelativeMPos().y > SONGLIST_BASEY))
			{
				SelectSnd->Reset();

				if (SelectedMode == MODE_DOTCUR)
				{
					if (/* difficulty index < */ SongList.at(Cursor)->Difficulties.size())
					{
						_gNext = new ScreenGameplay(this);
						_LNext = new ScreenLoading(this, _gNext);


						_gNext->Init(SongList.at(Cursor), 0);

						_LNext->Init();
					}else
						return;
				}else
				{
					// TODO: finish 7k mode gameplay screen
					_g7Next = new ScreenGameplay7K();
					_LNext = new ScreenLoading(this, _g7Next);

					_g7Next->Init(SongList7K.at(Cursor), 0);

					_LNext->Init();
				}

				Next = _LNext;
				SwitchBackGuiPending = true;
				WindowFrame.isGuiInputEnabled = false;
				StopLoops();
			}
				break;
		case KT_Escape:
			Running = false;
			break;
		case KT_BSPC:
			if (SelectedMode == MODE_DOTCUR)
				SelectedMode = MODE_7K;
			else
				SelectedMode = MODE_DOTCUR;
			Cursor = 0;
		}
	}
}
