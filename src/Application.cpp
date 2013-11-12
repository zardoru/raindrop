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

bool Accomulation = false;

void Application::Run()
{
	int Frames = 4, Counter = 0;
	float timeStep = 1.0 / 30.0, timeTotal = 0;
	Game = new ScreenMainMenu(NULL);
	Game->Init();

	while (Game->IsScreenRunning() && !WindowFrame.ShouldCloseWindow())
	{
		double newTime = glfwGetTime();
		double delta = newTime - oldTime;
		WindowFrame.ClearWindow();
		ImageLoader::UpdateTextures();

		timeTotal += delta;

		Game->Run(delta);

		// shit gets real
		if (timeTotal > timeStep)
		{
			if (Accomulation)
			{

				if (Counter == 0)
				{
					glAccum(GL_LOAD, 1.0 / Frames);
				}else
					glAccum(GL_ACCUM, 1.0 / Frames);

				Counter++;

				if (Counter >= Frames)
				{
					Counter = 0;
					glAccum(GL_RETURN, 1.0);
					WindowFrame.SwapBuffers();
				}

			}

			timeTotal -= timeStep;
		}

		MixerUpdate();

		if (!Accomulation)
			WindowFrame.SwapBuffers();
		oldTime = newTime;
	}
}

void Application::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	Game->HandleInput(key, code, isMouseInput);

	if (key == 'Y' && code == KE_Press)
		Accomulation = !Accomulation;
}

void Application::Close()
{
	if (Game)
		delete Game;
	WindowFrame.Cleanup();
}