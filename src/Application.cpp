#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Global.h"
#include "Screen.h"
#include "Configuration.h"
#include "Audio.h"
#include "Application.h"
#include "GameObject.h"
#include "BitmapFont.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "FileManager.h"

#include "Song.h"

#include "ScreenMainMenu.h"

Application::Application()
{
	oldTime = 0;
	Game = NULL;
}

void Application::Init()
{
	Configuration::Initialize();
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

void Application::HandleScrollInput(double xOff, double yOff)
{
	Game->HandleScrollInput(xOff, yOff);
}

void Application::Close()
{
	if (Game)
		delete Game;
	WindowFrame.Cleanup();
	Configuration::Cleanup();
}