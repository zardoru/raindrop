#include "pch.h"
#include "PlayerContext.h"
#include "LuaManager.h"
#include "ScoreKeeper7K.h"
#include "LuaBridge.h"
#include "Logging.h"
#include "Line.h"
#include "NoteTransformations.h"
#include "GameWindow.h"
#include "BitmapFont.h"
#include "Shader.h"

BitmapFont *fnt = nullptr;
CfgVar DebugNoteRendering("NoteRender", "Debug");

namespace Game {
	namespace VSRG {

		auto NoteLboundFuncHead = [](const TrackNote* A, const double &B) -> bool
		{
			return A->GetStartTime() < B;
		};

		auto NoteHboundFuncHead = [](const double &A, const TrackNote* B) -> bool
		{
			return A < B->GetStartTime();
		};

		auto NoteLboundFunc = [](const TrackNote* A, const double &B) -> bool
		{
			return A->GetEndTime() < B;
		};
		auto NoteHboundFunc = [](const double &A, const TrackNote* B) -> bool
		{
			return A < B->GetEndTime();
		};

		PlayerContext::PlayerContext(int pn, Game::VSRG::PlayscreenParameters p) : PlayerNoteskin(this)
		{
			PlayerNumber = pn;
			Drift = 0;
			JudgeOffset = 0;

			JudgeNotes = true;

			PlayerScoreKeeper = std::make_shared<ScoreKeeper>();
			Parameters = p;

			Gear = {};

			if (!fnt && DebugNoteRendering) {
				fnt = new BitmapFont();
				fnt->LoadSkinFontImage("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
			}

			Barline = nullptr;
		}

		PlayerContext::~PlayerContext()
		{
			if (Barline)
			{
				delete Barline;
				Barline = nullptr;
			}
		}

		void PlayerContext::Init() {
			PlayerNoteskin.SetupNoteskin(ChartState.HasTurntable, CurrentDiff->Channels);
		}

		void PlayerContext::Validate() {
			PlayerNoteskin.Validate();
			MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));


			if (!BindKeysToLanes(ChartState.HasTurntable))
				if (!BindKeysToLanes(!ChartState.HasTurntable))
					Log::LogPrintf("Couldn't get valid bindings for current key count %d.\n", GetChannelCount());

			if (PlayerNoteskin.IsBarlineEnabled())
				Barline = new Line();
		}


		const PlayerChartState& PlayerContext::GetPlayerState()
		{
			return ChartState;
		}

		void PlayerContext::SetupMechanics()
		{
			using namespace Game::VSRG;

			MechanicsSet = Parameters.PrepareMechanicsSet(CurrentDiff, PlayerScoreKeeper, GetJudgmentY());
			MechanicsSet->TransformNotes(ChartState);


			MechanicsSet->Setup(CurrentDiff.get(), PlayerScoreKeeper);
			// Setup mechanics set callbacks
			MechanicsSet->HitNotify = std::bind(&PlayerContext::HitNote, this, 
				std::placeholders::_1, 
				std::placeholders::_2, 
				std::placeholders::_3, 
				std::placeholders::_4);
			
			MechanicsSet->MissNotify = std::bind(&PlayerContext::MissNote, this, 
				std::placeholders::_1, 
				std::placeholders::_2, 
				std::placeholders::_3, 
				std::placeholders::_4, 
				std::placeholders::_5);
			
			MechanicsSet->IsLaneKeyDown = std::bind(&PlayerContext::GetGearLaneState, this, 
				std::placeholders::_1);
			
			MechanicsSet->SetLaneHoldingState = std::bind(&PlayerContext::SetLaneHoldState, this, 
				std::placeholders::_1, 
				std::placeholders::_2);

			MechanicsSet->PlayNoteSoundEvent = PlayKeysound;
			// We're set - setup all of the variables that depend on mechanics, scoring etc.. to their initial values.
		}

		void PlayerContext::OnPlayerKeyEvent(double Time, bool KeyDown, uint32_t lane)
		{
			PlayerReplay.AddEvent(Replay::Entry
			{
				Time - Drift,
				lane,
				KeyDown
			});

			if (KeyDown)
			{
				JudgeLane(lane, GetChartTimeAt(Time - Drift));
				Gear.IsPressed[lane] = true;
			}
			else
			{
				ReleaseLane(lane, GetChartTimeAt(Time - Drift));
				Gear.IsPressed[lane] = false;
			}
		}


		void PlayerContext::TranslateKey(int32_t Index, bool KeyDown, double Time)
		{
			if (Parameters.Auto)
				return;

			if (Index < 0)
				return;

			if (Gear.Bindings.find(Index) == Gear.Bindings.end())
				return;

			int GearIndex = Gear.Bindings[Index]; /* Binding this key to a lane */

			if (GearIndex >= VSRG::MAX_CHANNELS || GearIndex < 0)
				return;

			OnPlayerKeyEvent(Time + JudgeOffset, KeyDown, GearIndex);
		}


