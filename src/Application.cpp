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

BitmapFont FPSDisplay;

Application::Application()
{
}

void Application::Init()
{
	GraphMan.AutoSetupWindow();
	InitAudio();
	Game = NULL;
	FPSDisplay.LoadSkinFontImage("font.tga", glm::vec2(18, 32), glm::vec2(34,34), glm::vec2(10,16), 32);
	srand(time(0));
	oldTime = 0;
	glfwSetTime(0.0); // this should match
	// throw a message here
}

	

void Application::Run()
{
	Game = new ScreenSelectMusic();
	Game->Init();

	while (Game->IsScreenRunning() && glfwGetWindowParam(GLFW_OPENED))
	{
		double newTime = glfwGetTime();
		double delta = newTime - oldTime;
		std::stringstream str;
		str << "fps: " << 1/delta;
		GraphMan.ClearWindow();

		// shit gets real
		Game->Run(delta);

#ifndef DISABLE_CEGUI
		CEGUI::System::getSingleton().injectTimePulse(delta);
#endif
		FPSDisplay.DisplayText(str.str().c_str(), glm::vec2(0,0));
		MixerUpdate();
		glfwSwapBuffers();
		oldTime = newTime;
	}
}

void Application::HandleInput(int32 key, int32 code, bool isMouseInput)
{
	Game->HandleInput(key, code, isMouseInput);
}

void Application::Close()
{
	glfwCloseWindow();
	glfwTerminate();
}