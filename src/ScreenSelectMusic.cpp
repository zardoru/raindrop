#include "Global.h"
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "FileManager.h"
#include "ScreenGameplay.h"
#include "ScreenEdit.h"
#include "GraphicsManager.h"
#include "ImageLoader.h"
#include "Audio.h"
#include <GL/GL.h>

SoundSample *SelectSnd = NULL;
VorbisStream	*Loops[6];

ScreenSelectMusic::ScreenSelectMusic()
{
	Running = true;
	Font = NULL;
	if (!SelectSnd)
	{
		SelectSnd = new SoundSample((FileManager::GetSkinPrefix() + "select.ogg").c_str()/*(FileManager::GetSkinPrefix() + "select.ogg").c_str()
																			  */);
		MixerAddSample(SelectSnd);

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

	Cursor = 0;
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
	SelCursor.SetPosition(ScreenWidth/2-SelCursor.GetWidth(), 120);
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
#ifndef DISABLE_CEGUI
	else
	{
		if (SwitchBackGuiPending)
		{
			GraphMan.isGuiInputEnabled = true;
			SwitchBackGuiPending = false;
			int rn = rand() % 6;
			Loops[rn]->seek(0);
			Loops[rn]->Start();
		}
	}
#endif

	Time += Delta;
	Logo.AddRotation(12 * Delta);

	SelCursor.Alpha = (sin(Time*6)+1)/4 + 0.5;

	Background.Render();

	int Cur = 0;
	for (std::vector<Song*>::iterator i = SongList.begin(); i != SongList.end(); i++)
	{
		Font->DisplayText((*i)->SongName.c_str(), glm::vec2(ScreenWidth/2, Cur*20 + 120));
		Cur++;
	}

	Font->DisplayText("song select", glm::vec2(ScreenWidth/2-55, 0));
	Font->DisplayText("press space to confirm", glm::vec2(ScreenWidth/2-110, 20));
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

void ScreenSelectMusic::HandleInput(int32 key, int32 code, bool isMouseInput)
{
	if (Next)
	{
		Next->HandleInput(key, code, isMouseInput);
		return;
	}

	if (code == GLFW_PRESS)
	{
		if (key == GLFW_KEY_F4) // Edit mode!
		{
			ScreenEdit *_Next = new ScreenEdit(this);
			_Next->Init(SongList.at(Cursor));
			Next = _Next;
			SwitchBackGuiPending = true;
			StopLoops();
		}

		if (key == GLFW_KEY_UP)
		{
			Cursor--;
			if (Cursor < 0)
			{
				Cursor = SongList.size()-1;
			}
			SelCursor.SetPosition(ScreenWidth/2-SelCursor.GetWidth(), Cursor * SelCursor.GetHeight() + 120);
		}else if (key == GLFW_KEY_DOWN)
		{
			Cursor++;
			if (Cursor >= SongList.size())
				Cursor = 0;
			SelCursor.SetPosition(ScreenWidth/2-SelCursor.GetWidth(), Cursor * SelCursor.GetHeight() + 120);
		}else if (key == GLFW_KEY_SPACE)
		{
				SelectSnd->Reset();
				ScreenGameplay *_Next = new ScreenGameplay(this);
				_Next->Init(SongList.at(Cursor), 0);

				Next = _Next;
				SwitchBackGuiPending = true;
				GraphMan.isGuiInputEnabled = false;
				StopLoops();
		}
	}
}
