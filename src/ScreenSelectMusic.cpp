#include "Global.h"
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "ScreenLoading.h"
#include "FileManager.h"
#include "ScreenGameplay.h"
#include "ScreenEdit.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include <sstream>
#include <iomanip>

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
	for (std::vector<Song*>::iterator i = SongList.begin(); i != SongList.end(); i++)
	{
		Font->DisplayText((*i)->SongName.c_str(), glm::vec2(SONGLIST_BASEX, Cur*20 + SONGLIST_BASEY));
		Cur++;
	}

	Font->DisplayText("song select", glm::vec2(ScreenWidth/2-55, 0));
	Font->DisplayText("press space to confirm", glm::vec2(ScreenWidth/2-110, 20));


	if (SongList.size())
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
	if (Cursor < 0)
	{
		Cursor = SongList.size()-1;
	}

	if (Cursor >= SongList.size())
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
		ScreenEdit *_Next;
		ScreenLoading *_LNext;
		switch (BindingsManager::TranslateKey(key))
		{
		case KT_GoToEditMode: // Edit mode!
			
			_Next = new ScreenEdit(this);
			_LNext = new ScreenLoading(this, _Next);
			_Next->Init(SongList.at(Cursor));
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
				_gNext = new ScreenGameplay(this);
				_LNext = new ScreenLoading(this, _gNext);
				_gNext->Init(SongList.at(Cursor), 0);
				_LNext->Init();

				Next = _LNext;
				SwitchBackGuiPending = true;
				WindowFrame.isGuiInputEnabled = false;
				StopLoops();
			}
				break;
		case KT_Escape:
			Running = false;
		}
	}
}
