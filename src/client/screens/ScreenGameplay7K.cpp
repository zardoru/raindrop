#include <memory>
#include <filesystem>

#include <queue>
#include <future>

#include <Audio.h>
#include <Audiofile.h>
#include <AudioSourceOJM.h>

#include "Logging.h"
#include "../structure/Screen.h"

#include "LuaManager.h"
#include <LuaBridge/LuaBridge.h>
#include <rmath.h>
#include "../structure/SceneEnvironment.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include <game/Song.h>
#include <game/PlayerChartState.h>
#include <game/VSRGMechanics.h>
#include <game/ScoreKeeper7K.h>
#include "../game/PlayscreenParameters.h"
#include "../game/Noteskin.h"
#include "../game/PlayerContext.h"
#include "../bga/BackgroundAnimation.h"
#include "ScreenGameplay7K.h"

#include "ScreenEvaluation7K.h"

#include "../game/GameState.h"
#include "../game/Game.h"
#include "../structure/Configuration.h"

/// @themescript screengameplay7k.lua
void ScreenGameplay::Activate() {
    /// Called once the song time starts advancing.
    // @callback OnActivateEvent
    if (!Active)
        Animations->DoEvent("OnActivateEvent");

    Active = true;
}

bool ScreenGameplay::IsActive() const {
    return Active;
}

rd::Song *ScreenGameplay::GetSong() const {
    return MySong.get();
}


void ScreenGameplay::PlayKeysound(int Keysound) {
    if (Keysounds.find(Keysound) != Keysounds.end() && PlayReactiveSounds)
        for (auto &&s : Keysounds[Keysound])
            if (s) s->Play();
}


// Called right after the scorekeeper and the engine's objects are initialized.
void ScreenGameplay::SetupScriptConstants() {
    auto L = Animations->GetEnv();
    luabridge::push(L->GetState(), static_cast<Transformation *>(BGA.get()));
    /// The BGA's transform.
    // @autoinstance Background
    lua_setglobal(L->GetState(), "Background");
}

// Called before the script is executed at all.
void ScreenGameplay::SetupLua(LuaManager *Env) {
    /// Global Gamestate
    // @autoinstance Global
    GameState::GetInstance().InitializeLua(Env->GetState());
    PlayerContext::SetupLua(Env);

    AddScriptClasses(Env);

    luabridge::push(Env->GetState(), this);
    /// ScreenGameplay instance.
    // @autoinstance rd
    lua_setglobal(Env->GetState(), "rd");
}


PlayerContext *ScreenGameplay::GetPlayerContext(int i) {
    if (i >= 0 && i < Players.size())
        return Players[i].get();
    else
        return nullptr;
}

void ScreenGameplay::SetPlayerClip(int pn, AABB box) {
    PlayfieldClipEnabled[pn] = true;
    PlayfieldClipArea[pn] = box;
}

void ScreenGameplay::DisablePlayerClip(int pn) {
    PlayfieldClipEnabled[pn] = false;
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

    Animations->HandleInput(key, isPressed, isMouseInput);

    if (isPressed) {
        switch (BindingsManager::TranslateKey(key)) {
            case KT_Escape:
                if (SongPassTriggered)
                    Time.Success = -1;
                else
                    Running = false;
                break;
            case KT_Enter:
                if (!Active)
                    Activate();
                break;
            default:
                break;
        }

#ifndef NDEBUG
        if (key == 290) // f1
        {
            if (Music)
                Music->SetPitch(Music->GetPitch() - 0.2);
        }
        if (key == 291)
        {
            if (Music)
                Music->SetPitch(Music->GetPitch() + 0.2);
        }// f2
#endif

        if (BindingsManager::TranslateKey7K(key) != KT_Unknown) {
            for (auto &player : Players) {
                player->TranslateKey(
                        BindingsManager::TranslateKey7K(key),
                        true,
                        Time.Stream);
            }
        }
    } else {
        if (BindingsManager::TranslateKey7K(key) != KT_Unknown) {
            for (auto &player : Players) {
                player->TranslateKey(
                        BindingsManager::TranslateKey7K(key),
                        false,
                        Time.Stream);
            }
        }
    }

    return true;
}

void ScreenGameplay::RunAutoEvents() {
    if (!StageFailureTriggered && Active) {
        // Play BGM events.
        while (!BGMEvents.empty() && BGMEvents.front().Time <= Time.Stream) {
            for (auto &&s : Keysounds[BGMEvents.front().Sound])
                if (s) {
                    double dt = Time.Stream - BGMEvents.front().Time;
                    if (dt < s->GetDuration()) {
                        s->SeekTime(dt);
                        s->Play();
                    }
                }
            BGMEvents.pop();
        }
    }

    BGA->SetAnimationTime(Time.Stream);
}

