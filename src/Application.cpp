#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Global.h"
#include "Screen.h"
#include "GameObject.h"
#include "Song.h"
#include "ScreenMainMenu.h"
#include "Application.h"
#include "Audio.h"
#include "GameWindow.h"
#include "BitmapFont.h"
#include "ImageLoader.h"

Application::Application()
{
	oldTime = 0;
	Game = NULL;
}

void Application::Init()
{
	WindowFrame.AutoSetupWindow();
	InitAudio();
	Game = NULL;
	srand(time(0));
	// throw a message here
}

void Application::Run()
{
	Game = new ScreenMainMenu(NULL);
	((ScreenMainMenu*)Game)->Init();

	while (Game->IsScreenRunning() && !WindowFrame.ShouldCloseWindow())
	{
		double newTime = glfwGetTime();
		double delta = newTime - oldTime;
		ImageLoader::UpdateTextures();

		WindowFrame.ClearWindow();

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
	if (Game)
		delete Game;
	WindowFrame.Cleanup();
}