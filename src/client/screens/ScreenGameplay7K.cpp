#include <memory>
#include <filesystem>

#include <queue>
#include <future>

#include <Audio.h>
#include <sndio/Audiofile.h>
#include <sndio/AudioSourceOJM.h>

#include "Logging.h"
#include "../structure/Screen.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>
#include <rmath.h>
#include "../structure/SceneEnvironment.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include <ProcessedChart.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper.h>
#include "../game/PlayscreenParameters.h"
#include "../game/Noteskin.h"
#include "../game/PlayerContext.h"
#include "../bga/BackgroundAnimation.h"
#include "ScreenGameplay7K.h"

#include <math.h>
#include <ranges>

#include "ScreenEvaluation7K.h"

#include "../game/GameState.h"
#include "../game/Game.h"
#include "../structure/Configuration.h"

/// @themescript screengameplay7k.lua
void ScreenGameplay::activate() {
    /// Called once the song time starts advancing.
    // @callback OnActivateEvent
    if (!active_)
        scene_->trigger_event("OnActivateEvent");

    active_ = true;
}

bool ScreenGameplay::is_active() const {
    return active_;
}

otoworm::ChartGroup *ScreenGameplay::get_chart_group() const {
    return my_chart_group_.get();
}

void ScreenGameplay::play_keysound(int keysound) {
    auto fnd = keysounds_.find(keysound);

    if (fnd != keysounds_.end() && play_reactive_sounds_) {
        for (auto &&s: keysounds_[keysound]) {
            if (s) s->play();
        }
    }
}


// Called right after the scorekeeper and the engine's objects are initialized.
void ScreenGameplay::register_script_values() const {
    auto L = scene_->get_script_manager();
    luabridge::push(L->get_lua_state(), static_cast<Transformation *>(bga_.get()));
    /// The BGA's transform.
    // @autoinstance Background
    lua_setglobal(L->get_lua_state(), "Background");
}

// Called before the script is executed at all.
void ScreenGameplay::setup_scripts(LuaManager *Env) {
    /// Global Gamestate
    // @autoinstance Global
    GameState::get_instance().initialize_lua(Env->get_lua_state());
    PlayerContext::setup_script_context(Env);

    add_script_classes(Env);

    luabridge::push(Env->get_lua_state(), this);
    /// ScreenGameplay instance.
    // @autoinstance rd
    lua_setglobal(Env->get_lua_state(), "rd");
}


PlayerContext *ScreenGameplay::GetPlayerContext(int i) const {
    if (i >= 0 && i < players_.size())
        return players_[i].get();
    else
        return nullptr;
}

void ScreenGameplay::set_player_clip(int pn, AABB box) {
    playfield_clip_enabled_[pn] = true;
    playfield_clip_area_[pn] = box;
}

void ScreenGameplay::disable_player_clip(int pn) {
    playfield_clip_enabled_[pn] = false;
}

bool ScreenGameplay::HandleInput(int32_t key, bool isPressed, bool isMouseInput) {
    /*
    In here we should use the input arrangements depending on
    the amount of channels the current difficulty is using.
    Also potentially pausing and quitting the screen.
    Other than that most input can be safely ignored.
    */

    /* Handle nested screens. */
    if (Screen::HandleInput(key, isPressed, isMouseInput))
        return true;

    scene_->HandleInput(key, isPressed, isMouseInput);

    if (isPressed) {
        switch (BindingsManager::TranslateKey(key)) {
            case KT_Escape:
                if (song_pass_triggered_)
                    time_.success = -1;
                else
                    is_active_ = false;
                break;
            case KT_Enter:
                if (!active_)
                    activate();
                break;
            default:
                break;
        }

#ifndef NDEBUG
        if (key == 290) // f1
        {
            if (music_)
                music_->set_pitch(music_->get_pitch() - 0.2);
        }
        if (key == 291)
        {
            if (music_)
                music_->set_pitch(music_->get_pitch() + 0.2);
        }// f2
#endif

        if (BindingsManager::TranslateKey7K(key) != KT_Unknown) {
            for (auto &player : players_) {
                player->handle_lane_events(
                        BindingsManager::TranslateKey7K(key),
                        true,
                        time_.stream);
            }
        }
    } else {
        if (BindingsManager::TranslateKey7K(key) != KT_Unknown) {
            for (auto &player : players_) {
                player->handle_lane_events(
                        BindingsManager::TranslateKey7K(key),
                        false,
                        time_.stream);
            }
        }
    }

    return true;
}

