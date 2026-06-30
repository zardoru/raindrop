#include <future>
#include <queue>

#include <utility>
#include <glm.h>
#include <rmath.h>

#include <game/Song.h>
#include <game/ScoreKeeper7K.h>
#include <game/NoteTransformations.h>

#include <Audio.h>
#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>

#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "Logging.h"
#include "../songdb/SongLoader.h"
#include "../structure/Screen.h"
#include "GameWindow.h"
#include "ImageList.h"

#include "../structure/SceneEnvironment.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "../bga/BackgroundAnimation.h"

#include <ProcessedChart.h>
#include <game/VSRGMechanics.h>
#include <TextAndFileUtil.h>
#include "../game/PlayerContext.h"
#include "ScreenGameplay7K.h"

#include "LuaManager.h"
// #include <LuaBridge/LuaBridge.h>
#include "../game/Noteskin.h"
#include "Line.h"



#include "../structure/Configuration.h"

CfgVar DisableBGA("DisableBGA");


bool isChartLoaded;

ScreenGameplay::ScreenGameplay() : Screen("ScreenGameplay7K") {
    Time = {};
    Time.OldStream = NAN;
    Music = nullptr;

    StageFailureTriggered = false;
    SongPassTriggered = false;

    Active = false;

    StartMeasure = -1;

    // Don't play unless everything goes right (later checks)
    DoPlay = false;

    isChartLoaded = false;
}

void ScreenGameplay::Cleanup() {
    if (Music) {
        Music->stop();
    }

    for (auto& k : Keysounds) {
        for (auto &s: k.second)
            GetMixer()->RemoveSample(s.get());
    }
}

void ScreenGameplay::AssignMeasure(uint32_t Measure) {
    double mt = Players[0]->get_chart_state().get_time_at_measure(Measure);
    double wt = Players[0]->get_chart_state().real_to_warped_time(mt);
    for (auto &player : Players) {
        player->SetUnwarpedTime(mt);
    }

    // We use P1's BGM events in every case.
    // Remove non-played objects
    while (!BGMEvents.empty() && BGMEvents.front() <= wt)
        BGMEvents.pop();

    Time.Stream = mt;

    if (Music) {
        Log::Printf("ScreenGameplay7K: Setting player to time %f.\n", mt);
        Time.OldStream = NAN;
        Music->seek_time(Time.Stream);
    }

    Active = true;
}

void ScreenGameplay::Init(std::shared_ptr<otoworm::ChartGroup> chart_group) {
    MyChartGroup = std::move(chart_group);
    ForceActivation = false;

    for (auto i = 0; i < GameState::get_instance().get_player_count(); i++) {
        Players.push_back(std::make_unique<PlayerContext>(i, *GameState::get_instance().get_parameters(i)));
        GameState::get_instance().set_player_context(Players[i].get(), i);
        Players[i]->PlayKeysound = [this](auto && PH1) { PlayKeysound(std::forward<decltype(PH1)>(PH1)); };
    }
}

bool ScreenGameplay::LoadChartData() {
    uint8_t index = 0;
    // The song is the same for _everyone_, so...
    bool preloaded = true;
    if (!MyChartGroup)
        preloaded = false;
    else {
        for (auto &chart : MyChartGroup->charts) {
            if (!chart || !chart->transient) preloaded = false;
        }
    }

    if (!preloaded) {
        // The difficulty details are destroyed; which means we should load this from its original file.
        SongLoader Loader(GameState::get_instance().get_song_database());
        std::filesystem::path FN;

        Log::Printf("Loading Chart...");
        auto loaded_legacy_song = Loader.LoadFromMeta(
                MyChartGroup ? MyChartGroup->id : -1,
                GameState::get_instance().get_chart_shared(0),
                FN,
                index);

        if (loaded_legacy_song == nullptr) {
            Log::Printf("Failure to load chart. (Filename: %s)\n", Conversion::ToU8(FN.wstring()).c_str());
            return false;
        }

        MyChartGroup = loaded_legacy_song->OtoChartGroup;
        LoadedChartGroup = MyChartGroup;
        GameState::get_instance().set_selected_chart_group(MyChartGroup);

        /*
            At this point, LoadedChartGroup owns the loaded otoworm data.
        */
    }


    BGA = BackgroundAnimation::CreateBGAFromChartGroup(index, MyChartGroup, this);

    isChartLoaded = true;
    return true;
}

CfgVar DebugLoadAudio("DebugAudioLoad", "Debug");

