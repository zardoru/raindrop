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
#include "BitmapFont.h"

void Utility::DebugBreak()
{
#ifndef NDEBUG
#ifdef WIN32
	__asm int 3
#else
	asm("int 3");
#endif
#endif
}

Application::Application()
{
}

void Application::Init()
{
	GraphMan.AutoSetupWindow();
	InitAudio();
	Game = NULL;

	oldTime = 0;
	glfwSetTime(0.0); // this should match
	// throw a message here
}

	

void Application::Run()
{
	Game = new ScreenSelectMusic();
	Game->Init();

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