void ScreenGameplay::run_auto_events() {
    if (!stage_failure_triggered_ && active_) {
        // Play BGM events.
        while (!bgm_events_.empty() && bgm_events_.front().time <= time_.stream) {
            for (auto &&s : keysounds_[bgm_events_.front().sound])
                if (s) {
                    double dt = time_.stream - bgm_events_.front().time;
                    if (dt < s->get_duration()) {
                        s->seek_time(dt);
                        s->play();
                    }
                }
            bgm_events_.pop();
        }
    }

    bga_->SetAnimationTime(time_.stream);
}

void ScreenGameplay::evaluate_stage_failure() {
    auto perform_stage_failure = [&]() {
        stage_failure_triggered_ = true;
        // ScoreKeeper->failStage();

        // go to evaluation screen, or back to song select depending on the skin
        GameState::get_instance().submit_score(0);

        // post-gameplay failure?
        if (!has_delayed_failure()) {
            fail_snd_.play();

            // We stop all audio..
            if (music_)
                music_->stop();

            for (auto &ks: keysounds_ | std::views::values)
                for (auto &&s : ks)
                    if (s)
                        s->stop();

            // run failure event

            /// If the player fails, this is called. 
            // Return time to wait before transitioning out of this screen. Clamped to [0,30]
            // @callback OnFailureEvent
            scene_->trigger_event("OnFailureEvent", 1);
            time_.failure = clamp(scene_->get_script_manager()->get_stack_f(), 0.0f, 30.0f);
        }
    };

    // Run failure first; make sure it has priority over checking whether it's a pass or not.
    if (all_players_failed() && !has_delayed_failure() && !stage_failure_triggered_)
        perform_stage_failure();

    // Okay then, so it's a pass?
    if (has_song_finished() && !stage_failure_triggered_) {
        if (!song_pass_triggered_) {
            // delayed failure check. 
            if (all_players_failed()) {
                perform_stage_failure(); // No, don't trigger SongPassTriggered. It wasn't a pass.
                return;
            }

            // do score submit
            GameState::get_instance().submit_score(0);

            song_pass_triggered_ = true; // Reached the end!

            /// If the player succeeds, this is called.
            // Returns time to exit screen. Clamped [0,30]
            // @callback OnSongFinishedEvent
            scene_->trigger_event("OnSongFinishedEvent", 1);
            time_.success = clamp(scene_->get_script_manager()->get_stack_f(), 1.0f, 30.0f);
        }
    }

    // Okay then, the song's done, and the success animation is done too. Time to evaluate.
    bool trigger_eval = false;
    if (time_.success < 0 && song_pass_triggered_) {
        trigger_eval = true;
    }

    if (stage_failure_triggered_) {
        time_.miss_layer = 10; // Infinite, for as long as it lasts.
        if (time_.failure <= 0) {
            if (Configuration::GetSkinConfigf("GoToSongSelectOnFailure") == 0) {
                trigger_eval = true;
            } else
                is_active_ = false;
        }
    }

    if (trigger_eval) {
        const auto screen_evaluation = std::make_shared<ScreenEvaluation>();
        screen_evaluation->Init(this);
        Next = screen_evaluation;
    }
}

bool ScreenGameplay::has_delayed_failure() const {
    for (auto &player : players_) {
        if (player->has_delayed_failure())
            return true;
    }

    return false;
}

bool ScreenGameplay::all_players_failed() const {
    for (auto &player : players_) {
        if (!player->has_failed())
            return false;
    }

    return true;
}

bool ScreenGameplay::has_song_finished() const {
    auto runtime = time_.stream;

    // music is not playing, game is active...
    if (music_ && !music_->is_playing() && active_) {
        runtime = time_.stream;
    }

    for (auto &player : players_) {
        if (!player->has_song_finished(runtime))
            return false;
    }

    return true;
}

void ScreenGameplay::update_song_time(float delta) {

    // First call.
    if (::isnan(time_.old_stream)) {
        if (music_ && music_->is_valid()) {
            if (time_.stream == 0) /* we have not sought already */
                music_->seek_time(-time_.waiting);

            // Music->SetPitch(0.8);
            music_->play();
        } else {
            time_.stream = -time_.waiting;
        }

        time_.audio_old = GetMixer()->GetTime();
    }

    // UpdateDecoder for the next delta.
    time_.old_stream = time_.stream;

    // Current Time
    if (music_ && music_->is_valid())
        /* map stream time to DAC queued sample times */
        time_.stream = music_->map_stream_clock(GetMixer()->GetTime());
    else {
        /* these remain deltas for rates*/
        double CurrAudioTime = GetMixer()->GetTime();
        time_.stream += CurrAudioTime - time_.audio_old;
        time_.audio_old = CurrAudioTime;
    }

#ifdef AUDIO_CLOCK_DEBUG
    if (music_->is_playing() && time_.stream > 0 && music_->get_played_time() > 0) {
        double expected = (GetMixer()->GetTime() - music_->get_played_time()) * music_->get_pitch();
        if (expected - time_.stream > 0.1) {
            std::cerr << "..." << std::endl;
        }
    }
#endif
}

