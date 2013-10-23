#include "Global.h"
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "FileManager.h"
#include "ScreenGameplay.h"
#include "ScreenEdit.h"
#include "GameWindow.h"
#include "ImageLoader.h"
#include "Audio.h"
#include <sstream>

#define SONGLIST_BASEY 120
#define SONGLIST_BASEX ScreenWidth*3/4

SoundSample *SelectSnd = NULL, *ClickSnd=NULL;
VorbisStream	*Loops[6];

ScreenSelectMusic::ScreenSelectMusic()
{
	Running = true;
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

	OldCursor = Cursor = 0;
}

void ScreenSelectMusic::Init()
{
	FileManager::GetSongList(SongList);

	SwitchBackGuiPending = true;
	// screen music :p
	// startVorbisStream("GameData/Skins/default/loop4.ogg");

	if (!Font)
	{
		Font = new BitmapFont();
		Font->LoadSkinFontImage("font_screenevaluation.tga", glm::vec2(10, 20), glm::vec2(32, 32), glm::vec2(10,20), 32);
	}
	SelCursor.SetImage(ImageLoader::LoadSkin("songselect_cursor.png"));
	SelCursor.SetSize(20);
	SelCursor.SetPosition(SONGLIST_BASEX-SelCursor.GetWidth(), SONGLIST_BASEY);
	Background.SetImage(ImageLoader::LoadSkin("ScreenEvaluationBackground.png"));
	Logo.SetImage(ImageLoader::LoadSkin("logo.png"));
	Logo.SetSize(480);
	Logo.Centered = true;
	Logo.SetPosition(Logo.GetWidth()/4, ScreenHeight - Logo.GetHeight()/4);
	Time = 0;
}

void ScreenSelectMusic::Cleanup()
{
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

	std::stringstream ss;
	ss << "song author: " << SongList.at(Cursor)->SongAuthor<< "\n"
	   << "difficulties:" << SongList.at(Cursor)->Difficulties.size();

	Font->DisplayText(ss.str().c_str(), glm::vec2(ScreenWidth/6, 120));
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
		switch (BindingsManager::TranslateKey(key))
		{
		case KT_GoToEditMode: // Edit mode!
			_Next = new ScreenEdit(this);
			_Next->Init(SongList.at(Cursor));
			Next = _Next;
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
				_gNext->Init(SongList.at(Cursor), 0);

				Next = _gNext;
				SwitchBackGuiPending = true;
				WindowFrame.isGuiInputEnabled = false;
				StopLoops();
			}
				break;
		}
	}
}