		bool PlayerContext::IsFailEnabled() const
		{
			return !Parameters.NoFail;
		}

		bool PlayerContext::IsUpscrolling() const
		{
			return GetAppliedSpeedMultiplier(LastUpdateTime - Drift) < 0;
		}


		double PlayerContext::GetCurrentBPM() const
		{
			return ChartState.GetBpmAt(GetWarpedSongTime());
		}

		double PlayerContext::GetJudgmentY() const
		{
			if (IsUpscrolling())
				return PlayerNoteskin.GetJudgmentY();
			else
				return ScreenHeight - PlayerNoteskin.GetJudgmentY();
		}


		VSRG::Difficulty* PlayerContext::GetDifficulty() const
		{
			return CurrentDiff.get();
		}

		double PlayerContext::GetDuration() const
		{
			return ChartState.ConnectedDifficulty->Duration;
		}

		double PlayerContext::GetBeatDuration() const
		{
			return ChartState.GetBeatAt(GetDuration());
		}

		int PlayerContext::GetChannelCount() const
		{
			return ChartState.ConnectedDifficulty->Channels;
		}

		int PlayerContext::GetPlayerNumber() const
		{
			return PlayerNumber;
		}

		bool PlayerContext::GetIsHeldKey(int lane) const
		{
			if (lane >= 0 && lane < GetChannelCount())
				return Gear.HeldKey[lane];
			else
				return false;
		}

		bool PlayerContext::GetUsesTurntable() const
		{
			return ChartState.HasTurntable;
		}

		double PlayerContext::GetAppliedSpeedMultiplier(double Time) const
		{
			auto sm = ChartState.GetSpeedMultiplierAt(Time);
			if (Parameters.Upscroll)
				return -sm;
			else
				return sm;
		}

		double PlayerContext::GetCurrentBeat() const
		{
			return ChartState.GetBeatAt(LastUpdateTime - Drift);
		}

		double PlayerContext::GetUserMultiplier() const
		{
			return Parameters.UserSpeedMultiplier;
		}

		double PlayerContext::GetCurrentVerticalSpeed() const
		{
			return ChartState.GetDisplacementSpeedAt(LastUpdateTime - Drift);
		}

		double PlayerContext::GetWarpedSongTime() const
		{
			return ChartState.GetWarpedSongTime(LastUpdateTime - Drift);
		}

		void PlayerContext::SetupLua(LuaManager *Env)
		{
			assert(Env != nullptr);

			luabridge::getGlobalNamespace(Env->GetState())
				/// @engineclass Player
				.beginClass<PlayerContext>("PlayerContext")
				/// Current song beat
				// @roproperty Beat 
				.addProperty("Beat", &PlayerContext::GetCurrentBeat)
				/// Song time in warped space
				// @roproperty Time
				.addProperty("Time", &PlayerContext::GetWarpedSongTime)
				/// Song duration in warp space
				// @roproperty Duration
				.addProperty("Duration", &PlayerContext::GetDuration)
				/// Beat duration in warp space
				// @roproperty BeatDuration
				.addProperty("BeatDuration", &PlayerContext::GetBeatDuration)
				/// Active channel count
				// @roproperty Channels
				.addProperty("Channels", &PlayerContext::GetChannelCount)
				/// Current chart BPM
				// @roproperty BPM
				.addProperty("BPM", &PlayerContext::GetCurrentBPM)
				/// Whether failure is enabled
				// @roproperty CanFail
				.addProperty("CanFail", &PlayerContext::IsFailEnabled)
				/// Whether the player has failed
				// @roproperty HasFailed
				.addProperty("HasFailed", &PlayerContext::HasFailed)
				/// Whether the notes are currently moving towards the top of the screen
				// @roproperty Upscroll
				.addProperty("Upscroll", &PlayerContext::IsUpscrolling)
				/// The current displacement speed for the notes
				// @roproperty Speed
				.addProperty("Speed", &PlayerContext::GetCurrentVerticalSpeed)
				/// The current effective position of the judgment line
				// @roproperty JudgmentY
				.addProperty("JudgmentY", &PlayerContext::GetJudgmentY)
				/// Current difficulty assigned to the player
				// @roproperty Difficulty
				.addProperty("Difficulty", &PlayerContext::GetDifficulty)
				/// Whether the current chart is using a turntable
				// @roproperty Turntable
				.addProperty("Turntable", &PlayerContext::GetUsesTurntable)
				/// Current speed multiplier
				// @property UserSpeedMultiplier
				.addProperty("UserSpeedMultiplier", &PlayerContext::GetUserMultiplier, &PlayerContext::SetUserMultiplier)
				/// Same as Turntable
				// @roproperty HasTurntable
				.addProperty("HasTurntable", &PlayerContext::GetUsesTurntable)
				/// Get current gauge health as a percentage
				// @roproperty LifebarPercent
				.addProperty("LifebarPercent", &PlayerContext::GetLifePST)
				/// Current player number
				// @roproperty Number
				.addProperty("Number", &PlayerContext::GetPlayerNumber)
				/// Get currently active score type's score value
				// @roproperty Score
				.addProperty("Score", &PlayerContext::GetScore)
				/// Current player combo
				// @roproperty Combo
				.addProperty("Combo", &PlayerContext::GetCombo)
				/// Get Pacemaker text (a la rank:+/-xxx - the rank part.)
				// @function GetPacemakerText
				// @param bm Whether to use the BMS grading pacemaker. Uses raindrop pacemaker otherwise
				.addFunction("GetPacemakerText", &PlayerContext::GetPacemakerText)
				/// Get pacemaker value (a la rank:+/-xxx - the xxx part.)
				// @function GetPacemakerText
				// @param bm Whether to use the BMS grading pacemaker. Uses raindrop pacemaker otherwise
				.addFunction("GetPacemakerValue", &PlayerContext::GetPacemakerValue)
				/// Get if there's a hold currently being held at the given lane
				// @function IsHoldActive
				// @param lane Lane, 0-index based
				// @return A boolean, stating whether the lane has a hold currently being pressed.
				.addFunction("IsHoldActive", &PlayerContext::GetIsHeldKey)
				/// Get the closest note time to the last key press' timestamp.
				// @function GetClosestNoteTime
				// @param lane Lane, 0-index based
				// @return Closest note time, in MS.
				.addFunction("GetClosestNoteTime", &PlayerContext::GetClosestNoteTime)
				/// Get current player @{ScoreKeeper7K} instance
				// @roproperty Scorekeeper
				.addProperty("Scorekeeper", &PlayerContext::GetScoreKeeper)
				.endClass();
		}

