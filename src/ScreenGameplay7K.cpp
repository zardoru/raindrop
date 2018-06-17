#include "pch.h" 

#include "Logging.h"
#include "Screen.h"
#include "Audio.h"

#include "LuaManager.h"
#include "SceneEnvironment.h"

#include "ScoreKeeper7K.h"
#include "ScreenGameplay7K.h"
#include "ScreenEvaluation7K.h"
#include "Noteskin.h"
#include "GameState.h"

#include "LuaBridge.h"


namespace Game {
	namespace VSRG {
		void ScreenGameplay::Activate()
		{
			if (!Active)
				Animations->DoEvent("OnActivateEvent");

			Active = true;
		}

		bool ScreenGameplay::IsActive() const
		{
			return Active;
		}

		Game::VSRG::Song* ScreenGameplay::GetSong() const
		{
			return MySong.get();
		}


		void ScreenGameplay::PlayKeysound(int Keysound)
		{
			if (Keysounds.find(Keysound) != Keysounds.end() && PlayReactiveSounds)
				for (auto &&s : Keysounds[Keysound])
					if (s) s->Play();
		}


		// Called right after the scorekeeper and the engine's objects are initialized.
		void ScreenGameplay::SetupScriptConstants()
		{
			auto L = Animations->GetEnv();
			luabridge::push(L->GetState(), static_cast<Transformation*>(BGA.get()));
			lua_setglobal(L->GetState(), "Background");
		}

		// Called before the script is executed at all.
		void ScreenGameplay::SetupLua(LuaManager* Env)
		{
			GameState::GetInstance().InitializeLua(Env->GetState());
			Game::VSRG::PlayerContext::SetupLua(Env);

#define f(n, x) addProperty(n, &ScreenGameplay::x)
			luabridge::getGlobalNamespace(Env->GetState())
				.beginClass <ScreenGameplay>("ScreenGameplay7K")
				.f("Active", IsActive)
				.f("Song", GetSong)
				.addFunction("GetPlayer", &ScreenGameplay::GetPlayerContext)
				.addFunction("SetPlayerClip", &ScreenGameplay::SetPlayerClip)
				.addFunction("DisablePlayerClip", &ScreenGameplay::DisablePlayerClip)
				// All of these depend on the player.

				.endClass();

			luabridge::push(Env->GetState(), this);
			lua_setglobal(Env->GetState(), "Game");
		}


		Game::VSRG::PlayerContext* ScreenGameplay::GetPlayerContext(int i)
		{
			if (i >= 0 && i < Players.size())
				return Players[i].get();
			else
				return nullptr;
		}

		void ScreenGameplay::SetPlayerClip(int pn, AABB box)
		{
			PlayfieldClipEnabled[pn] = true;
			PlayfieldClipArea[pn] = box;
		}

		void ScreenGameplay::DisablePlayerClip(int pn)
		{
			PlayfieldClipEnabled[pn] = false;
		}

