#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Global.h"
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenSelectMusic.h"
#include "ScreenGameplay.h"
#include "ScreenEdit.h"
#include "Application.h"
#include "Audio.h"
#include "GameWindow.h"
#include "BitmapFont.h"

Application::Application()
{
}

void Application::Init()
{
	WindowFrame.AutoSetupWindow();
	InitAudio();
	Game = NULL;
	srand(time(0));
	oldTime = 0;
	// throw a message here
}

	

void Application::Run()
{
	Game = new ScreenSelectMusic();
	Game->Init();

	while (Game->IsScreenRunning() && !WindowFrame.ShouldCloseWindow())
	{
		double newTime = glfwGetTime();
		double delta = newTime - oldTime;
		WindowFrame.ClearWindow();

		// shit gets real
		Game->Run(delta);

		MixerUpdate();
		WindowFrame.SwapBuffers();
		oldTime = newTime;
	}
}

void Application::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	Game->HandleInput(key, code, isMouseInput);
}

void Application::Close()
{
	WindowFrame.Cleanup();
}