void
ScreenGameplay::on_player_hit(rd::ScoreKeeperJudgment judgment, double dt, uint32_t lane, bool hold, bool release,
                            int pn) const {
    /// When a note is hit, this is called.
    // @callback HitEvent
    // @param judgment Judgment value.
    // @param dev Deviation from the note in ms.
    // @param lane 1-index based lane.
    // @param hold Whether the note was a hold.
    // @param release Whether it was a hold release.
    // @param pn Player number. Identifies who hit the note.
    if (scene_->get_script_manager()->CallFunction("HitEvent", 6)) {
        scene_->get_script_manager()->PushArgument(judgment);
        scene_->get_script_manager()->PushArgument(dt);
        scene_->get_script_manager()->PushArgument((int) lane + 1);
        scene_->get_script_manager()->PushArgument(hold);
        scene_->get_script_manager()->PushArgument(release);
        scene_->get_script_manager()->PushArgument(pn);
        scene_->get_script_manager()->RunFunction();
    }

    auto PlayerScoreKeeper = players_[pn]->get_score_keeper();
    if (PlayerScoreKeeper->getMaxJudgableNotes() == PlayerScoreKeeper->getScore(rd::ST_NOTES_HIT)) {
        /// Once a player achieves a full combo, this is called. This is called inmediately after HitEvent
        // so the script can keep track of player number who last hit.
        // @callback OnFullComboEvent
        scene_->trigger_event("OnFullComboEvent");
    }

}

void ScreenGameplay::on_player_miss(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn) const {
    bga_->OnMiss();

    /// Whenever a player fails, this is called.
    // @callback MissEvent
    // @param dev Deviation from the note in ms.
    // @param lane 1-index based lane.
    // @param hold Whether the note was a hold.
    // @param pn Player number identifying who missed the note.
    if (scene_->get_script_manager()->CallFunction("MissEvent", 4)) {
        scene_->get_script_manager()->PushArgument(dt);
        scene_->get_script_manager()->PushArgument((int) lane + 1);
        scene_->get_script_manager()->PushArgument(hold);
        scene_->get_script_manager()->PushArgument(pn);
        scene_->get_script_manager()->RunFunction();
    }
}

void ScreenGameplay::on_player_gear_key_event(uint32_t lane, bool keydown, int pn) const {
    /// Called when a gear button was pressed or released
    // @callback GearKeyEvent
    // @param lane 1-index based lane.
    // @param keydown Whether the key is down or up.
    // @param pn Player number identifying who performed this event.
    if (scene_->get_script_manager()->CallFunction("GearKeyEvent", 3)) {
        scene_->get_script_manager()->PushArgument((int) lane + 1);
        scene_->get_script_manager()->PushArgument(keydown);
        scene_->get_script_manager()->PushArgument(pn);

        scene_->get_script_manager()->RunFunction();
    }
}

bool ScreenGameplay::Run(double Delta) {
    if (Next)
        return RunNested(Delta);

    if (!load_successful_)
        return false;

    if (start_active_) {
        activate();
        start_active_ = false;
    }

    if (active_) {
        time_.game += Delta;
        time_.miss_layer -= Delta;
        time_.failure -= Delta;
        time_.success -= Delta;

        update_song_time(Delta);

        if (time_.game >= time_.waiting) {
            evaluate_stage_failure();
        }
    }

    run_auto_events();
    for (auto &p : players_)
        p->update(time_.stream);

    scene_->UpdateTargets(Delta);
    bga_->Update(Delta);
    render();

    if (Delta > 0.1)
        Log::Logf("ScreenGameplay7K: Delay@[ST%.03f/RST:%.03f] = %f\n", GetScreenTime(), time_.game, Delta);

    return is_active_;
}


void ScreenGameplay::render() {
    scene_->DrawUntilLayer(13);

    for (auto &p : players_) {
        if (playfield_clip_enabled_[p->get_player_number()]) {
            renderer::set_scissor(true);

            auto reg = playfield_clip_area_[p->get_player_number()];

            renderer::set_scissor_region_wnd(
                    reg.X1, reg.Y1, reg.width(), reg.height()
            );

            p->render(time_.stream);
            renderer::set_scissor(false);
        } else {
            p->render(time_.stream);
        }
    }

    scene_->DrawFromLayer(14);

}