		bool ScreenGameplay::HandleInput(int32_t key, KeyEventType code, bool isMouseInput)
		{
			/*
			In here we should use the input arrangements depending on
			the amount of channels the current difficulty is using.
			Also potentially pausing and quitting the screen.
			Other than that most input can be safely ignored.
			*/

			/* Handle nested screens. */
			if (Screen::HandleInput(key, code, isMouseInput))
				return true;

			Animations->HandleInput(key, code, isMouseInput);

			if (code == KE_PRESS)
			{
				switch (BindingsManager::TranslateKey(key))
				{
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

				if (BindingsManager::TranslateKey7K(key) != KT_Unknown)
				{
					for (auto &player : Players)
					{
						player->TranslateKey(
							BindingsManager::TranslateKey7K(key),
							true,
							Time.Stream);
					}
				}
			}
			else
			{
				if (BindingsManager::TranslateKey7K(key) != KT_Unknown)
				{
					for (auto &player : Players)
					{
						player->TranslateKey(
							BindingsManager::TranslateKey7K(key),
							false,
							Time.Stream);
					}
				}
			}

			return true;
		}

		void ScreenGameplay::RunAutoEvents()
		{
			if (!StageFailureTriggered && Active)
			{
				// Play BGM events.
				while (BGMEvents.size() && BGMEvents.front().Time <= Time.Stream)
				{
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

			BGA->SetAnimationTime(Time.InterpolatedStream);
		}

		void ScreenGameplay::CheckShouldEndScreen()
		{
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

					for (auto i = Keysounds.begin(); i != Keysounds.end(); ++i)
						for (auto &&s : i->second)
							if (s)
								s->Stop();

					// Run stage failed animation.
					Animations->DoEvent("OnFailureEvent", 1);
					Time.Failure = Clamp(Animations->GetEnv()->GetFunctionResultF(), 0.0f, 30.0f);
				}
			};

			// Run failure first; make sure it has priority over checking whether it's a pass or not.
			if (PlayersHaveFailed() && !ShouldDelayFailure() && !StageFailureTriggered)
				perform_stage_failure();

			// Okay then, so it's a pass?
			if (SongHasFinished() && !StageFailureTriggered)
			{
				if (!SongPassTriggered)
				{
					// delayed failure check. 
					if (PlayersHaveFailed()) {
						perform_stage_failure(); // No, don't trigger SongPassTriggered. It wasn't a pass.
						return;
					}

					// do score submit
					GameState::GetInstance().SubmitScore(0);

					SongPassTriggered = true; // Reached the end!
					Animations->DoEvent("OnSongFinishedEvent", 1);
					Time.Success = Clamp(Animations->GetEnv()->GetFunctionResultF(), 3.0f, 30.0f);
				}
			}

			// Okay then, the song's done, and the success animation is done too. Time to evaluate.
			if (Time.Success < 0 && SongPassTriggered)
			{
				auto Eval = std::make_shared<ScreenEvaluation>();
				Eval->Init(this);
				Next = Eval;
			}

			if (StageFailureTriggered)
			{
				Time.Miss = 10; // Infinite, for as long as it lasts.
				if (Time.Failure <= 0)
				{
					if (Configuration::GetSkinConfigf("GoToSongSelectOnFailure") == 0)
					{
						auto Eval = std::make_shared<ScreenEvaluation>();
						Eval->Init(this);
						Next = Eval;
					}
					else
						Running = false;
				}
			}
		}

		bool ScreenGameplay::ShouldDelayFailure()
		{
			for (auto &player : Players) {
				if (player->HasDelayedFailure())
					return true;
			}

			return false;
		}

		bool ScreenGameplay::PlayersHaveFailed()
		{
			for (auto &player : Players) {
				if (!player->HasFailed())
					return false;
			}

			return true;
		}

		bool ScreenGameplay::SongHasFinished()
		{
			auto runtime = Time.Stream;

			// music is not playing, game is active...
			if (Music && !Music->IsPlaying() && Active) {
				runtime = Time.InterpolatedStream;
			}

			for (auto &player : Players) {
				if (!player->HasSongFinished(runtime))
					return false;
			}

			return true;
		}

		void ScreenGameplay::UpdateSongTime(float Delta)
		{
			// Check if we should play the music..
			if (Time.OldStream == -1)
			{
				if (Music)
					Music->Play();
				Time.AudioStart = MixerGetTime();
				Time.AudioOld = Time.AudioStart;
				if (StartMeasure <= 0)
				{
					Time.InterpolatedStream = 0;
					Time.Stream = 0;
				}
			}
			else
			{
				/* Update music. */
				Time.InterpolatedStream += Delta;
			}

			// Update for the next delta.
			Time.OldStream = Time.Stream;

			// Run interpolation
			double CurrAudioTime = MixerGetTime();
			double SongDelta = 0;
			if (Music && Music->IsPlaying())
				SongDelta = Music->GetPlayedTimeDAC() - Time.OldStream;
			else
				SongDelta = (CurrAudioTime - Time.AudioOld);


			double deltaErr = abs(Time.Stream - Time.InterpolatedStream);
			bool AboveTolerance = deltaErr * 1000 > TimeError.ToleranceMS;
			if ((SongDelta != 0 && AboveTolerance) || !Time.InterpolateStream) // Significant delta with a x ms difference? We're pretty off..
			{
				if (TimeError.ToleranceMS && Time.InterpolateStream)
					Log::LogPrintf(
						"Audio Desync: delta = %f ms difference = %f ms."
						" Real song time %f (expected %f) Audio current time: %f (old = %f)\n",
						SongDelta * 1000,
						deltaErr * 1000,
						Time.Stream,
						Time.InterpolatedStream,
						CurrAudioTime,
						Time.AudioOld);

				Time.InterpolatedStream = Time.Stream;
			}

			Time.AudioOld = CurrAudioTime;
			Time.Stream += SongDelta;
		}

		void ScreenGameplay::OnPlayerHit(ScoreKeeperJudgment judgment, double dt, uint32_t lane, bool hold, bool release, int pn)
		{

			if (Animations->GetEnv()->CallFunction("HitEvent", 6))
			{
				Animations->GetEnv()->PushArgument(judgment);
				Animations->GetEnv()->PushArgument(dt);
				Animations->GetEnv()->PushArgument((int)lane + 1);
				Animations->GetEnv()->PushArgument(hold);
				Animations->GetEnv()->PushArgument(release);
				Animations->GetEnv()->PushArgument(pn);
				Animations->GetEnv()->RunFunction();
			}

			auto PlayerScoreKeeper = Players[pn]->GetScoreKeeper();
			if (PlayerScoreKeeper->getMaxNotes() == PlayerScoreKeeper->getScore(ST_NOTES_HIT))
				Animations->DoEvent("OnFullComboEvent");

		}

		void ScreenGameplay::OnPlayerMiss(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn)
		{
			BGA->OnMiss();

			if (Animations->GetEnv()->CallFunction("MissEvent", 4))
			{
				Animations->GetEnv()->PushArgument(dt);
				Animations->GetEnv()->PushArgument((int)lane + 1);
				Animations->GetEnv()->PushArgument(hold);
				Animations->GetEnv()->PushArgument(pn);
				Animations->GetEnv()->RunFunction();
			}
		}

		void ScreenGameplay::OnPlayerGearKeyEvent(uint32_t lane, bool keydown, int pn)
		{
			if (Animations->GetEnv()->CallFunction("GearKeyEvent", 3))
			{
				Animations->GetEnv()->PushArgument((int)lane + 1);
				Animations->GetEnv()->PushArgument(keydown);
				Animations->GetEnv()->PushArgument(pn);

				Animations->GetEnv()->RunFunction();
			}
		}

		bool ScreenGameplay::Run(double Delta)
		{
			if (Next)
				return RunNested(Delta);

			if (!DoPlay)
				return false;

			if (ForceActivation)
			{
				Activate();
				ForceActivation = false;
			}

			if (Active)
			{
				Time.Game += Delta;
				Time.Miss -= Delta;
				Time.Failure -= Delta;
				Time.Success -= Delta;

				if (Time.Game >= Time.Waiting)
				{
					UpdateSongTime(Delta);

					// PlayerContext::Update(Time.Stream)
					CheckShouldEndScreen();
				}
				else
				{
					Time.InterpolatedStream = -(Time.Waiting - Time.Game);
					Time.Stream = Time.InterpolatedStream;
				}
			}

			RunAutoEvents();
			for (auto &p : Players)
				p->Update(Time.InterpolatedStream);

			Animations->UpdateTargets(Delta);
			BGA->Update(Delta);
			Render();

			if (Delta > 0.1)
				Log::Logf("ScreenGameplay7K: Delay@[ST%.03f/RST:%.03f] = %f\n", GetScreenTime(), Time.Game, Delta);

			return Running;
		}


		void ScreenGameplay::Render()
		{
			Animations->DrawUntilLayer(13);

			for (auto &p : Players) {
				if (PlayfieldClipEnabled[p->GetPlayerNumber()])
				{
					Renderer::SetScissor(true);

					auto reg = PlayfieldClipArea[p->GetPlayerNumber()];

					Renderer::SetScissorRegionWnd(
						reg.X1, reg.Y1, reg.width(), reg.height()
					);

					p->Render(Time.InterpolatedStream);
					Renderer::SetScissor(false);
				}
				else {
					p->Render(Time.InterpolatedStream);
				}

				Animations->DrawFromLayer(14);
			}

		}
	}
}