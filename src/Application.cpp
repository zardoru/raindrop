#include "Global.h"
#include <GL/glew.h>
#include <GL/glfw.h>
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

Application::Application()
{
}

void Application::Init()
{
	GraphMan.AutoSetupWindow();
	InitAudio();
	Game = NULL;
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
		GraphMan.ClearWindow();

		// shit gets real
		Game->Run(delta);

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