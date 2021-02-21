#include <cstdint>
#include <boost/program_options.hpp>
#include <memory>
#include <queue>
#include <future>
#include <fstream>
#include <glm.h>
#include <rmath.h>
#include <json.hpp>

#include <game/GameConstants.h>
#include <game/Song.h>
#include <game/VSRGMechanics.h>

#include "Game.h"
#include "Logging.h"
#include "Screen.h"

#include "Audio.h"
#include "Application.h"
#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "ImageLoader.h"
#include "GameWindow.h"
#include "LuaManager.h"

#include "PlayscreenParameters.h"
#include "GameState.h"


#include "ScreenMainMenu.h"

#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>
#include <iostream>
#include "BackgroundAnimation.h"
#include "ScreenGameplay7K.h"

#include "ScreenLoading.h"

#include "IPC.h"

/* fixme: to be replaced with rmlui */
// #include "RaindropRocketInterface.h"

#include "SongLoader.h"
#include "SongList.h"
#include "SongWheel.h"
#include "ScreenCustom.h"

#include "Configuration.h"
#include "TextAndFileUtil.h"

#include "TruetypeFont.h"

void RunRaindropTests();
bool Auto = false;
bool DoRun = false;

Application::Application(int argc, char *argv[])
{
    oldTime = 0;
    Game = nullptr;
    RunMode = MODE_PLAY;
    Upscroll = false;
    difIndex = 0;

    ParseArgs(argc, argv);
}

void Application::ParseArgs(int argc, char **argv)
{
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,?",
        "show help message")
        ("preview,p",
        "Preview File")
        ("input,i", po::value<std::string>(),
        "Input File")
        ("gencache,c",
        "Generate Song Cache")
        ("measure,m", po::value<unsigned>()->default_value(0),
        "Measure")
        ("A,A",
        "Auto")
        ("S,S",
        "Stop Preview Instance")
        ("R,R",
        "Release IPC Pool")
        ("L,L", po::value<std::string>(),
        "Load Custom Scene")
		("fontcache,F", po::value<std::string>(),
        "Generate Font Cache")
        ("config,x", po::value<std::string>(), 
        "Set config file")
        ;

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch (...)
    {
        Log::Printf("unknown / incompatible option supplied\n");
        RunMode = MODE_NULL;
        return;
    }
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc;
        return;
    }

    if (vm.count("preview"))
    {
        RunMode = MODE_VSRGPREVIEW;
    }

    if (vm.count("input"))
    {
        InFile = vm["input"].as<std::string>();
    }

    if (vm.count("config"))
    {
        Configuration::SetConfigFile(vm["config"].as<std::string>());
    }

    if (vm.count("gencache"))
    {
        RunMode = MODE_GENSONGCACHE;
    }

	if (vm.count("fontcache")) {
		RunMode = MODE_GENFONTCACHE;
		InFontTextFile = vm["fontcache"].as<std::string>();
	}



    if (vm.count("measure"))
    {
        Measure = vm["measure"].as<unsigned>();
    }

    if (vm.count("a"))
    {
        Author = vm["a"].as<std::string>();
    }

    if (vm.count("A"))
    {
        Auto = true;
    }

    if (vm.count("S"))
    {
        RunMode = MODE_STOPPREVIEW;
    }

    if (vm.count("R"))
    {
        RunMode = MODE_NULL;
        IPC::RemoveQueue();
    }

    if (vm.count("L"))
    {
        RunMode = MODE_CUSTOMSCREEN;
        InFile = vm["L"].as<std::string>();
    }

}

void Application::Init()
{
    using Clock = std::chrono::high_resolution_clock;
    auto t1 = Clock::now();

    /*
     // is this necessary, tbh?
#if (defined WIN32) && !(defined MINGW)
	SetConsoleOutputCP(CP_UTF8);
	_setmode(_fileno(stdout), _O_U8TEXT);
#else
    setlocale(LC_ALL, "");
#endif
     */

    setbuf(stdout, 0);

	Log::Printf(RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT " start.\n");
	// Log::Printf("Current Time: %s.\n", t1);
	Log::Printf("Working directory: %s\n", Conversion::ToU8(std::filesystem::current_path().wstring()).c_str());

	/*
#if (defined WIN32)
	Log::Printf("Current codepage: %u\n", GetACP());
#endif
	 */

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
            SongWheel::GetInstance().LoadSongsOnce(GameState::GetInstance().GetSongDatabase());
            SongWheel::GetInstance().Join();
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

    if (RunMode == MODE_CUSTOMSCREEN)
        Setup = true;

    if (Setup)
    {
        DoRun = WindowFrame.AutoSetupWindow(this);
        InitAudio();
        // XXX: rmlui
        // Engine::SetupRocket();
        Game = nullptr;
    }
    else
    {
        if (RunMode != MODE_NULL)
            DoRun = true;
    }

    Log::Printf("Total Initialization Time: %fs\n", std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t1).count() / 1000000.0);
}

