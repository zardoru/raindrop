#include "Global.h"
#include <GL/glew.h>
#include <GL/glfw.h>
#include <CEGUI.h>
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "ScreenGameplay.h"
#include "ScreenEdit.h"
#include "Application.h"
#include "Audio.h"
#include "GraphicsManager.h"

cAudio::IAudioManager* audioMgr;
cAudio::IAudioDeviceList* pDeviceList;



Application::Application()
{
}

void Application::Init()
{
	cAudio::cAudioString DeviceName;
	GraphMan.AutoSetupWindow();

	audioMgr = cAudio::createAudioManager(false);
	pDeviceList = cAudio::createAudioDeviceList();
	DeviceName = pDeviceList->getDefaultDeviceName();
	audioMgr->initialize(DeviceName.c_str());
	Game = NULL;

	oldTime = 0;
	glfwSetTime(0.0); // this should match
	// throw a message here
}

void Application::Run()
{
	//Game = new ScreenSelectMusic();
	ScreenEdit* sE = new ScreenEdit(NULL);
	Song* editSong = new Song();
	editSong->SongDir = "IT.ogg";

	sE->Init(editSong);

	Game = sE;
	
	//Game->Init();

	while (Game->IsScreenRunning())
	{
		double newTime = glfwGetTime();
		double delta = newTime - oldTime;

		GraphMan.ClearWindow();

		// shit gets real
		Game->Run(delta);

#ifndef DISABLE_CEGUI
		CEGUI::System::getSingleton().injectTimePulse(delta);
#endif

		glfwSwapBuffers();
		oldTime = newTime;
	}
}

void Application::HandleInput(int key, int code, bool isMouseInput)
{
	Game->HandleInput(key, code, isMouseInput);
}

void Application::Close()
{
	glfwCloseWindow();
	glfwTerminate();
}