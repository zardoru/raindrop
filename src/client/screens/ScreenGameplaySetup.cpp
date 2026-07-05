#include <future>
#include <queue>

#include <utility>
#include <glm.h>
#include <rmath.h>

#include <game/ScoreKeeper.h>
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
#include <text_and_file_util.h>
#include "../game/PlayerContext.h"
#include "ScreenGameplay.h"

#include "LuaManager.h"
// #include <LuaBridge/LuaBridge.h>
#include <algorithm>
#include <ranges>

#include "../game/Noteskin.h"
#include "Line.h"



#include "../structure/Configuration.h"

CfgVar disable_bga("DisableBGA");


ScreenGameplay::ScreenGameplay() : Screen("ScreenGameplay7K") {
    time_ = {};
    time_.old_stream = NAN;
    music_ = nullptr;

    stage_failure_triggered_ = false;
    song_pass_triggered_ = false;

    active_ = false;

    start_measure_ = -1;

    // Don't play unless everything goes right (later checks)
    load_successful_ = false;
}

void ScreenGameplay::cleanup() {
    if (music_) {
        music_->stop();
    }

    for (auto& k : keysounds_) {
        for (auto &s: k.second)
            GetMixer()->RemoveSample(s.get());
    }
}

void ScreenGameplay::jump_to_measure(const uint32_t measure) {
    const double mt = players_[0]->get_chart_state().get_time_at_measure(measure);
    const double wt = players_[0]->get_chart_state().real_to_warped_time(mt);
    for (const auto &player : players_) {
        player->set_unwarped_time(mt);
    }

    // We use P1's BGM events in every case.
    // Remove non-played objects
    while (!bgm_events_.empty() && bgm_events_.front() <= wt)
        bgm_events_.pop();

    time_.stream = mt;

    if (music_) {
        Log::Printf("ScreenGameplay7K: Setting player to time %f.\n", mt);
        time_.old_stream = NAN;
        music_->seek_time(time_.stream);
    }

    active_ = true;
}

void ScreenGameplay::initialize(std::shared_ptr<otoworm::ChartGroup> chart_group) {
    my_chart_group_ = std::move(chart_group);
    start_active_ = false;

    for (auto i = 0; i < GameState::get_instance().get_player_count(); i++) {
        players_.push_back(std::make_unique<PlayerContext>(i, *GameState::get_instance().get_parameters(i)));
        GameState::get_instance().set_player_context(players_[i].get(), i);
        players_[i]->play_keysound = [this](auto && PH1) { play_keysound(std::forward<decltype(PH1)>(PH1)); };
    }
}

bool ScreenGameplay::load_chart_data() {
    uint8_t index = 0;
    // The song is the same for _everyone_, so...
    bool preloaded = true;
    if (!my_chart_group_)
        preloaded = false;
    else {
        for (const auto &chart : my_chart_group_->charts) {
            if (!chart || !chart->transient) preloaded = false;
        }
    }

    if (!preloaded) {
        // The difficulty details are destroyed; which means we should load this from its original file.
        SongLoader Loader(GameState::get_instance().get_song_database());
        std::filesystem::path fn;

        Log::Printf("Loading Chart...");
        const auto loaded_chart_group = Loader.LoadFromMeta(
                my_chart_group_ ? my_chart_group_->id : -1,
                GameState::get_instance().get_chart_shared(0),
                fn,
                index);

        if (loaded_chart_group == nullptr) {
            Log::Printf("Failure to load chart. (Filename: %s)\n", otoworm::locale::wstring_to_utf8(fn.wstring()).c_str());
            return false;
        }

        my_chart_group_ = loaded_chart_group;
        loaded_chart_group_ = my_chart_group_;
        GameState::get_instance().set_selected_chart_group(my_chart_group_);

        /*
            At this point, LoadedChartGroup owns the loaded otoworm data.
        */
    }


    bga_ = BackgroundAnimation::CreateBGAFromChartGroup(index, my_chart_group_, this);

    return true;
}

CfgVar DebugLoadAudio("DebugAudioLoad", "Debug");