bool ScreenGameplay::LoadSongAudio() {
    CfgVar SkipLoadAudio("SkipAudioLoad", "Debug");

    if (SkipLoadAudio) {
        Log::LogPrintf("SkipAudioLoad is ON, skipping audio load.\n");
        return true;
    }

    if (!isChartLoaded) {
        Log::LogPrintf("what the hell is going on");
    }

    auto Rate = GameState::get_instance().get_parameters(0)->Rate;

    Log::LogPrintf("Chart audio: Load start!\n");
    auto &ps = Players[0]->get_chart_state();
    auto SoundList = ps.get_sound_list();
    if (!Music) {
        bool attempt_music_load = true;
        Music = std::make_unique<AudioStream>(GetMixer());
        Music->set_pitch(Rate);


        if (MyChartGroup->song_filename.empty())
            attempt_music_load = false;

        auto s = MyChartGroup->path / MyChartGroup->song_filename;

        if (attempt_music_load)
            Log::LogPrintf("Chart Audio: Attempt to load \"%ls\"...\n", s.wstring().c_str());

        if (std::filesystem::exists(s)
            && attempt_music_load
            && Music->open(s)) {
            Log::Printf("Stream for %s succesfully opened.\n", MyChartGroup->song_filename.c_str());
        } else {
            if (!Players[0]->get_chart_state().is_virtual()) {
                // Caveat: Try to autodetect an mp3/ogg file.
                auto SngDir = MyChartGroup->path;

                if (DebugLoadAudio)
                    Log::LogPrintf("Attempt to autodetect audio from directory...\n");

                // Open the first MP3 and OGG file in the directory
                for (const auto& i : std::filesystem::directory_iterator(SngDir)) {
                    auto extension = i.path().extension();
                    if (extension == ".mp3" || extension == ".ogg")
                        if (Music->open(i.path())) {
                            if (DebugLoadAudio)
                                Log::LogPrintf("Got audio on path... %S\n", i.path().wstring().c_str());

                            if (SoundList.empty())
                                return true;
                        }
                }

                // Quit; couldn't find audio for a chart that requires it.
                Music = nullptr;

                // don't abort load if we have keysounds
                if (SoundList.empty()) {
                    Log::Printf("Unable to load song (Path: %ls)\n", MyChartGroup->song_filename.wstring().c_str());
                    return false;
                }
            }
        }
    }

    // Load samples.
    if (MyChartGroup->song_filename.extension() == ".ojm") {
        Log::Printf("O2JAM: Loading OJM.\n");
        OJMAudio = std::make_unique<AudioSourceOJM>(this);
        OJMAudio->SetPitch(Rate);
        OJMAudio->open(MyChartGroup->path / MyChartGroup->song_filename);

        for (int i = 1; i <= 2000; i++) {
            std::shared_ptr<AudioSample> Snd = OJMAudio->GetFromIndex(i);

            if (Snd != nullptr)
                Keysounds[i].push_back(Snd);
        }
    } else if (!SoundList.empty()) {
        Log::LogPrintf("Chart Audio: Loading samples... ");
        LoadSamples();

    } else if (ps.is_bmson()) {
        Log::Printf("BMSON: Loading Slice data...\n");
        LoadBmson();
    }


    return true;
}

void ScreenGameplay::LoadSamples() {
    auto Rate = GameState::get_instance().get_parameters(0)->Rate;
    auto &ps = Players[0]->get_chart_state();
    auto SoundList = ps.get_sound_list();

    auto start = std::chrono::high_resolution_clock::now();
    for (auto & i : SoundList) {
        auto ks = std::make_shared<AudioSample>(GetMixer());

        ks->set_pitch(Rate);
        std::filesystem::path rfd = i.second;
        std::filesystem::path afd = MyChartGroup->path / rfd;

        if (DebugLoadAudio) {
            Log::LogPrintf("Attempt to load sound %S (%i)...\n", afd.wstring().c_str(), i.first);

            if (!std::filesystem::exists(afd)) {
                Log::LogPrintf("\t... but it does not exist.\n", afd.wstring().c_str());
            }
        }

        ks->open(afd, false);
        Keysounds[i.first].push_back(ks);
        CheckInterruption();
    }

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
    Log::LogPrintf("Keysounds loading in the background. Taken %I64dms to finish.", dur);
}

