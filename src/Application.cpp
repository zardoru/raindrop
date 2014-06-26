#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GameGlobal.h"
#include "Screen.h"
#include "Configuration.h"
#include "Audio.h"
#include "Directory.h"
#include "Application.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "FileManager.h"
#include "GameState.h"

#include "ScreenMainMenu.h"
#include "ScreenGameplay7K.h"
#include "Converter.h"

Application::Application(int argc, char *argv[])
{
	oldTime = 0;
	Game = NULL;

	RunMode = MODE_PLAY;
	Args.Argc = argc;
	Args.Argv = argv;

	Upscroll = false;
	difIndex = 0;
	Measure = 0;
	Author = "raindrop";
}

inline bool ValidArg(int count, int req, int i)
{
	return (i + req) < count;
}

void Application::ParseArgs()
{
	for (int i = 1; i < Args.Argc; i++)
	{
		if (Args.Argv[i][0] == '-')
		{
			switch (Args.Argv[i][1])
			{
			case 'p': // Preview file (VSRG preview mode)
				RunMode = MODE_VSRGPREVIEW;
			case 'i': // Infile (convert mode/preview file)
				if (ValidArg(Args.Argc, 1, i))
				{
					InFile = Args.Argv[i+1];
					i++;
				}

				continue;
			case 'o': // Outfile (convert mode)

				RunMode = MODE_CONVERT;

				if (ValidArg(Args.Argc, 1, i))
				{
					OutFile = Args.Argv[i+1];
					i++;
				}

				continue;
			case 'c': // Generate cache
				RunMode = MODE_GENCACHE;
				continue;
			case 'g': // Mode to convert to

				if (ValidArg(Args.Argc, 1, i))
				{
					String Mode = Args.Argv[i+1];
					if (Mode == "om")
						ConvertMode = CONV_OM;
					else if (Mode == "sm")
						ConvertMode = CONV_SM;
					else if (Mode == "bms")
						ConvertMode = CONV_BMS;
					i++;
				}

				continue;
			case 'm': // Measure selected for preview
				if (ValidArg(Args.Argc, 1, i))
				{
					Measure = atoi (Args.Argv[i+1]);
					i++;
				} 
				continue;
			case 'a':

				if (ValidArg(Args.Argc, 1, i))
				{
					Author = Args.Argv[i+1];
					i++;
				} 

			default:
				continue;

			}
		}
	}
}

void Application::Init()
{
	double T1 = glfwGetTime();

	ParseArgs();

#ifdef WIN32
	SetConsoleOutputCP(CP_UTF8);
	_setmode(_fileno(stdout), _O_U8TEXT); 
#endif

	GameState::GetInstance().Initialize();
	GameState::Printf("Initializing... \n");

	Configuration::Initialize();
	FileManager::Initialize();

	if (RunMode == MODE_PLAY || RunMode == MODE_VSRGPREVIEW)
	{
		WindowFrame.AutoSetupWindow(this);
		InitAudio();
		Game = NULL;
	}

	wprintf(L"Total Initialization Time: %fs\n", glfwGetTime() - T1);
}

void Application::Run()
{
	double T1 = glfwGetTime();
	bool RunLoop = true;

	wprintf(L"Link start.\n");

	if (RunMode == MODE_PLAY)
	{
		Game = new ScreenMainMenu(NULL);
		((ScreenMainMenu*)Game)->Init();

	}else if (RunMode == MODE_VSRGPREVIEW)
	{
		Game = new ScreenGameplay7K();
		VSRG::Song* Sng = LoadSong7KFromFilename(InFile.path(), InFile.ParentDirectory().path(), NULL);
		((ScreenGameplay7K*)Game)->Init (Sng, difIndex, Upscroll);
	}else if (RunMode == MODE_CONVERT)
	{
		VSRG::Song* Sng = LoadSong7KFromFilename(InFile.Filename().path(), InFile.ParentDirectory().path(), NULL);

		if (Sng) 
		{
		// if (ConvertMode == CONV_OM) // for now this is the default
			ConvertToOM (Sng, OutFile.path(), Author);
		}

		RunLoop = false;
	}else if (RunMode == MODE_GENCACHE)
	{

		RunLoop = false;
	}

	wprintf(L"Time: %fs\n", glfwGetTime() - T1);

	if (!RunLoop)
		return;

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