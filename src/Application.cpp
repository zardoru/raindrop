#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GameGlobal.h"
#include "Logging.h"
#include "Screen.h"
#include "Configuration.h"
#include "Audio.h"
#include "Directory.h"
#include "Application.h"
#include "GraphObject2D.h"
#include "BitmapFont.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "GameState.h"
#include "ImageList.h"

#include "ScreenMainMenu.h"
#include "ScreenGameplay7K.h"
#include "ScreenLoading.h"
#include "Converter.h"

#include "SongLoader.h"
#include "SongWheel.h"

bool Auto = false;
bool DoRun = false;

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
	srand(time(0));

	Log::Printf("sizeof VSRG::Difficulty: %d, sizeof TimingData: %d\n", sizeof(VSRG::Song), sizeof(TimingData));
}

inline bool ValidArg(int count, int req, int i)
{
	return (i + req) < count;
}

void Application::ParseArgs()
{
	for (int i = 1; i < Args.Argc; i++)
	{
		Log::Printf("%d: %ls\n", i, Utility::Widen(Args.Argv[i]).c_str());
	}

	for (int i = 1; i < Args.Argc; i++)
	{
		if (Args.Argv[i][0] == '-')
		{
			switch (Args.Argv[i][1])
			{
			case 'p': // Preview file (VSRG preview mode)
				RunMode = MODE_VSRGPREVIEW;
				continue;

			case 'i': // Infile (convert mode/preview file)
				if (ValidArg(Args.Argc, 1, i))
				{
					InFile = Directory(Args.Argv[i+1]);
					i++;
				}

				continue;
			case 'o': // Outfile (convert mode)

				RunMode = MODE_CONVERT;

				if (ValidArg(Args.Argc, 1, i))
				{
					OutFile = Directory(Args.Argv[i+1]);
					i++;
				}

				continue;
			case 'c': // Generate cache
				RunMode = MODE_GENCACHE;
				continue;
			case 'g': // Mode to convert to

				if (ValidArg(Args.Argc, 1, i))
				{
					GString Mode = Args.Argv[i+1];
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
					Author = Args.Argv[i+2];
					i++;
				} 

				continue;

			case 'A':
				Auto = true;
				continue;

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
#else
	setlocale(LC_ALL, "");
#endif

	GameState::GetInstance().Initialize();
	Log::Printf("Initializing... \n");

	Configuration::Initialize();

	if (RunMode == MODE_PLAY || RunMode == MODE_VSRGPREVIEW)
	{
		DoRun = WindowFrame.AutoSetupWindow(this);
		InitAudio();
		Game = NULL;
	}
	else
		DoRun = true;

	Log::Printf("Total Initialization Time: %fs\n", glfwGetTime() - T1);
}

void Application::Run()
{
	double T1 = glfwGetTime();
	bool RunLoop = true;

	if (!DoRun)
		return;

	if (RunMode == MODE_PLAY)
	{
		Game = new ScreenMainMenu(NULL);
		((ScreenMainMenu*)Game)->Init();

	}else if (RunMode == MODE_VSRGPREVIEW)
	{
		VSRG::Song* Sng = LoadSong7KFromFilename(InFile.Filename().path(), InFile.ParentDirectory().path(), NULL);

		if (!Sng || !Sng->Difficulties.size())
		{
			Log::Printf("File %s could not be loaded for preview.\n", InFile.c_path());
			return;
		}

		ScreenGameplay7K *SGame = new ScreenGameplay7K();
		ScreenLoading *LoadScreen = new ScreenLoading(NULL, SGame);

		Sng->SongDirectory = InFile.ParentDirectory().path() + "/";

		ScreenGameplay7K::Parameters Param;
		Param.Upscroll = Upscroll;
		Param.StartMeasure = Measure;
		Param.Preloaded = true;
		Param.Auto = Auto;

		SGame->Init (Sng, difIndex, Param);
		LoadScreen->Init();

		Game = LoadScreen;
	}else if (RunMode == MODE_CONVERT)
	{
		VSRG::Song* Sng = LoadSong7KFromFilename(InFile.Filename().path(), InFile.ParentDirectory().path(), NULL);

		if (Sng && Sng->Difficulties.size()) 
		{
			if (ConvertMode == CONV_OM) // for now this is the default
				ConvertToOM (Sng, OutFile.path(), Author);
			else
				ConvertToSMTiming(Sng, OutFile.path());
		}

		RunLoop = false;
	}else if (RunMode == MODE_GENCACHE)
	{
		Log::Printf("Generating cache...\n");
		Game::SongWheel::GetInstance().ReloadSongs();

		RunLoop = false;
	}

	Log::Printf("Time: %fs\n", glfwGetTime() - T1);

	if (!RunLoop)
		return;

	ImageLoader::UpdateTextures();

	oldTime = glfwGetTime();
	while (Game->IsScreenRunning() && !WindowFrame.ShouldCloseWindow())
	{
		double newTime = glfwGetTime();
		double delta = newTime - oldTime;
		ImageLoader::UpdateTextures();

		WindowFrame.ClearWindow();

		Game->Update(delta);

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
	{
		Game->Cleanup();
		delete Game;
	}

	WindowFrame.Cleanup();
	Configuration::Cleanup();
}