void ScreenGameplay::CheckShouldEndScreen() {
    auto perform_stage_failure = [&]() {
        StageFailureTriggered = true;
        // ScoreKeeper->failStage();

        // go to evaluation screen, or back to song select depending on the skin
        GameState::GetInstance().SubmitScore(0);

        // post-gameplay failure?
        if (!ShouldDelayFailure()) {
            FailSnd.Play();

            // We stop all audio..
            if (Music)
                Music->Stop();

            for (auto & Keysound : Keysounds)
                for (auto &&s : Keysound.second)
                    if (s)
                        s->Stop();

            // run failure event

            /// If the player fails, this is called. 
            // Return time to wait before transitioning out of this screen. Clamped to [0,30]
            // @callback OnFailureEvent
            Animations->DoEvent("OnFailureEvent", 1);
            Time.Failure = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
        }
    };

    // Run failure first; make sure it has priority over checking whether it's a pass or not.
    if (PlayersHaveFailed() && !ShouldDelayFailure() && !StageFailureTriggered)
        perform_stage_failure();

    // Okay then, so it's a pass?
    if (SongHasFinished() && !StageFailureTriggered) {
        if (!SongPassTriggered) {
            // delayed failure check. 
            if (PlayersHaveFailed()) {
                perform_stage_failure(); // No, don't trigger SongPassTriggered. It wasn't a pass.
                return;
            }

            // do score submit
            GameState::GetInstance().SubmitScore(0);

            SongPassTriggered = true; // Reached the end!

            /// If the player succeeds, this is called.
            // Returns time to exit screen. Clamped [0,30]
            // @callback OnSongFinishedEvent
            Animations->DoEvent("OnSongFinishedEvent", 1);
            Time.Success = Clamp(Animations->GetEnv()->GetFunctionResultF(), 1.0f, 30.0f);
        }
    }

    // Okay then, the song's done, and the success animation is done too. Time to evaluate.
    if (Time.Success < 0 && SongPassTriggered) {
        auto Eval = std::make_shared<ScreenEvaluation>();
        Eval->Init(this);
        Next = Eval;
    }

    if (StageFailureTriggered) {
        Time.Miss = 10; // Infinite, for as long as it lasts.
        if (Time.Failure <= 0) {
            if (Configuration::GetSkinConfigf("GoToSongSelectOnFailure") == 0) {
                auto Eval = std::make_shared<ScreenEvaluation>();
                Eval->Init(this);
                Next = Eval;
            } else
                Running = false;
        }
    }
}

bool ScreenGameplay::ShouldDelayFailure() {
    for (auto &player : Players) {
        if (player->HasDelayedFailure())
            return true;
    }

    return false;
}

bool ScreenGameplay::PlayersHaveFailed() {
    for (auto &player : Players) {
        if (!player->HasFailed())
            return false;
    }

    return true;
}

bool ScreenGameplay::SongHasFinished() {
    auto runtime = Time.Stream;

    // music is not playing, game is active...
    if (Music && !Music->IsPlaying() && Active) {
        runtime = Time.Stream;
    }

    for (auto &player : Players) {
        if (!player->HasSongFinished(runtime))
            return false;
    }

    return true;
}

void ScreenGameplay::UpdateSongTime(float Delta) {

    // First call.
    if (isnan(Time.OldStream)) {
        if (Music && Music->IsValid()) {
            if (Time.Stream == 0) /* we have not sought already */
                Music->SeekTime(-Time.Waiting);

            // Music->SetPitch(0.8);
            Music->Play();
        } else {
            Time.Stream = -Time.Waiting;
        }

        Time.AudioOld = GetMixer()->GetTime();
    }

    // UpdateDecoder for the next delta.
    Time.OldStream = Time.Stream;

    // Current Time
    if (Music && Music->IsValid())
        /* map stream time to DAC queued sample times */
        Time.Stream = Music->MapStreamClock(GetMixer()->GetTime());
    else {
        /* these remain deltas for rates*/
        double CurrAudioTime = GetMixer()->GetTime();
        Time.Stream += CurrAudioTime - Time.AudioOld;
        Time.AudioOld = CurrAudioTime;
    }

#ifdef AUDIO_CLOCK_DEBUG
    if (Music->IsPlaying() && Time.Stream > 0 && Music->GetPlayedTime() > 0) {
        double expected = (GetMixer()->GetTime() - Music->GetPlayedTime()) * Music->GetPitch();
        if (expected - Time.Stream > 0.1) {
            std::cerr << "..." << std::endl;
        }
    }
#endif
}

