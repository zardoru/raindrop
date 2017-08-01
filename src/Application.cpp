#include "pch.h"

#include "GameGlobal.h"
#include "Logging.h"
#include "Screen.h"

#include "Audio.h"
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
#include "ScreenCustom.h"

#include "ScreenVideoTest.h"

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
        ("output,o", po::value<std::string>(),
        "Output File")
        ("gencache,c",
        "Generate Song Cache")
        ("format,g", po::value<std::string>(),
        "Target Format")
        ("measure,m", po::value<unsigned>()->default_value(0),
        "Measure")
        ("author,a", po::value<std::string>()->default_value("raindrop"),
        "Author")
        ("A,A",
        "Auto")
        ("S,S",
        "Stop Preview Instance")
        ("R,R",
        "Release IPC Pool")
        ("L,L", po::value<std::string>(),
        "Load Custom Scene")
		("test,T", "Run test suite")
		("fontcache,F", po::value<std::string>(),
		"Generate Font Cache")
        ;

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch (...)
    {
        std::cout << "unknown / incompatible option supplied" << std::endl;
        return;
    }
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
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

    if (vm.count("output"))
    {
        RunMode = MODE_CONVERT;
        OutFile = vm["output"].as<std::string>();
    }

    if (vm.count("gencache"))
    {
        RunMode = MODE_GENSONGCACHE;
    }

	if (vm.count("fontcache")) {
		RunMode = MODE_GENFONTCACHE;
		InFontTextFile = vm["fontcache"].as<std::string>();
	}

    if (vm.count("format"))
    {
		ConvertMode = std::map<std::string, CONVERTMODE>{
			{ "om", CONVERTMODE::CONV_OM },
			{ "sm", CONVERTMODE::CONV_SM },
			{ "bms", CONVERTMODE::CONV_BMS },
			{ "uqbms", CONVERTMODE::CONV_UQBMS },
			{ "nps", CONVERTMODE::CONV_NPS },
			{ "acctest", CONVERTMODE::CONV_ACCTEST }
        }.at(vm["format"].as<std::string>());

		Log::LogPrintf("Setting format to %s (%d)\n", vm["format"].as<std::string>().c_str(), ConvertMode);
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

	if (vm.count("test"))
	{
		RunMode = MODE_TEST;
	}

    return;
}