		Replay PlayerContext::GetReplay()
		{
			return PlayerReplay;
		}

		void PlayerContext::LoadReplay(std::filesystem::path path)
		{
			PlayerReplay.Load(path);
		}

		double PlayerContext::GetScore() const
		{
			return PlayerScoreKeeper->getScore(Parameters.GetScoringType());
		}

		int PlayerContext::GetCombo() const
		{
			return PlayerScoreKeeper->getScore(ST_COMBO);
		}

		bool PlayerContext::BindKeysToLanes(bool UseTurntable)
		{
			std::string KeyProfile;
			std::string value;
			std::vector<std::string> res;

			auto CurrentDiff = ChartState.ConnectedDifficulty;

			if (UseTurntable)
				KeyProfile = (std::string)CfgVar("KeyProfileSpecial" + Utility::IntToStr(CurrentDiff->Channels));
			else
				KeyProfile = (std::string)CfgVar("KeyProfile" + Utility::IntToStr(CurrentDiff->Channels));

			value = (std::string)CfgVar("Keys", KeyProfile);
			res = Utility::TokenSplit(value);

			for (unsigned i = 0; i < CurrentDiff->Channels; i++)
			{
				Gear.ClosestNoteMS[i] = 0;

				if (i < res.size())
					Gear.Bindings[static_cast<int>(latof(res[i]))] = i;
				else
				{
					if (!Parameters.Auto)
					{
						Log::Printf("Mising bindings starting from lane " + Utility::IntToStr(i) + " using profile " + KeyProfile);
						return false;
					}
				}

				Gear.HeldKey[i] = false;
				Gear.IsPressed[i] = 0;
			}

			return true;
		}

		void PlayerContext::HitNote(double TimeOff, uint32_t Lane, bool IsHold, bool IsHoldRelease)
		{
			auto Judgment = PlayerScoreKeeper->hitNote(TimeOff);

			if (OnHit)
				OnHit(Judgment, TimeOff, Lane, IsHold, IsHoldRelease, PlayerNumber);
		}

		void PlayerContext::MissNote(double TimeOff, uint32_t Lane, bool IsHold, bool dont_break_combo, bool early_miss)
		{
			PlayerScoreKeeper->missNote(dont_break_combo, early_miss);

			if (IsHold)
				Gear.HeldKey[Lane] = false;

			if (OnMiss)
				OnMiss(TimeOff, Lane, IsHold, dont_break_combo, early_miss, PlayerNumber);
		}

		void PlayerContext::SetLaneHoldState(uint32_t Lane, bool NewState)
		{
			Gear.HeldKey[Lane] = NewState;
		}

