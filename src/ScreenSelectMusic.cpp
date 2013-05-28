#include "Global.h"
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "FileManager.h"
#include "ScreenGameplay.h"
#include "ScreenEdit.h"
#include "GraphicsManager.h"
#include "Audio.h"

SoundSample *SelectSnd = NULL;
VorbisStream	*Loops[6];

ScreenSelectMusic::ScreenSelectMusic()
{
	Running = true;
	if (!SelectSnd)
	{
		SelectSnd = new SoundSample((FileManager::GetSkinPrefix() + "select.ogg").c_str());
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
}

void ScreenSelectMusic::Init()
{
	FileManager::GetSongList(SongList);

	SwitchBackGuiPending = true;
	// screen music :p
	// startVorbisStream("GameData/Skins/default/loop4.ogg");

#ifndef DISABLE_CEGUI

	using namespace CEGUI;
	
	WindowManager& winMgr = WindowManager::getSingleton();
	FrameWindow* fWnd = static_cast<FrameWindow*>(winMgr.createWindow( "TaharezLook/FrameWindow", "screenWindow" ));

	root = (DefaultWindow*)winMgr.createWindow("DefaultWindow", "Root");

	fWnd->setText("Select your song.");
	fWnd->setPosition(UVector2(cegui_reldim(0.1f), cegui_reldim( 0.1f)));
    fWnd->setSize(UVector2(cegui_reldim(0.8f), cegui_reldim( 0.8f)));
	root->addChildWindow(fWnd);

	songbox = (Listbox*)winMgr.createWindow("TaharezLook/Listbox", "songbox");
	fWnd->addChildWindow(songbox);

	int32 newid = 0;
	for (std::vector<Song*>::iterator i = SongList.begin(); i != SongList.end(); i++)
	{
		ListboxTextItem *newItem = new ListboxTextItem((*i)->SongName, newid);
		newItem->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		newItem->setSelectionColours(CEGUI::colour(0,0,1));
		songbox->addItem( newItem );
		newid++;
	}
	songbox->setPosition(UVector2(cegui_reldim(0), cegui_reldim(0)));
	songbox->setSize(UVector2(cegui_reldim(1), cegui_reldim( 0.75f)));

	PushButton *btn = static_cast<PushButton*>(winMgr.createWindow("TaharezLook/Button", "RunScreenButton"));
    fWnd->addChildWindow(btn);
    btn->setPosition(UVector2(cegui_reldim(0.25f), cegui_reldim( 0.87f)));
    btn->setSize(UVector2(cegui_reldim(0.50f), cegui_reldim( 0.05f)));
    btn->setText("Run music!");

	PushButton *btn2 = static_cast<PushButton*>(winMgr.createWindow("TaharezLook/Button", "QuitScreenButton"));
    fWnd->addChildWindow(btn2);
    btn2->setPosition(UVector2(cegui_reldim(0.25f), cegui_reldim( 0.93f)));
    btn2->setSize(UVector2(cegui_reldim(0.50f), cegui_reldim( 0.05f)));
    btn2->setText("Quit");

	PushButton *btn3 = static_cast<PushButton*>(winMgr.createWindow("TaharezLook/Button", "ReloadSongsButton"));
    fWnd->addChildWindow(btn3);
    btn3->setPosition(UVector2(cegui_reldim(0.25f), cegui_reldim( 0.8f)));
    btn3->setSize(UVector2(cegui_reldim(0.50f), cegui_reldim( 0.05f)));
    btn3->setText("Reload Songs");

	// Button that activates the music. Oh yeah!
	winMgr.getWindow("RunScreenButton")->
		subscribeEvent(PushButton::EventClicked, Event::Subscriber(&ScreenSelectMusic::RunMusic, this));

	winMgr.getWindow("QuitScreenButton")->
		subscribeEvent(PushButton::EventClicked, Event::Subscriber(&ScreenSelectMusic::QuitGame, this));

	winMgr.getWindow("ReloadSongsButton")->
		subscribeEvent(PushButton::EventClicked, Event::Subscriber(&ScreenSelectMusic::ReloadSongs, this));


	System::getSingleton().setGUISheet(root);
#endif
}

void ScreenSelectMusic::Cleanup()
{
#ifndef DISABLE_CEGUI
	using namespace CEGUI;
	WindowManager& winMgr = WindowManager::getSingleton();
#endif
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
			CEGUI::System::getSingleton().setGUISheet(root);
			GraphMan.isGuiInputEnabled = true;
			SwitchBackGuiPending = false;
			int rn = rand() % 6;
			Loops[rn]->seek(0);
			Loops[rn]->Start();
		}
	}
#endif

#ifndef DISABLE_CEGUI
	CEGUI::System::getSingleton().renderGUI();
#endif
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
		if (key == GLFW_KEY_F1)
		{
			if (SongList.size() && SongList.at(0)->Difficulties.size())
			{
				ScreenGameplay *_Next = new ScreenGameplay(this);
				_Next->Init(SongList.at(0), 0);

				Next = _Next;
				SwitchBackGuiPending = true;
				GraphMan.isGuiInputEnabled = false;
				StopLoops();
			}
		}

		if (key == GLFW_KEY_F4) // Edit mode!
		{
			if (songbox->getFirstSelectedItem())
			{
				ScreenEdit *_Next = new ScreenEdit(this);
				_Next->Init(SongList.at(songbox->getFirstSelectedItem()->getID()));
				Next = _Next;
				SwitchBackGuiPending = true;
				StopLoops();
			}
		}
	}

}

#ifndef DISABLE_CEGUI
bool ScreenSelectMusic::RunMusic(const CEGUI::EventArgs&)
{
	SelectSnd->Reset();
	if (songbox->getFirstSelectedItem())
	{
		if (SongList.at(songbox->getFirstSelectedItem()->getID())->Difficulties.size())
		{
			ScreenGameplay *_Next = new ScreenGameplay(this);

			StopLoops();

			_Next->Init(SongList.at(songbox->getFirstSelectedItem()->getID()), 0);

			Next = _Next;
			SwitchBackGuiPending = true;
			GraphMan.isGuiInputEnabled = false;
		}
	}
	return true;
}

bool ScreenSelectMusic::QuitGame(const CEGUI::EventArgs&)
{
	Running = false;
	return true;
}

bool ScreenSelectMusic::ReloadSongs(const CEGUI::EventArgs&)
{

	songbox->resetList();

	SongList.clear();

	FileManager::GetSongList(SongList);

	int32 newid = 0;
	for (std::vector<Song*>::iterator i = SongList.begin(); i != SongList.end(); i++)
	{
		CEGUI::ListboxTextItem *newItem = new CEGUI::ListboxTextItem((*i)->SongName, newid);
		newItem->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		newItem->setSelectionColours(CEGUI::colour(0,0,1));
		songbox->addItem( newItem );
		newid++;
	}
	return true;
}
#endif