bool ScreenGameplay::load_song_audio() {
    if (const CfgVar skip_load_audio("SkipAudioLoad", "Debug"); skip_load_audio) {
        Log::LogPrintf("SkipAudioLoad is ON, skipping audio load.\n");
        return true;
    }

    const auto rate = GameState::get_instance().get_parameters(0)->Rate;

    Log::LogPrintf("Chart audio: Load start!\n");
    auto &ps = players_[0]->get_chart_state();
    const auto sound_list = ps.get_sound_list();
    if (!music_) {
        bool attempt_music_load = true;
        music_ = std::make_unique<AudioStream>(GetMixer());
        music_->set_pitch(rate);


        if (my_chart_group_->song_filename.empty())
            attempt_music_load = false;

        const auto s = my_chart_group_->path / my_chart_group_->song_filename;

        if (attempt_music_load)
            Log::LogPrintf("Chart Audio: Attempt to load \"%ls\"...\n", s.wstring().c_str());

        if (std::filesystem::exists(s)
            && attempt_music_load
            && music_->open(s)) {
            Log::Printf("Stream for %s succesfully opened.\n", my_chart_group_->song_filename.c_str());
        } else {
            if (!players_[0]->get_chart_state().is_virtual()) {
                // Caveat: Try to autodetect an mp3/ogg file.
                const auto sng_dir = my_chart_group_->path;

                if (DebugLoadAudio)
                    Log::LogPrintf("Attempt to autodetect audio from directory...\n");

                // Open the first MP3 and OGG file in the directory
                for (const auto& i : std::filesystem::directory_iterator(sng_dir)) {
                    if (auto extension = i.path().extension(); extension == ".mp3" || extension == ".ogg")
                        if (music_->open(i.path())) {
                            if (DebugLoadAudio)
                                Log::LogPrintf("Got audio on path... %S\n", i.path().wstring().c_str());

                            if (sound_list.empty())
                                return true;
                        }
                }

                // Quit; couldn't find audio for a chart that requires it.
                music_ = nullptr;

                // don't abort load if we have keysounds
                if (sound_list.empty()) {
                    Log::Printf("Unable to load song (Path: %ls)\n", my_chart_group_->song_filename.wstring().c_str());
                    return false;
                }
            }
        }
    }

    // Load samples.
    if (my_chart_group_->song_filename.extension() == ".ojm") {
        Log::Printf("O2JAM: Loading OJM.\n");
        ojm_audio_ = std::make_unique<AudioSourceOJM>(this);
        ojm_audio_->SetPitch(rate);
        ojm_audio_->open(my_chart_group_->path / my_chart_group_->song_filename);

        for (int i = 1; i <= 2000; i++) {
            std::shared_ptr<AudioSample> Snd = ojm_audio_->GetFromIndex(i);

            if (Snd != nullptr)
                keysounds_[i].push_back(Snd);
        }
    } else if (!sound_list.empty()) {
        Log::LogPrintf("Chart Audio: Loading samples... ");
        load_samples();

    } else if (ps.is_bmson()) {
        Log::Printf("BMSON: Loading Slice data...\n");
        load_bmson();
    }


    return true;
}

void ScreenGameplay::load_samples() {
    const auto rate = GameState::get_instance().get_parameters(0)->Rate;
    auto &ps = players_[0]->get_chart_state();
    const auto sound_list = ps.get_sound_list();

    const auto start = std::chrono::high_resolution_clock::now();
    for (auto & i : sound_list) {
        auto ks = std::make_shared<AudioSample>(GetMixer());

        ks->set_pitch(rate);
        std::filesystem::path rfd = i.second;
        std::filesystem::path afd = my_chart_group_->path / rfd;

        if (DebugLoadAudio) {
            Log::LogPrintf("Attempt to load sound %S (%i)...\n", afd.wstring().c_str(), i.first);

            if (!std::filesystem::exists(afd)) {
                Log::LogPrintf("\t... but it does not exist.\n", afd.wstring().c_str());
            }
        }

        ks->open(afd, false);
        keysounds_[i.first].push_back(ks);
        CheckInterruption();
    }

    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - start).count();
    Log::LogPrintf("Keysounds loading in the background. Taken %I64dms to finish.", dur);
}