		void PlayerContext::PlayLaneKeysound(uint32_t Lane)
		{
			TrackNote *TN = Gear.CurrentKeysounds[Lane];
			if (!TN) return;

			PlayKeysound(TN->GetSound());
		}

		double PlayerContext::GetChartTimeAt(double time) const
		{
			if (MechanicsSet->GetTimingKind() == TT_BEATS) {
				return ChartState.GetBeatAt(time);
			}
			/*else if (MechanicsSet->GetTimingKind() == TT_TIME) {
				return time;
			}*/
			else
				return time;
		}

		// true if holding down key
		bool PlayerContext::GetGearLaneState(uint32_t Lane)
		{
			return Gear.IsPressed[Lane] != 0;
		}

		void PlayerContext::RunAuto(TrackNote *m, double usedTime, uint32_t k)
		{
			auto perfect_auto = true;
			double TimeThreshold = usedTime + 0.008; // latest time a note can activate.
			if (m->GetStartTime() <= TimeThreshold)
			{
				if (m->IsEnabled()) {
					if (m->IsHold())
					{
						if (m->WasHit())
						{
							if (m->GetEndTime() < TimeThreshold) {
								double hit_time = clamp_to_interval(usedTime, m->GetEndTime(), 0.008);
								// We use clamp_to_interval for those pesky outliers.
								if (perfect_auto) ReleaseLane(k, m->GetEndTime());
								else ReleaseLane(k, hit_time);
							}
						}
						else {
							double hit_time = clamp_to_interval(usedTime, m->GetStartTime(), 0.008);
							if (perfect_auto) JudgeLane(k, m->GetStartTime());
							else JudgeLane(k, hit_time);
						}
					}
					else
					{
						double hit_time = clamp_to_interval(usedTime, m->GetStartTime(), 0.008);
						if (perfect_auto) {
							JudgeLane(k, m->GetStartTime());
							ReleaseLane(k, m->GetEndTime());
						}
						else {
							JudgeLane(k, hit_time);
							ReleaseLane(k, hit_time);
						}
					}
				}

			}
		}

		void PlayerContext::RunMeasures(double time)
		{
			/*
				Notes are always ran at unwarped time. GameChartData unwarps the time.
			*/
			double timeClosest[VSRG::MAX_CHANNELS];
			auto perfect_auto = true;

			for (int i = 0; i < VSRG::MAX_CHANNELS; i++)
				timeClosest[i] = std::numeric_limits<double>::infinity();

			double usedTime = GetChartTimeAt(time);
			auto &NotesByChannel = ChartState.NotesTimeOrdered;

			for (auto k = 0U; k < CurrentDiff->Channels; k++)
			{
				for (auto mp = NotesByChannel[k].begin(); mp != NotesByChannel[k].end(); ++mp) {
					auto m = *mp;
					// Keysound update to closest note.
					if (m->IsEnabled())
					{
						auto t = abs(usedTime - m->GetEndTime());
						if (t < timeClosest[k])
						{
							if (CurrentDiff->IsVirtual)
								Gear.CurrentKeysounds[k] = &(*m);
							Gear.ClosestNoteMS[k] = abs(usedTime - m->GetEndTime());
							timeClosest[k] = t;
						}
					}

					if (!m->IsJudgable() || !CanJudge())
						continue;

					// Autoplay
					if (Parameters.Auto) {
						RunAuto(m, usedTime, k);
						if (!m->IsJudgable()) continue;
					}

					if (!CanJudge()) continue; // don't check for judgments after stage has failed.

					if (MechanicsSet->OnUpdate(usedTime, &(*m), k))
						break;
				} // end for notes
			} // end for channels
		}

		void PlayerContext::ReleaseLane(uint32_t Lane, double Time)
		{
			GearKeyEvent(Lane, false);

			if (!CanJudge()) return; // don't judge any more after stage is failed.

			auto &NotesByChannel = ChartState.NotesTimeOrdered;
			auto Start = NotesByChannel[Lane].begin();
			auto End = NotesByChannel[Lane].end();

			// Use this optimization when we can make sure vertical properly aligns up with time.
			//if (ChartState.IsNoteTimeSorted())
			{
				// In comparison to the regular compare function, since end times are what matter with holds (or lift events, where start == end)
				// this does the job as it should instead of comparing start times where hold tails would be completely ignored.
				auto threshold = (PlayerScoreKeeper->usesO2() ?
					PlayerScoreKeeper->getLateMissCutoffMS() :
					(PlayerScoreKeeper->getLateMissCutoffMS() / 1000.0));

				auto timeLower = (Time - threshold);
				auto timeHigher = (Time + threshold);

				Start = std::lower_bound(
					NotesByChannel[Lane].begin(), 
					NotesByChannel[Lane].end(), timeLower, NoteLboundFunc);

				// Locate the first hold that we can judge in this range (Pending holds. Similar to what was done when drawing.)
				auto rStart = std::reverse_iterator<std::vector<TrackNote*>::iterator>(Start);
				for (auto i = rStart; i != NotesByChannel[Lane].rend(); ++i)
				{
					auto ip = *i;
					if (ip->IsHold() 
						&& ip->IsEnabled() 
						&& ip->IsJudgable() 
						&& ip->WasHit() 
						&& !ip->FailedHit())
						Start = i.base() - 1;
				}

				End = std::upper_bound(
					NotesByChannel[Lane].begin(), 
					NotesByChannel[Lane].end(), 
					timeHigher, 
					NoteHboundFunc);

				if (End != NotesByChannel[Lane].end())
					++End;
			}

			for (auto mp = Start; mp != End; ++mp)
			{
				auto m = *mp;
				if (!m->IsJudgable()) continue;
				if (MechanicsSet->OnReleaseLane(Time, &(*m), Lane)) // Are we done judging..?
					break;
			}
		}

