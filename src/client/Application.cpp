#include <cstdint>
#include <boost/program_options.hpp>
#include <memory>
#include <queue>
#include <future>
#include <fstream>
#include <glm.h>
#include <rmath.h>


#include <game/GameConstants.h>
#include <game/VSRGMechanics.h>
#include <note_loader.h>

#include "game/Game.h"
#include "Logging.h"
#include "structure/Screen.h"

#include "Audio.h"
#include "Application.h"
#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "TextureCollection.h"
#include "GameWindow.h"
#include "LuaManager.h"

#include "game/PlayscreenParameters.h"
#include "game/GameState.h"


#include "screens/ScreenMainMenu.h"

#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>
#include <iostream>
#include "bga/BackgroundAnimation.h"
#include "game/PlayerContext.h"
#include "screens/ScreenGameplay.h"

#include "screens/ScreenLoading.h"

#include "IPC.h"

/* fixme: to be replaced with rmlui */
// #include "RaindropRocketInterface.h"

#include "songdb/SongLoader.h"
#include "songdb/SongList.h"
#include "songdb/SongWheel.h"
#include "structure/ScreenCustom.h"

#include "structure/Configuration.h"
#include <text_and_file_util.h>

#include "TruetypeFont.h"

bool Auto = false;
bool good_to_go = false;

Application::Application(const int argc, char *argv[])
{
    oldTime = 0;
    root = nullptr;
    mode = MODE_PLAY;
    Upscroll = false;
    difIndex = 0;

    parse_args(argc, argv);
}

void Application::parse_args(const int argc, char **argv)
{
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,?",
        "show help message")
        ("preview,p", /* unused */
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
        mode = MODE_NULL;
        return;
    }
    po::notify(vm);

    if (vm.contains("help"))
    {
        std::cout << desc;
        return;
    }

    if (vm.contains("input"))
    {
        InFile = vm["input"].as<std::string>();
        mode = MODE_VSRGPREVIEW;
    }

    if (vm.contains("config"))
    {
        Configuration::SetConfigFile(vm["config"].as<std::string>());
    }

    if (vm.contains("gencache"))
    {
        mode = MODE_GENSONGCACHE;
    }

	if (vm.contains("fontcache")) {
		mode = MODE_GENFONTCACHE;
		InFontTextFile = vm["fontcache"].as<std::string>();
	}



    if (vm.contains("measure"))
    {
        measure_ = vm["measure"].as<unsigned>();
    }

    if (vm.contains("a"))
    {
        Author = vm["a"].as<std::string>();
    }

    if (vm.contains("A"))
    {
        Auto = true;
    }

    if (vm.contains("S"))
    {
        mode = MODE_STOPPREVIEW;
    }

    if (vm.contains("R"))
    {
        mode = MODE_NULL;
        IPC::RemoveQueue();
    }

    if (vm.contains("L"))
    {
        mode = MODE_CUSTOMSCREEN;
        InFile = vm["L"].as<std::string>();
    }

}

void Application::init()
{
    using Clock = std::chrono::high_resolution_clock;
    auto t1 = Clock::now();

    setbuf(stdout, 0);

	Log::Printf(RAINDROP_WINDOWTITLE RAINDROP_VERSIONTEXT " start.\n");
	Log::Printf("Working directory: %s\n", otoworm::locale::wstring_to_utf8(std::filesystem::current_path().wstring()).c_str());

    Configuration::Initialize();
    GameState::get_instance().initialize();
    Log::Printf("Configuration and Game State OK. \n");

    bool open_window = false;

    if (mode == MODE_PLAY)
    {
        open_window = true;

        if (Configuration::GetConfigf("Preload"))
        {
            Log::Printf("Preloading songs...");
            SongWheel::get_instance().load_songs_once(GameState::get_instance().get_song_database());
            SongWheel::get_instance().join_loading_thread();
        }
    }
    if (mode == MODE_VSRGPREVIEW)
    {
#ifdef NDEBUG
        if (IPC::IsInstanceAlreadyRunning())
            open_window = false;
        else
#endif
            open_window = true;
    }

    if (mode == MODE_CUSTOMSCREEN)
        open_window = true;

    if (open_window)
    {
        good_to_go = window.setup(this);
        init_audio();
        root = nullptr;
    }
    else
    {
        if (mode != MODE_NULL)
            good_to_go = true;
    }

    Log::Printf("Total Initialization Time: %fs\n", std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t1).count() / 1000000.0);
}

