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
#include "Sprite.h"
#include "BitmapFont.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "GameState.h"
#include "ImageList.h"

#include "ScreenMainMenu.h"
#include "ScreenGameplay7K.h"
#include "ScreenLoading.h"
#include "Converter.h"

#include "IPC.h"
#include "RaindropRocketInterface.h"

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
					else if (Mode == "uqbms")
						ConvertMode = CONV_UQBMS;
					else if (Mode == "nps")
						ConvertMode = CONV_NPS;
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

			case 'S':
				RunMode = MODE_STOPPREVIEW;
				break;
			case 'R':
				RunMode = MODE_NULL;
				IPC::RemoveQueue();
				break;
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

#if (defined WIN32) && !(defined MINGW)
	SetConsoleOutputCP(CP_UTF8);
	_setmode(_fileno(stdout), _O_U8TEXT); 
#else
	setlocale(LC_ALL, "");
#endif

	GameState::GetInstance().Initialize();
	Log::Printf("Initializing... \n");

	Configuration::Initialize();

	bool Setup = false;

	if (RunMode == MODE_PLAY)
	{
		Setup = true;

		if (Configuration::GetConfigf("Preload"))
		{
			Log::Printf("Preloading songs...");
			Game::SongWheel::GetInstance().Initialize(0, 0, Game::GameState::GetInstance().GetSongDatabase(), false);
			Game::SongWheel::GetInstance().Join();
		}

	}
	if (RunMode == MODE_VSRGPREVIEW)
	{
#ifdef NDEBUG
		if (IPC::IsInstanceAlreadyRunning())
			Setup = false;
		else
#endif
			Setup = true;
	}

	if (Setup)
	{
		DoRun = WindowFrame.AutoSetupWindow(this);
		InitAudio();
		Engine::SetupRocket();
		Game = nullptr;
	}else
	{
		if (RunMode != MODE_NULL)
			DoRun = true;
	}

	Log::Printf("Total Initialization Time: %fs\n", glfwGetTime() - T1);
}

void Application::SetupPreviewMode()
{
	// Load the song.
	shared_ptr<VSRG::Song> Sng = LoadSong7KFromFilename(InFile.Filename().path(), InFile.ParentDirectory().path(), nullptr);

	if (!Sng || !Sng->Difficulties.size())
	{
		Log::Printf("File %s could not be loaded for preview. (%d/%d)\n", InFile.c_path(), (long long int)Sng.get(), Sng ? Sng->Difficulties.size() : 0);
		return;
	}

	// Avoid a crash...
	GameState::GetInstance().SetSelectedSong(Sng);
	GameState::GetInstance().SetDifficultyIndex(difIndex);

	// Create loading screen and gameplay screen.
	ScreenGameplay7K *SGame = new ScreenGameplay7K();
	ScreenLoading *LoadScreen = new ScreenLoading(nullptr, SGame);

	// Set them up.
	Sng->SongDirectory = InFile.ParentDirectory().path() + "/";

	GameParameters Param;
	Param.Upscroll = Upscroll;
	Param.StartMeasure = Measure;
	Param.Preloaded = true;
	Param.Auto = Auto;
	
	SGame->Init(Sng, difIndex, Param);
	LoadScreen->Init();

	Game = LoadScreen;
}

bool Application::PollIPC()
{
	IPC::Message Msg = IPC::PopMessageFromQueue();
	switch (Msg.MessageKind)
	{
	case IPC::Message::MSG_STARTFROMMEASURE:
		Measure = Msg.Param;
		InFile = GString(Msg.Path);
		Game->Close();
		delete Game;

		SetupPreviewMode();

		return true;
		break;
	case IPC::Message::MSG_STOP:
		Game->Close();
		return true;
		break;
	case IPC::Message::MSG_NULL:
	default:
		return false;
	}
}


void ExportToBMSUnquantized(VSRG::Song* Source, Directory PathOut);

void Application::Run()
{
	double T1 = glfwGetTime();
	bool RunLoop = true;

	if (!DoRun)
		return;

	if (RunMode == MODE_PLAY)
	{
		Game = new ScreenMainMenu(NULL);
		static_cast<ScreenMainMenu*>(Game)->Init();

	}else if (RunMode == MODE_VSRGPREVIEW)
	{
		if (IPC::IsInstanceAlreadyRunning())
		{
			// So okay then, we'll send a message telling the existing process to restart with this file, at this time.
			IPC::Message Msg;
			Msg.MessageKind = IPC::Message::MSG_STARTFROMMEASURE;
			Msg.Param = Measure;
			strncpy(Msg.Path, InFile.c_path(), 256);

			IPC::SendMessageToQueue(&Msg);
			RunLoop = false;
		}
		else
		{
			SetupPreviewMode();

			if (!Game)
				return;

			// Set up the message queue. We need this if we're in preview mode to be able to control raindrop from the command line.
			IPC::SetupMessageQueue();
		}
	}else if (RunMode == MODE_CONVERT)
	{
		shared_ptr<VSRG::Song> Sng = LoadSong7KFromFilename(InFile.Filename().path(), InFile.ParentDirectory().path(), NULL);

		if (Sng && Sng->Difficulties.size()) 
		{
			if (ConvertMode == CONV_OM) // for now this is the default
				ConvertToOM(Sng.get(), OutFile.path(), Author);
			else if (ConvertMode == CONV_BMS)
				ConvertToBMS(Sng.get(), OutFile.path());
			else if (ConvertMode == CONV_UQBMS)
				ExportToBMSUnquantized(Sng.get(), OutFile.path());
			else if (ConvertMode == CONV_NPS)
				ConvertToNPSGraph(Sng.get(), OutFile.path());
			else
				ConvertToSMTiming(Sng.get(), OutFile.path());
		}
		else
		{
			if (Sng) Log::Printf("No notes or timing were loaded.\n");
			else Log::Printf("Failure loading file at all.\n");
		}

		RunLoop = false;
	}else if (RunMode == MODE_GENCACHE)
	{
		Log::Printf("Generating cache...\n");
		Game::GameState::GetInstance().Initialize();
		Game::SongWheel::GetInstance().Initialize(0, 0, Game::GameState::GetInstance().GetSongDatabase(), false);
		Game::SongWheel::GetInstance().Join();

		RunLoop = false;
	}
	else if (RunMode == MODE_STOPPREVIEW)
	{
		if (IPC::IsInstanceAlreadyRunning())
		{
			// So okay then, we'll send a message telling the existing process to restart with this file, at this time.
			IPC::Message Msg;
			Msg.MessageKind = IPC::Message::MSG_STOP;

			IPC::SendMessageToQueue(&Msg);
		}

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

		if (RunMode == MODE_VSRGPREVIEW) // Run IPC Message Queue Querying.
			if (PollIPC()) continue;

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