		ScoreKeeper* PlayerContext::GetScoreKeeper() const
		{
			return PlayerScoreKeeper.get();
		}

		std::shared_ptr<ScoreKeeper> PlayerContext::GetScoreKeeperShared() const
		{
			return PlayerScoreKeeper;
		}

		void PlayerContext::SetCanJudge(bool canjudge)
		{
			JudgeNotes = canjudge;
		}

		bool PlayerContext::CanJudge()
		{
			return JudgeNotes;
		}

		void PlayerContext::SetUnwarpedTime(double time)
		{
			ChartState.ResetNotes();
			ChartState.DisableNotesUntil(time);
		}

		int PlayerContext::GetCurrentGaugeType() const
		{
			return Parameters.GaugeType;
		}

		int PlayerContext::GetCurrentScoreType() const
		{
			return Parameters.GetScoringType();
		}

		int PlayerContext::GetCurrentSystemType() const
		{
			return MechanicsSet->GetTimingKind();
		}

		double PlayerContext::GetDrift() const
		{
			return Drift;
		}

		double PlayerContext::GetJudgeOffset() const
		{
			return JudgeOffset;
		}

		double PlayerContext::GetRate() const
		{
			return Parameters.Rate;
		}

		void PlayerContext::JudgeLane(uint32_t Lane, double Time)
		{
			GearKeyEvent(Lane, true);

			if (!CanJudge())
				return;

			auto &Notes = ChartState.NotesTimeOrdered[Lane];

			auto Start = Notes.begin();
			auto End = Notes.end();

			auto threshold = (PlayerScoreKeeper->usesO2() ?
				PlayerScoreKeeper->getJudgmentCutoffMS() :
				(PlayerScoreKeeper->getJudgmentCutoffMS() / 1000.0));

			// Use this optimization when we can make sure vertical properly aligns up with time, as with ReleaseLane.
			//if (ChartState.IsNoteTimeSorted())
			{
				auto timeLower = (Time - threshold);
				auto timeHigher = (Time + threshold);

				Start = std::lower_bound(Notes.begin(), Notes.end(), timeLower, NoteLboundFuncHead);
				End = std::upper_bound(Notes.begin(), Notes.end(), timeHigher, NoteHboundFuncHead);
			}

			bool notJudged = true;

			Gear.ClosestNoteMS[Lane] = MsDisplayMargin;

			for (auto mp = Start; mp != End; ++mp)
			{
				auto m = *mp;
				double dev = (Time - m->GetStartTime()) * 1000;
				double tD = abs(dev);

				Gear.ClosestNoteMS[Lane] = std::min(tD, (double)Gear.ClosestNoteMS[Lane]);

				if (Gear.ClosestNoteMS[Lane] >= MsDisplayMargin)
					Gear.ClosestNoteMS[Lane] = 0;

				if (!m->IsJudgable())
					continue;

				if (MechanicsSet->OnPressLane(Time, &(*m), Lane))
				{
					notJudged = false;
					return; // we judged a note in this lane, so we're done.
				}
			}

			if (notJudged)
				if (Gear.CurrentKeysounds[Lane])
					PlayKeysound(Gear.CurrentKeysounds[Lane]->GetSound());
		}

		bool PlayerContext::HasFailed() const
		{
			return PlayerScoreKeeper->isStageFailed(GetCurrentGaugeType()) && !Parameters.NoFail;
		}

		bool PlayerContext::HasDelayedFailure()
		{
			return PlayerScoreKeeper->hasDelayedFailure(GetCurrentGaugeType());
		}

		double PlayerContext::GetClosestNoteTime(int lane) const
		{
			if (lane >= 0 && lane < GetChannelCount())
				return Gear.ClosestNoteMS[lane];
			else
				return std::numeric_limits<double>::infinity();
		}