void
ScreenGameplay::OnPlayerHit(rd::ScoreKeeperJudgment judgment, double dt, uint32_t lane, bool hold, bool release,
                            int pn) {
    /// When a note is hit, this is called.
    // @callback HitEvent
    // @param judgment Judgment value.
    // @param dev Deviation from the note in ms.
    // @param lane 1-index based lane.
    // @param hold Whether the note was a hold.
    // @param release Whether it was a hold release.
    // @param pn Player number. Identifies who hit the note.
    if (Animations->GetEnv()->CallFunction("HitEvent", 6)) {
        Animations->GetEnv()->PushArgument(judgment);
        Animations->GetEnv()->PushArgument(dt);
        Animations->GetEnv()->PushArgument((int) lane + 1);
        Animations->GetEnv()->PushArgument(hold);
        Animations->GetEnv()->PushArgument(release);
        Animations->GetEnv()->PushArgument(pn);
        Animations->GetEnv()->RunFunction();
    }

    auto PlayerScoreKeeper = Players[pn]->GetScoreKeeper();
    if (PlayerScoreKeeper->getMaxNotes() == PlayerScoreKeeper->getScore(rd::ST_NOTES_HIT)) {
        /// Once a player achieves a full combo, this is called. This is called inmediately after HitEvent
        // so the script can keep track of player number who last hit.
        // @callback OnFullComboEvent
        Animations->DoEvent("OnFullComboEvent");
    }

}

void ScreenGameplay::OnPlayerMiss(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn) {
    BGA->OnMiss();

    /// Whenever a player fails, this is called.
    // @callback MissEvent
    // @param dev Deviation from the note in ms.
    // @param lane 1-index based lane.
    // @param hold Whether the note was a hold.
    // @param pn Player number identifying who missed the note.
    if (Animations->GetEnv()->CallFunction("MissEvent", 4)) {
        Animations->GetEnv()->PushArgument(dt);
        Animations->GetEnv()->PushArgument((int) lane + 1);
        Animations->GetEnv()->PushArgument(hold);
        Animations->GetEnv()->PushArgument(pn);
        Animations->GetEnv()->RunFunction();
    }
}

void ScreenGameplay::OnPlayerGearKeyEvent(uint32_t lane, bool keydown, int pn) {
    /// Called when a gear button was pressed or released
    // @callback GearKeyEvent
    // @param lane 1-index based lane.
    // @param keydown Whether the key is down or up.
    // @param pn Player number identifying who performed this event.
    if (Animations->GetEnv()->CallFunction("GearKeyEvent", 3)) {
        Animations->GetEnv()->PushArgument((int) lane + 1);
        Animations->GetEnv()->PushArgument(keydown);
        Animations->GetEnv()->PushArgument(pn);

        Animations->GetEnv()->RunFunction();
    }
}

bool ScreenGameplay::Run(double Delta) {
    if (Next)
        return RunNested(Delta);

    if (!DoPlay)
        return false;

    if (ForceActivation) {
        Activate();
        ForceActivation = false;
    }

    if (Active) {
        Time.Game += Delta;
        Time.Miss -= Delta;
        Time.Failure -= Delta;
        Time.Success -= Delta;

        UpdateSongTime(Delta);

        if (Time.Game >= Time.Waiting) {
            CheckShouldEndScreen();
        }
    }

    RunAutoEvents();
    for (auto &p : Players)
        p->Update(Time.Stream);

    Animations->UpdateTargets(Delta);
    BGA->Update(Delta);
    Render();

    if (Delta > 0.1)
        Log::Logf("ScreenGameplay7K: Delay@[ST%.03f/RST:%.03f] = %f\n", GetScreenTime(), Time.Game, Delta);

    return Running;
}


void ScreenGameplay::Render() {
    Animations->DrawUntilLayer(13);

    for (auto &p : Players) {
        if (PlayfieldClipEnabled[p->GetPlayerNumber()]) {
            Renderer::SetScissor(true);

            auto reg = PlayfieldClipArea[p->GetPlayerNumber()];

            Renderer::SetScissorRegionWnd(
                    reg.X1, reg.Y1, reg.width(), reg.height()
            );

            p->Render(Time.Stream);
            Renderer::SetScissor(false);
        } else {
            p->Render(Time.Stream);
        }

        Animations->DrawFromLayer(14);
    }

}