void Application::SetupPreviewMode()
{
    // Load the song.
    auto song = LoadSong7KFromFilename(InFile);


    if (!song || !song->Difficulties.size())
    {
        Log::Printf("File %ls could not be loaded for preview. (ptr %d/diffcnt %d)\n", InFile.c_str(), (long long int)song.get(), song ? song->Difficulties.size() : 0);
        return;
    }

    GameState::GetInstance().SetSelectedSong(song);
    // Create loading screen and gameplay screen.
    auto game = std::make_shared<ScreenGameplay>();
    auto LoadScreen = std::make_shared<ScreenLoading>(game);

    // Set them up.
	song->SongDirectory = std::filesystem::absolute(InFile.parent_path());

	/*
    rd::VSRG::PlayscreenParameters param;

    param.Upscroll = Upscroll;
    param.StartMeasure = Measure;
    param.Preloaded = true;
    param.Auto = Auto;
    */
    
	GameState::GetInstance().GetParameters(0)->Auto = Auto;
    game->Init(song);
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
        InFile = std::string(Msg.Path);
        Game->Close();
		Game = nullptr;

        SetupPreviewMode();

        return true;
    case IPC::Message::MSG_STOP:
        Game->Close();
        return true;
    case IPC::Message::MSG_NULL:
    default:
        return false;
    }
}


void Application::Run()
{
    double T1 = WindowFrame.GetCurrentTime();
    bool RunLoop = true;

    if (!DoRun)
        return;

    if (RunMode == MODE_PLAY)
    {
        auto scr = std::make_shared<ScreenMainMenu>();
        scr->Init();
        Game = scr;
    }
    else if (RunMode == MODE_VSRGPREVIEW)
    {
        if (IPC::IsInstanceAlreadyRunning())
        {
            // So okay then, we'll send a message telling the existing process to restart with this file, at this time.
            IPC::Message Msg;
            Msg.MessageKind = IPC::Message::MSG_STARTFROMMEASURE;
            Msg.Param = Measure;
            strncpy(Msg.Path, Conversion::ToU8(InFile.wstring()).c_str(), 256);

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
    }
    else if (RunMode == MODE_GENSONGCACHE)
    {
        Log::Printf("Generating cache...\n");
        GameState::GetInstance().Initialize();
        SongWheel::GetInstance().Initialize(GameState::GetInstance().GetSongDatabase());
        SongWheel::GetInstance().Join();

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
    else if (RunMode == MODE_CUSTOMSCREEN)
    {
        Log::Printf("Initializing custom, ad-hoc screen...\n");
		auto s = Conversion::ToU8(InFile.wstring());
        auto scr = std::make_shared<ScreenCustom>(GameState::GetInstance().GetSkinFile(s));
        Game = scr;
	}
	else if (RunMode == MODE_GENFONTCACHE)
	{
		Log::Printf("Generating font cache for provided file...\n");

		TruetypeFont::GenerateFontCache(InFontTextFile, InFile);

		RunLoop = false;
	}

    Log::Printf("Time: %fs\n", WindowFrame.GetCurrentTime() - T1);

    if (!RunLoop)
        return;

    ImageLoader::UpdateTextures();
	GameState::GetInstance().SetRootScreen(Game);

    oldTime = WindowFrame.GetCurrentTime();
    while (Game->IsScreenRunning() && !WindowFrame.ShouldCloseWindow())
    {
        double newTime = WindowFrame.GetCurrentTime();
        double delta = newTime - oldTime;
        ImageLoader::UpdateTextures();

		WindowFrame.RunInput();

        WindowFrame.ClearWindow();

        if (RunMode == MODE_VSRGPREVIEW) // Run IPC Message Queue Querying.
            if (PollIPC()) continue;

        Game->Update(delta);

        MixerUpdate();
        WindowFrame.SwapBuffers();
		WindowFrame.UpdateFullscreen();

        oldTime = newTime;
    }
}

void Application::HandleInput(int32_t key, bool isPressed, bool isMouseInput)
{
	if (BindingsManager::TranslateKey(key) == KT_ReloadCFG && isPressed)
		Configuration::Reload();

    Game->HandleInput(key, isPressed, isMouseInput);
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
		Game = nullptr;
    }

    WindowFrame.Cleanup();
    Configuration::Cleanup();
}

void Application::HandleTextInput(unsigned cp)
{
    Game->HandleTextInput(cp);
}