		void PlayerContext::SetUserMultiplier(float Multip)
		{
			Parameters.UserSpeedMultiplier = Multip;
		}

		void PlayerContext::SetPlayableData(std::shared_ptr<VSRG::Difficulty> diff, double Drift)
		{
			CfgVar JudgeOffsetMS("JudgeOffsetMS");
			double DesiredDefaultSpeed = Configuration::GetSkinConfigf("DefaultSpeedUnits");
			Game::VSRG::ESpeedType Type = (Game::VSRG::ESpeedType)(int)Configuration::GetSkinConfigf("DefaultSpeedKind");

			CurrentDiff = diff;
			this->Drift = Drift;
			JudgeOffset = JudgeOffsetMS / 1000.0;

			// this has to happen after the setup so we can use the effective parameters!
			// use data from the replay if one is loaded
			if (PlayerReplay.IsLoaded()) {
				Parameters = PlayerReplay.GetEffectiveParameters();
				DesiredDefaultSpeed = Parameters.UserSpeedMultiplier;

				// treat desired default speed as the actual multiplier
				Type = SPEEDTYPE_MULTIPLIER;

				PlayerReplay.AddPlaybackListener([this](Replay::Entry entry) {
					this->OnPlayerKeyEvent(entry.Time + this->GetDrift(), entry.Down, entry.Lane);
				});
			}

			auto d = Parameters.Setup(DesiredDefaultSpeed, Type, Drift, diff);
			ChartState = *d;
			ChartState.PrepareOrderedNotes(); // Invalid ordered notes until we do this.
			delete d;
			
			SetupMechanics();

			// setupmechanics sets the effective gauge/etc so we have to do that first
			PlayerReplay.SetSongData(
				Parameters,
				Type,
				diff->Data->FileHash,
				diff->Data->IndexInFile
			);
		}

		std::vector<AutoplaySound> PlayerContext::GetBgmData()
		{
			CfgVar DisableKeysounds("DisableKeysounds");

			// Load up BGM events
			auto BGMs = CurrentDiff->Data->BGMEvents;
			if (DisableKeysounds)
				NoteTransform::MoveKeysoundsToBGM(CurrentDiff->Channels, ChartState.Notes, BGMs, Drift);

			return BGMs;
		}

		double PlayerContext::HasSongFinished(double time) const
		{
			double wt = ChartState.GetWarpedSongTime(time);
			double cutoff;

			if (PlayerScoreKeeper->usesO2()) { // beat-based judgements 
				double curBPS = ChartState.GetBpsAt(time);
				double cutoffspb = 1 / curBPS;

				cutoff = cutoffspb * PlayerScoreKeeper->getLateMissCutoffMS();
			} 
			else // time-based judgments
				cutoff = PlayerScoreKeeper->getLateMissCutoffMS() / 1000.0;

			return wt > CurrentDiff->Duration + cutoff;
		}

		double PlayerContext::GetWaitingTime()
		{
			CfgVar WaitingTime("WaitingTime");
			return std::max(std::max(WaitingTime > 1.0? WaitingTime : 1.5, 0.0), CurrentDiff ? -CurrentDiff->Offset : 0);
		}


		void PlayerContext::GearKeyEvent(uint32_t Lane, bool KeyDown)
		{
			if (OnGearKeyEvent) {
				OnGearKeyEvent(Lane, KeyDown, PlayerNumber);
			}
		}

		void PlayerContext::Update(double SongTime)
		{
			auto driftedTime = SongTime - Drift;
			auto Beat = ChartState.GetBeatAt(ChartState.GetWarpedSongTime(driftedTime));
			PlayerNoteskin.Update(SongTime - LastUpdateTime, Beat);
			LastUpdateTime = SongTime;
			PlayerReplay.Update(SongTime - Drift);
			RunMeasures(SongTime - Drift);
		}

		void PlayerContext::Render(double SongTime)
		{
			int rnc = DrawMeasures(SongTime - Drift);

		}

		double PlayerContext::GetLifePST() const
		{
			auto LifebarType = GetCurrentGaugeType();
			auto lifebar_amount = PlayerScoreKeeper->getLifebarAmount(LifebarType);
			if (LifebarType == LT_GROOVE || LifebarType == LT_EASY)
				return std::max(2, int(floor(lifebar_amount * 50) * 2));
			else
				return ceil(lifebar_amount * 50) * 2;
		}

		std::string PlayerContext::GetPacemakerText(bool bm) const
		{
			if (bm) {
				auto bmpm = PlayerScoreKeeper->getAutoPacemaker();
				return bmpm.first;
			}
			else {
				auto pm = PlayerScoreKeeper->getAutoRankPacemaker();
				return pm.first;
			}
		}