void ScreenGameplay::load_bmson() {
    const auto Rate = GameState::get_instance().get_parameters(0)->Rate;
    auto &ps = players_[0]->get_chart_state();
    const auto dir = my_chart_group_->path;
    std::map<int, AudioSample> audio;
    std::mutex audio_data_mutex;
    std::mutex keysound_data_mutex;
    const auto &slicedata = ps.get_bmson_slice_data();

    // do bmson loading - threaded slicing!
    std::vector<std::future<void>> threads;
    std::atomic<int> obj_cnt(0);

    const auto load_start_time = std::chrono::high_resolution_clock::now();
    for (const auto& audio_file : slicedata.audio_files) {
        auto fn = [&](const std::pair<int, std::string>& audio_file) {
            const auto path = (dir / audio_file.second);
            AudioSample *p;

            // Audio load (parallelly?)
            audio_data_mutex.lock();
            p = &audio[audio_file.first];
            audio_data_mutex.unlock();

            p->set_pitch(Rate);

            // Verbose, but not as verbose as other languages.

            Log::LogPrintf("BMSON: Load sound %s AUDIO ID: %d\n", otoworm::locale::wstring_to_utf8(path.wstring()).c_str(),
                           audio_file.first);
            const auto t = std::chrono::high_resolution_clock::now();

            // Open file
            if (!p->open(path))
                throw std::runtime_error(otoworm::util::format("Unable to load %s.", audio_file.second.c_str()));


            // Done. Slicing
            const auto d = std::chrono::high_resolution_clock::now() - t;
            const auto cd = std::chrono::duration_cast<std::chrono::milliseconds>(d);

            Log::LogPrintf("BMSON: Slicing %d. Read in %I64dms...\n", audio_file.first, cd.count());
            const auto t2 = std::chrono::high_resolution_clock::now();
            // Slice file
            // For each wav/sound index on the list
            for (const auto& wav : slicedata.slices) {
                // for each slice on this index (mix-note)
                for (const auto sound : wav.second) {
                    // This is a slice of our available big boy.
                    if (sound.first == audio_file.first) {
                        p->slice(sound.second.start, sound.second.end);
                        keysound_data_mutex.lock();
                        keysounds_[wav.first].push_back(p->CopySlice());
                        keysound_data_mutex.unlock();
                    }
                    // obj_cnt++;
                }
            }

            const auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - t2);
            Log::LogPrintf("BMSON: Sliced %d in %I64dms...\n", audio_file.first, d2.count());
        };

        threads.push_back(std::async(std::launch::async, fn, audio_file));
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
    for (auto &ks : keysounds_) {
        ks.second.shrink_to_fit();
    }

    const auto load_dur = std::chrono::high_resolution_clock::now() - load_start_time;
    const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(load_dur).count();
    Log::LogPrintf("BMSON: Loaded slices in %I64d\n", dur);
    //Log::Printf("BMSON: Generated %d sound objects.\n", wavs);
}