void Application::Init()
{
    using Clock = std::chrono::high_resolution_clock;
    auto t1 = Clock::now();

#if (defined WIN32) && !(defined MINGW)
	if (RunMode != MODE_TEST) {
		SetConsoleOutputCP(CP_UTF8);
		_setmode(_fileno(stdout), _O_U8TEXT);
	}
#else
    setlocale(LC_ALL, "");
#endif

	Log::Printf(RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT " start.\n");
	// Log::Printf("Current Time: %s.\n", t1);
	Log::Printf("Working directory: %s\n", Utility::ToU8(std::filesystem::current_path().wstring()).c_str());

#if (defined WIN32)
	Log::Printf("Current codepage: %u\n", GetACP());
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
            Game::SongWheel::GetInstance().LoadSongsOnce(Game::GameState::GetInstance().GetSongDatabase());
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

    if (RunMode == MODE_CUSTOMSCREEN)
        Setup = true;

    if (Setup)
    {
        DoRun = WindowFrame.AutoSetupWindow(this);
        InitAudio();
        Engine::SetupRocket();
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
        Log::Printf("File %s could not be loaded for preview. (%d/%d)\n", InFile.c_str(), (long long int)song.get(), song ? song->Difficulties.size() : 0);
        return;
    }

    // Avoid a crash...
    GameState::GetInstance().SetSelectedSong(song);

    // Create loading screen and gameplay screen.
    auto game = std::make_shared<Game::VSRG::ScreenGameplay>();
    auto LoadScreen = std::make_shared<ScreenLoading>(game);

    // Set them up.
	song->SongDirectory = std::filesystem::absolute(InFile.parent_path());

	/*
    Game::VSRG::PlayscreenParameters param;

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

void ExportToBMSUnquantized(Game::VSRG::Song* Source, std::filesystem::path PathOut);

void Application::Run()
{
    double T1 = glfwGetTime();
    bool RunLoop = true;

	// if we're just running tests, legitimately don't give a crap about
	// what goes on below.
	if (RunMode == MODE_TEST)
	{
		RunRaindropTests();
		return;

	}


    if (!DoRun)
        return;

    if (RunMode == MODE_PLAY)
    {
        Game = std::make_shared<ScreenMainMenu>();
        static_cast<ScreenMainMenu*>(Game.get())->Init();
		//Game = std::make_shared<ScreenVideoTest>();
    }
    else if (RunMode == MODE_VSRGPREVIEW)
    {
        if (IPC::IsInstanceAlreadyRunning())
        {
            // So okay then, we'll send a message telling the existing process to restart with this file, at this time.
            IPC::Message Msg;
            Msg.MessageKind = IPC::Message::MSG_STARTFROMMEASURE;
            Msg.Param = Measure;
            strncpy(Msg.Path, Utility::ToU8(InFile.wstring()).c_str(), 256);

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
    else if (RunMode == MODE_CONVERT)
    {
		InFile = std::filesystem::absolute(InFile);
        std::shared_ptr<Game::VSRG::Song> Sng = LoadSong7KFromFilename(InFile.filename(), InFile.parent_path(), nullptr);

		Log::Printf("Conversion mode activated.\n");
        if (Sng && Sng->Difficulties.size())
        {
			Log::Printf("Initiating conversion for mode %d.\n", ConvertMode);
            if (ConvertMode == CONVERTMODE::CONV_OM) // for now this is the default
                ConvertToOM(Sng.get(), OutFile, Author);
            else if (ConvertMode == CONVERTMODE::CONV_BMS)
                ConvertToBMS(Sng.get(), OutFile);
            else if (ConvertMode == CONVERTMODE::CONV_UQBMS)
                ExportToBMSUnquantized(Sng.get(), OutFile);
            else if (ConvertMode == CONVERTMODE::CONV_NPS)
                ConvertToNPSGraph(Sng.get(), OutFile);
			else if (ConvertMode == CONVERTMODE::CONV_ACCTEST)
			{
				auto msr = Sng->Difficulties[0]->Data->Measures;
				auto timeset = std::set<double>();
				for (auto m : msr) {
					for (auto i = 0; i < Sng->Difficulties[0]->Channels; i++)
						for (auto note : m.Notes[i])
							if (note.NoteKind == 0)
								timeset.insert(note.StartTime);
				}

				if (OutFile.string().length()) {
					Log::Printf("Attempt to open %s...\n", std::filesystem::absolute(OutFile).string().c_str());
					std::fstream out(OutFile.string(), std::ios::out);

					if (!out.is_open())
						Log::Printf("Couldn't open!?\n");

					out << std::setprecision(17) << std::fixed;
					for (auto d : timeset)
						out << d << std::endl;
				}
				else {
					for (auto d : timeset)
						Log::Printf("%.17f\n", d);
				}
			}
			else
                ConvertToSMTiming(Sng.get(), OutFile);
        }
        else
        {
            if (Sng) Log::Printf("No notes or timing were loaded.\n");
            else Log::Printf("Failure loading file at all.\n");
        }

        RunLoop = false;
    }
    else if (RunMode == MODE_GENSONGCACHE)
    {
        Log::Printf("Generating cache...\n");
        Game::GameState::GetInstance().Initialize();
        Game::SongWheel::GetInstance().Initialize(Game::GameState::GetInstance().GetSongDatabase());
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
    else if (RunMode == MODE_CUSTOMSCREEN)
    {
        Log::Printf("Initializing custom, ad-hoc screen...\n");
		auto s = Utility::ToU8(InFile.wstring());
        auto scr = std::make_shared<ScreenCustom>(GameState::GetInstance().GetSkinFile(s));
        Game = scr;
    }

    Log::Printf("Time: %fs\n", glfwGetTime() - T1);

    if (!RunLoop)
        return;

    ImageLoader::UpdateTextures();
	GameState::GetInstance().SetRootScreen(Game);

    oldTime = glfwGetTime();
    while (Game->IsScreenRunning() && !WindowFrame.ShouldCloseWindow())
    {
        double newTime = glfwGetTime();
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

void Application::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
{
	if (BindingsManager::TranslateKey(key) == KT_ReloadCFG && code == KE_PRESS)
		Configuration::Reload();

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
		Game = nullptr;
    }

    WindowFrame.Cleanup();
    Configuration::Cleanup();
}

void Application::HandleTextInput(unsigned cp)
{
    Game->HandleTextInput(cp);
}