void ScreenGameplay::LoadBmson() {
    auto Rate = GameState::get_instance().get_parameters(0)->Rate;
    auto &ps = Players[0]->get_chart_state();
    auto dir = MyChartGroup->path;
    std::map<int, AudioSample> audio;
    std::mutex audio_data_mutex;
    std::mutex keysound_data_mutex;
    const auto &slicedata = ps.get_bmson_slice_data();

    // do bmson loading - threaded slicing!
    std::vector<std::future<void>> threads;
    std::atomic<int> obj_cnt(0);

    auto load_start_time = std::chrono::high_resolution_clock::now();
    for (auto audiofile : slicedata.audio_files) {
        auto fn = [&](const std::pair<int, std::string>& audiofile) {
            auto path = (dir / audiofile.second);
            AudioSample *p;

            // Audio load (parallelly?)
            audio_data_mutex.lock();
            p = &audio[audiofile.first];
            audio_data_mutex.unlock();

            p->set_pitch(Rate);

            // Verbose, but not as verbose as other languages.

            Log::LogPrintf("BMSON: Load sound %s AUDIO ID: %d\n", Conversion::ToU8(path.wstring()).c_str(),
                           audiofile.first);
            auto t = std::chrono::high_resolution_clock::now();

            // Open file
            if (!p->open(path))
                throw std::runtime_error(Utility::Format("Unable to load %s.", audiofile.second.c_str()));


            // Done. Slicing
            auto d = std::chrono::high_resolution_clock::now() - t;
            auto cd = std::chrono::duration_cast<std::chrono::milliseconds>(d);

            Log::LogPrintf("BMSON: Slicing %d. Read in %I64dms...\n", audiofile.first, cd.count());
            auto t2 = std::chrono::high_resolution_clock::now();
            // Slice file
            // For each wav/sound index on the list
            for (const auto& wav : slicedata.slices) {
                // for each slice on this index (mix-note)
                for (auto sound : wav.second) {
                    // This is a slice of our available big boy.
                    if (sound.first == audiofile.first) {
                        p->slice(sound.second.start, sound.second.end);
                        keysound_data_mutex.lock();
                        Keysounds[wav.first].push_back(p->CopySlice());
                        keysound_data_mutex.unlock();
                    }
                    // obj_cnt++;
                }
            }

            auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - t2);
            Log::LogPrintf("BMSON: Sliced %d in %I64dms...\n", audiofile.first, d2.count());
        };

        threads.push_back(std::async(std::launch::async, fn, audiofile));
    }

    bool go_on = true;
    while (go_on) {
        bool one_thread_is_not_finished = false;
        for (auto &thread : threads) {
            if (thread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
                one_thread_is_not_finished = true;
            }
        }

        go_on = one_thread_is_not_finished;
    }

    // Get rid of that extra space
    for (auto &ks : Keysounds) {
        ks.second.shrink_to_fit();
    }

    auto load_dur = std::chrono::high_resolution_clock::now() - load_start_time;
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(load_dur).count();
    Log::LogPrintf("BMSON: Loaded slices in %I64d\n", dur);
    //Log::Printf("BMSON: Generated %d sound objects.\n", wavs);
}

bool ScreenGameplay::ProcessSong() {
    TimeError.AudioDrift = 0;

    double SpeedConstant = 0; // Unless set, assume we're using speed changes

    int ApplyDriftVirtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
    int ApplyDriftDecoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");

    auto chart = GameState::get_instance().get_chart_shared(0);
    if (!chart && MyChartGroup && !MyChartGroup->charts.empty())
        chart = MyChartGroup->charts.front();

    if (!chart) {
        Log::Printf("Error loading chart: no otoworm chart for gameplay.\n");
        return false;
    }

    if (((ApplyDriftVirtual && chart->has_no_audio_stream) ||  // We want to apply it to a keysounded file and it's virtual
         (ApplyDriftDecoder &&
          chart->has_no_audio_stream))) // or we want to apply it to a non-keysounded file and it's not virtual
        TimeError.AudioDrift += MixerGetLatency();

    TimeError.AudioDrift += Configuration::GetConfigf("Offset7K");

    if (chart->has_no_audio_stream)
        TimeError.AudioDrift += Configuration::GetConfigf("OffsetKeysounded");
    else
        TimeError.AudioDrift += Configuration::GetConfigf("OffsetNonKeysounded");

    Log::Logf("TimeCompensation: %f (Latency: %f / Offset: %f)\n", TimeError.AudioDrift, MixerGetLatency(),
              chart->offset);

    Log::Printf("Processing song... ");

    for (auto &&p : Players) {
        auto player_chart = GameState::get_instance().get_chart_shared(p->get_player_number());

        if (!player_chart && MyChartGroup && !MyChartGroup->charts.empty())
            player_chart = MyChartGroup->charts.front();

        if (!player_chart) continue;

        p->set_playable_data(player_chart, TimeError.AudioDrift);

        p->init();
        GameState::get_instance().set_scorekeeper7_k(
                p->GetScoreKeeperShared(),
                p->get_player_number()
        );

        if (!p->get_chart_state().has_timing_data()) {
            Log::Printf("Error loading chart: No timing data for player %d.\n", p->get_player_number());
            return false;
        }
    }

    auto bgm0 = Players[0]->GetBgmData();

    sort(bgm0.begin(), bgm0.end());
    for (auto &s : bgm0)
        BGMEvents.push(s);

    for (auto &&p : Players) {
        Time.Waiting = std::max(Time.Waiting, p->get_waiting_time());
    }

    return true;
}