		int PlayerContext::GetPacemakerValue(bool bm) const
		{
			if (bm) {
				auto bmpm = PlayerScoreKeeper->getAutoPacemaker();
				return bmpm.second;
			}
			else {
				auto pm = PlayerScoreKeeper->getAutoRankPacemaker();
				return pm.second;
			}
		}

		void PlayerContext::DrawBarlines(double CurrentVertical, double UserSpeedMultiplier)
		{
			for (auto i : ChartState.MeasureBarlines)
			{
				double realV = (CurrentVertical - i) * UserSpeedMultiplier +
					PlayerNoteskin.GetBarlineOffset() * sign(UserSpeedMultiplier) + GetJudgmentY();
				if (realV > 0 && realV < ScreenWidth)
				{
					Barline->SetLocation(Vec2(PlayerNoteskin.GetBarlineStartX(), realV),
						Vec2(PlayerNoteskin.GetBarlineStartX() + PlayerNoteskin.GetBarlineWidth(), realV));
					Barline->Render();
				}
			}
		}

		Mat4 id;

		int PlayerContext::DrawMeasures(double song_time)
		{
			int rnc = 0;
			/*
				DrawMeasures should get the unwarped song time.
				Internally, it uses warped song time.
			*/
			auto wt = ChartState.GetWarpedSongTime(song_time);

			// note Y displacement at song_time
			auto chart_displacement = ChartState.GetChartDisplacementAt(wt);

			// effective speed multiplier
			auto chart_multiplier = GetAppliedSpeedMultiplier(song_time);
			auto effective_chart_speed_multiplier = chart_multiplier * Parameters.UserSpeedMultiplier;

			// since + is downward, - is upward!
			bool upscrolling = effective_chart_speed_multiplier < 0;

			if (PlayerNoteskin.IsBarlineEnabled())
				DrawBarlines(chart_displacement, effective_chart_speed_multiplier);

			// Set some parameters...
			Renderer::SetShaderParameters(false, false, true, true, false, false, Parameters.GetHiddenMode());

			// Sudden = 1, Hidden = 2, flashlight = 3 (Defined in the shader)
			if (Parameters.GetHiddenMode())
			{
				Renderer::Shader::SetUniform(
					Renderer::DefaultShader::GetUniform(Renderer::U_HIDCENTER), 
					Parameters.GetHiddenCenter());
				Renderer::Shader::SetUniform(
					Renderer::DefaultShader::GetUniform(Renderer::U_HIDSIZE), 
					Parameters.GetHiddenTransitionSize());
				Renderer::Shader::SetUniform(
					Renderer::DefaultShader::GetUniform(Renderer::U_HIDFLSIZE), 
					Parameters.GetHiddenCenterSize());
			}

			Renderer::SetPrimitiveQuadVBO();
			auto &Notes = ChartState.NotesVerticallyOrdered;
			auto jy = GetJudgmentY();

			for (auto k = 0U; k < CurrentDiff->Channels; k++)
			{
				// From the note's vertical StaticVert transform to position on screen.
				auto Locate = [&](double StaticVert) -> double
				{
					return (chart_displacement - StaticVert) * effective_chart_speed_multiplier + jy;
				};

				auto Start = Notes[k].begin();
				auto End = Notes[k].end();

				// We've got guarantees about our note locations.
				//if (ChartState.IsNoteTimeSorted())
				{
					/* Find the location of the first/next visible regular note */
					auto LocPredicate = [&](const TrackNote* A, double TrackDisplacement) -> bool
					{
						if (!upscrolling)
							return TrackDisplacement < Locate(A->GetVertical());
						else // Signs are switched. We need to preserve the same order.
							return TrackDisplacement > Locate(A->GetVertical());
					};

					// Signs are switched. Doesn't begin by the first note closest to the lower edge, but the one closest to the higher edge.
					if (!upscrolling)
						Start = std::lower_bound(
							Notes[k].begin(), 
							Notes[k].end(), 
							ScreenHeight + PlayerNoteskin.GetNoteOffset(), 
							LocPredicate);
					else
						Start = std::lower_bound(
							Notes[k].begin(),
							Notes[k].end(), 
							0 - PlayerNoteskin.GetNoteOffset(), 
							LocPredicate);

					// Locate the first hold that we can draw in this range
					/*
						Since our object is on screen, our hold may be on screen but may not be this note
						since only head locations are used.
						Find this possible hold by checking if it intersects the screen.
					*/
					if (Start != Notes[k].begin()) {
						auto i = Start - 1;//std::reverse_iterator<std::vector<TrackNote>::iterator>(Start);
						if ((*i)->IsHold() && (*i)->IsVisible())
						{
							auto Vert = Locate((*i)->GetVertical());
							auto VertEnd = Locate((*i)->GetHoldEndVertical());
							if (IntervalsIntersect(0, ScreenHeight, std::min(Vert, VertEnd), std::max(Vert, VertEnd)))
							{
								Start = i;
							}
						}
					}

					// Find the note that is out of the drawing range
					// As before. Top becomes bottom, bottom becomes top.
					if (!upscrolling)
						End = std::lower_bound(Notes[k].begin(), Notes[k].end(), 0 - PlayerNoteskin.GetNoteOffset(), LocPredicate);
					else
						End = std::lower_bound(Notes[k].begin(), Notes[k].end(), ScreenHeight + PlayerNoteskin.GetNoteOffset(), LocPredicate);
				}

				// Now, draw them.
				for (auto mp = Start; mp != End; ++mp)
				{
					auto m = *mp;
					double Vertical = 0;
					double VerticalHoldEnd;

					// Don't attempt drawing this object if not visible.
					if (!m->IsVisible())
						continue;

					Vertical = Locate(m->GetVertical());
					VerticalHoldEnd = Locate(m->GetHoldEndVertical());

					// Old check method that doesn't rely on a correct vertical ordering.
					/*if (!ChartState.IsNoteTimeSorted())
					{
						if (m->IsHold())
						{
							if (!IntervalsIntersect(0, ScreenHeight, 
								std::min(Vertical, VerticalHoldEnd), std::max(Vertical, VerticalHoldEnd))) continue;
						}
						else
						{
							if (Vertical < -PlayerNoteskin.GetNoteOffset() || 
								Vertical > ScreenHeight + PlayerNoteskin.GetNoteOffset()) continue;
						}
					}*/

					double JudgeY;

					// LR2 style keep-on-the-judgment-line
					bool AboveLine = Vertical < GetJudgmentY();
					if (!(AboveLine ^ upscrolling) && m->IsJudgable())
						JudgeY = GetJudgmentY();
					else
						JudgeY = Vertical;

					// We draw the body first, so that way the heads get drawn on top
					if (m->IsHold())
					{
						// todo: move this note state determination to the note itself 
						enum : int { Failed, Active, BeingHit, SuccesfullyHit };
						int Level = -1;

						if (m->IsEnabled() && !m->FailedHit())
							Level = Active;
						if (!m->IsEnabled() && m->FailedHit())
							Level = Failed;
						if (!m->IsEnabled() && !m->FailedHit() && !m->WasHit())
							Level = Failed;
						if (m->IsEnabled() && m->WasHit() && !m->FailedHit())
							Level = BeingHit;
						if (!m->IsEnabled() && m->WasHit() && !m->FailedHit())
							Level = SuccesfullyHit;

						double Pos;
						double Size;
						// If we're being hit and..
						bool decrease_hold_size = PlayerNoteskin.ShouldDecreaseHoldSizeWhenBeingHit() && Level == 2;
						auto reference_point = 0.0f;
						if (decrease_hold_size)
						{
							reference_point = JudgeY;
						}
						else // We were failed, not being hit or were already hit
						{
							reference_point = Vertical;
						}

						Pos = (VerticalHoldEnd + reference_point) / 2;
						Size = VerticalHoldEnd - reference_point;

						PlayerNoteskin.DrawHoldBody(k, Pos, Size, Level);
						PlayerNoteskin.DrawHoldTail(*m, k, VerticalHoldEnd, Level);

						if (PlayerNoteskin.AllowDanglingHeads() || decrease_hold_size)
							PlayerNoteskin.DrawHoldHead(*m, k, JudgeY, Level);
						else
							PlayerNoteskin.DrawHoldHead(*m, k, Vertical, Level);
					}
					else
					{
						if (PlayerNoteskin.AllowDanglingHeads())
							PlayerNoteskin.DrawNote(*m, k, JudgeY);
						else
							PlayerNoteskin.DrawNote(*m, k, Vertical);
					}

					rnc++; // Rendered note count increases...
				}
			}

			/* Clean up */
			Renderer::SetShaderParameters(false, false, true, true, false, false, 0);
			Renderer::FinalizeDraw();


			if (DebugNoteRendering) {
				fnt->Render(Utility::Format("NOTES RENDERED: %d\nSORTEDTIME: %d\nRNG: %f to %f\nMULT/EFFECTIVEMULT/SPEED: %f/%f/%f",
					rnc,
					true,//ChartState.IsNoteTimeSorted(),
					chart_displacement, chart_displacement + ScreenHeight,
					chart_multiplier, effective_chart_speed_multiplier, GetCurrentVerticalSpeed() * effective_chart_speed_multiplier), Vec2(0, 0));
			}
			return rnc;
		}

		

}

}