void Application::setup_preview_mode()
{
    // Load the chart group.
    const auto chart_group = otoworm::load_song_from_file(InFile);


    if (!chart_group || chart_group->charts.empty())
    {
        Log::Printf("File %ls could not be loaded for preview. (ptr %d/chartcnt %d)\n", InFile.c_str(), (long long int)chart_group.get(), chart_group ? chart_group->charts.size() : 0);
        return;
    }

    GameState::get_instance().set_selected_chart_group(chart_group);
    // Create loading screen and gameplay screen.
    auto game = std::make_shared<ScreenGameplay>();
    auto LoadScreen = std::make_shared<ScreenLoading>(game);

    // Set them up.
	chart_group->path = std::filesystem::absolute(InFile.parent_path());

	GameState::get_instance().get_parameters(0)->Auto = Auto;
    game->initialize(chart_group);
    LoadScreen->Init();

    root = LoadScreen;
}

bool Application::poll_ipc()
{
    IPC::Message msg = IPC::PopMessageFromQueue();
    switch (msg.message_class)
    {
    case IPC::Message::MSG_STARTFROMMEASURE:
        measure_ = msg.param;
        InFile = std::string(msg.Path);
        root->Close();
		root = nullptr;

        setup_preview_mode();

        return true;
    case IPC::Message::MSG_STOP:
        root->Close();
        return true;
    case IPC::Message::MSG_NULL:
    default:
        return false;
    }
}


void Application::run()
{
    const double current_time = window.get_current_time();
    bool run_loop = true;

    if (!good_to_go)
        return;

    if (mode == MODE_PLAY)
    {
        const auto scr = std::make_shared<ScreenMainMenu>();
        scr->Init();
        root = scr;
    }
    else if (mode == MODE_VSRGPREVIEW)
    {
        if (IPC::IsInstanceAlreadyRunning())
        {
            // So okay then, we'll send a message telling the existing process to restart with this file, at this time.
            IPC::Message msg;
            msg.message_class = IPC::Message::MSG_STARTFROMMEASURE;
            msg.param = measure_;
            strncpy(msg.Path, otoworm::locale::wstring_to_utf8(InFile.wstring()).c_str(), 256);

            IPC::SendMessageToQueue(&msg);
            run_loop = false;
        }
        else
        {
            setup_preview_mode();

            if (!root)
                return;

            // Set up the message queue. We need this if we're in preview mode to be able to control raindrop from the command line.
            IPC::SetupMessageQueue();
        }
    }
    else if (mode == MODE_GENSONGCACHE)
    {
        Log::Printf("Generating cache...\n");
        GameState::get_instance().initialize();
        SongWheel::get_instance().initialize(GameState::get_instance().get_song_database());
        SongWheel::get_instance().join_loading_thread();

        run_loop = false;
    }
    else if (mode == MODE_STOPPREVIEW)
    {
        if (IPC::IsInstanceAlreadyRunning())
        {
            // So okay then, we'll send a message telling the existing process to restart with this file, at this time.
            IPC::Message msg;
            msg.message_class = IPC::Message::MSG_STOP;

            IPC::SendMessageToQueue(&msg);
        }

        run_loop = false;
    }
    else if (mode == MODE_CUSTOMSCREEN)
    {
        Log::Printf("Initializing custom, ad-hoc screen...\n");
		const auto s = otoworm::locale::wstring_to_utf8(InFile.wstring());
        const auto scr = std::make_shared<ScreenCustom>(GameState::get_instance().get_skin_file(s));
        root = scr;
	}
	else if (mode == MODE_GENFONTCACHE)
	{
		Log::Printf("Generating font cache for provided file...\n");
		TruetypeFont::GenerateFontCache(InFontTextFile, InFile);
		run_loop = false;
	}

    Log::Printf("Time: %fs\n", window.get_current_time() - current_time);

    if (!run_loop)
        return;

    TextureCollection::upload_and_reload_textures();
	GameState::get_instance().set_root_screen(root);

    oldTime = window.get_current_time();
    while (root->IsScreenRunning() && !window.should_close_window())
    {
        const double new_time = window.get_current_time();
        const double delta = new_time - oldTime;
        TextureCollection::upload_and_reload_textures();

		window.run_input();

        window.clear_window();

        if (mode == MODE_VSRGPREVIEW) // Run IPC Message Queue Querying.
            if (poll_ipc()) continue;

        root->update(delta);

        update_mixer();
        window.swap_buffers();
		window.update_fullscreen();

        oldTime = new_time;
    }
}

void Application::on_input(const int32_t key, const bool is_pressed, const bool is_mouse_input) const {
	if (BindingsManager::translate_key(key) == KT_ReloadCFG && is_pressed)
		Configuration::Reload();

    root->on_input(key, is_pressed, is_mouse_input);
}

void Application::on_scroll_input(const double x_off, const double y_off) const {
    root->on_scroll_input(x_off, y_off);
}

void Application::close()
{
    if (root)
    {
        root->cleanup();
		root = nullptr;
    }

    window.cleanup();
    Configuration::cleanup();
}

void Application::on_text_input(unsigned cp) const {
    root->on_text_input(cp);
}