bool ScreenGameplay::process_song() {
    TimeError.AudioDrift = 0;

    double SpeedConstant = 0; // Unless set, assume we're using speed changes

    const int apply_drift_virtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
    const int apply_drift_decoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");

    auto chart = GameState::get_instance().get_chart_shared(0);
    if (!chart && my_chart_group_ && !my_chart_group_->charts.empty())
        chart = my_chart_group_->charts.front();

    if (!chart) {
        Log::Printf("Error loading chart: no otoworm chart for gameplay.\n");
        return false;
    }

    if (((apply_drift_virtual && chart->has_no_audio_stream) ||  // We want to apply it to a keysounded file and it's virtual
         (apply_drift_decoder &&
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

    for (auto &&p : players_) {
        auto player_chart = GameState::get_instance().get_chart_shared(p->get_player_number());

        if (!player_chart && my_chart_group_ && !my_chart_group_->charts.empty())
            player_chart = my_chart_group_->charts.front();

        if (!player_chart) continue;

        p->set_playable_data(player_chart, TimeError.AudioDrift);

        p->init();
        GameState::get_instance().set_scorekeeper7_k(
                p->get_score_keeper_shared(),
                p->get_player_number()
        );

        if (!p->get_chart_state().has_timing_data()) {
            Log::Printf("Error loading chart: No timing data for player %d.\n", p->get_player_number());
            return false;
        }
    }

    auto bgm0 = players_[0]->create_autoplay_sound_list();

    std::ranges::sort(bgm0.begin(), bgm0.end());
    for (auto &s : bgm0)
        bgm_events_.push(s);

    for (auto &&p : players_) {
        time_.waiting = std::max(time_.waiting, p->get_waiting_time());
    }

    return true;
}

bool ScreenGameplay::load_bga() const {
    if (!disable_bga) {
        try {
            bga_->Load();
            scene_->add_target(bga_.get(), true);
        }
        catch (std::exception &e) {
            Log::LogPrintf("Failure to load BGA: %s.\n", e.what());
        }
    }

    return true;
}

void ScreenGameplay::load_resources() {
    const auto miss_snd_file = Configuration::GetSkinSound("Miss");
    const auto fail_snd_file = Configuration::GetSkinSound("Fail");

    miss_snd_.open(miss_snd_file);
    fail_snd_.open(fail_snd_file);

    // For some reason, it tries to load song audio before chartdata is assigned from meta???
    // so I have to be a bit more explicit about the order, then
    if (!load_chart_data()) {
        load_successful_ = false;
        return;
    }

    if (!process_song()) {
        load_successful_ = false;
        return;
    }

    if (!load_song_audio()) {
        load_successful_ = false;
        return;
    }

    if (!load_bga()) {
        load_successful_ = false;
        return;
    }

    setup_scripts(scene_->get_script_manager());
    TimeError.ToleranceMS = CfgVar("ErrorTolerance");

    if (TimeError.ToleranceMS <= 0)
        TimeError.ToleranceMS = 16; // ms

    register_script_values();

    scene_->preload(GameState::get_instance().get_skin_file("screengameplay7k.lua"), "Preload");
    Log::Printf("Done.\n");

    if (start_measure_ > 0)
        jump_to_measure(start_measure_);

    start_active_ = start_active_ || (Configuration::GetSkinConfigf("InmediateActivation") == 1);


    // We're done with the data stored in the difficulties that aren't the one we're using. Clear it up.
    for (auto &chart : my_chart_group_->charts) {
        if (chart != GameState::get_instance().get_chart_shared(0))
            chart->reset_transient();
    }

    if (const CfgVar await("AwaitKeysoundLoad"); await) {
        const auto st = std::chrono::high_resolution_clock::now();
        Log::LogPrintf("Awaiting for keysounds to finish loading...\n");
        for (auto &val: keysounds_ | std::views::values) {
            for (const auto &snd : val) {
                snd->await_load();
                // GetMixer()->AddSample(snd.get());
            }
        }

        const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - st).count();
        Log::LogPrintf("Done. Taken %I64dms to finish.\n", dur);
    }

    load_successful_ = true;
}

void ScreenGameplay::post_load_initialization() {

    if (!load_successful_) // Failure to load something important?
    {
        is_active_ = false;
        return;
    }

    play_reactive_sounds_ = (!Configuration::GetConfigf("DisableHitsounds"));

    scene_->get_image_list()->ForceFetch();
    bga_->Validate();

    for (const auto &p : players_) {
        p->validate();

        // TODO: parameter types/names
        p->on_hit = [this](auto && PH1, auto && PH2, auto && PH3, auto && PH4, auto && PH5, auto && PH6) {
            on_player_hit(std::forward<decltype(PH1)>(PH1),
                        std::forward<decltype(PH2)>(PH2),
                        std::forward<decltype(PH3)>(PH3),
                        std::forward<decltype(PH4)>(PH4),
                        std::forward<decltype(PH5)>(PH5),
                        std::forward<decltype(PH6)>(PH6));
        };

        p->on_miss = [this](double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int playerNumber) {
            on_player_miss(dt, lane, hold, dontbreakcombo, earlymiss, playerNumber);
        };

        p->on_gear_key_event = [this](auto && PH1, auto && PH2, auto && PH3) {
            on_player_gear_key_event(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3));
        };
    }


    scene_->initialize("", false);
    is_active_ = true;
}