bool ScreenGameplay::LoadBGA() const {
    if (!DisableBGA) {
        try {
            BGA->Load();
            Animations->AddTarget(BGA.get(), true);
        }
        catch (std::exception &e) {
            Log::LogPrintf("Failure to load BGA: %s.\n", e.what());
        }
    }

    return true;
}

void ScreenGameplay::LoadResources() {
    auto MissSndFile = Configuration::GetSkinSound("Miss");
    auto FailSndFile = Configuration::GetSkinSound("Fail");

    MissSnd.open(MissSndFile);
    FailSnd.open(FailSndFile);

    // For some reason, it tries to load song audio before chartdata is assigned from meta???
    // so I have to be a bit more explicit about the order, then
    if (!LoadChartData()) {
        DoPlay = false;
        return;
    }

    if (!ProcessSong()) {
        DoPlay = false;
        return;
    }

    if (!LoadSongAudio()) {
        DoPlay = false;
        return;
    }

    if (!LoadBGA()) {
        DoPlay = false;
        return;
    }

    SetupLua(Animations->GetEnv());
    TimeError.ToleranceMS = CfgVar("ErrorTolerance");

    if (TimeError.ToleranceMS <= 0)
        TimeError.ToleranceMS = 16; // ms

    SetupScriptConstants();

    Animations->Preload(GameState::get_instance().get_skin_file("screengameplay7k.lua"), "Preload");
    Log::Printf("Done.\n");

    if (StartMeasure > 0)
        AssignMeasure(StartMeasure);

    ForceActivation = ForceActivation || (Configuration::GetSkinConfigf("InmediateActivation") == 1);


    // We're done with the data stored in the difficulties that aren't the one we're using. Clear it up.
    for (auto &chart : MyChartGroup->charts) {
        if (chart != GameState::get_instance().get_chart_shared(0))
            chart->reset_transient();
    }

    CfgVar await("AwaitKeysoundLoad");
    if (await) {
        auto st = std::chrono::high_resolution_clock::now();
        Log::LogPrintf("Awaiting for keysounds to finish loading...\n");
        for (auto &vec : Keysounds) {
            for (auto &snd : vec.second) {
                snd->await_load();
                // GetMixer()->AddSample(snd.get());
            }
        }

        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - st).count();
        Log::LogPrintf("Done. Taken %I64dms to finish.\n", dur);
    }

    DoPlay = true;
}

void ScreenGameplay::InitializeResources() {

    if (!DoPlay) // Failure to load something important?
    {
        Running = false;
        return;
    }

    PlayReactiveSounds = (!(Configuration::GetConfigf("DisableHitsounds")));

    Animations->GetImageList()->ForceFetch();
    BGA->Validate();

    for (auto &p : Players) {
        p->validate();

        // TODO: parameter types/names
        p->OnHit = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4, auto && PH5, auto && PH6) {
            OnPlayerHit(std::forward<decltype(PH1)>(PH1),
                        std::forward<decltype(PH2)>(PH2),
                        std::forward<decltype(PH3)>(PH3),
                        std::forward<decltype(PH4)>(PH4),
                        std::forward<decltype(PH5)>(PH5),
                        std::forward<decltype(PH6)>(PH6));
        };

        p->OnMiss = [this](double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int playerNumber) {
            OnPlayerMiss(dt, lane, hold, dontbreakcombo, earlymiss, playerNumber);
        };

        p->OnGearKeyEvent = [this](auto && PH1, auto && PH2, auto && PH3) {
            OnPlayerGearKeyEvent(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3));
        };
    }


    Animations->Initialize("", false);
    Running = true;
}
