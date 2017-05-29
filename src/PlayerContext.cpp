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
		PlayerContext::PlayerContext(int pn, Game::VSRG::Parameters p) : PlayerNoteskin(this)
		{
			PlayerNumber = pn;
			Drift = 0;

			JudgeNotes = true;
			LifebarType = LT_AUTO;
			ScoringType = ST_EXP3;

			PlayerScoreKeeper = std::make_shared<ScoreKeeper>();
			Parameters = p;

			Hidden = {};

			Gear = {};

			if (!fnt && DebugNoteRendering) {
				fnt = new BitmapFont();
				fnt->LoadSkinFontImage("font.tga", Vec2(6, 15), Vec2(8, 16), Vec2(6, 15), 0);
			}
		}

		void PlayerContext::Init() {
			PlayerNoteskin.SetupNoteskin(ChartData.HasTurntable, CurrentDiff->Channels);
		}

		void PlayerContext::Validate() {
			PlayerNoteskin.Validate();
			MsDisplayMargin = (Configuration::GetSkinConfigf("HitErrorDisplayLimiter"));


			if (!BindKeysToLanes(ChartData.HasTurntable))
				if (!BindKeysToLanes(!ChartData.HasTurntable))
					Log::LogPrintf("Couldn't get valid bindings for current key count %d.\n", GetChannelCount());


			if (PlayerNoteskin.IsBarlineEnabled())
				Barline = std::make_unique<Line>();
		}


		const GameChartData& PlayerContext::GetPlayerState()
		{
			return ChartData;
		}

		void PlayerContext::SetupMechanics()
		{
			using namespace Game::VSRG;
			bool disable_forced_release = false;

			// This must be done before setLifeTotal in order for it to work.
			PlayerScoreKeeper->setMaxNotes(CurrentDiff->TotalScoringObjects);

			// JudgeScale, Stepmania and OD can't be run together - only one can be set.
			auto TimingInfo = CurrentDiff->Data->TimingInfo.get();

			// Pick a timing system
			if (Parameters.SystemType == VSRG::TI_NONE) {
				if (TimingInfo) {
					// Automatic setup
					Parameters.SystemType = TimingInfo->GetType();
				}
				else {
					Log::Printf("Null timing info - assigning raindrop defaults.\n");
					Parameters.SystemType = VSRG::TI_RAINDROP;
					// pick raindrop system for null Timing Info
				}
			}

			// If we got just assigned one or was already requested
			// unlikely: timing info type is none? what
		retry:
			if (Parameters.SystemType != VSRG::TI_NONE) {
				// Player requested a specific subsystem
				if (Parameters.SystemType == VSRG::TI_BMS || Parameters.SystemType == VSRG::TI_RDAC) {
					disable_forced_release = true;
					ScoringType = ST_EX;
					UsedTimingType = TT_TIME;
					if (TimingInfo->GetType() == VSRG::TI_BMS) {
						auto Info = static_cast<VSRG::BMSChartInfo*> (TimingInfo);
						if (!Info->PercentualJudgerank)
							PlayerScoreKeeper->setJudgeRank(Info->JudgeRank);
						else
							PlayerScoreKeeper->setJudgeScale(Info->JudgeRank / 100.0);
					}
					else {
						PlayerScoreKeeper->setJudgeRank(2);
					}
				}
				else if (Parameters.SystemType == VSRG::TI_O2JAM) {
					ScoringType = ST_O2JAM;
					UsedTimingType = TT_BEATS;
					PlayerScoreKeeper->setJudgeRank(-100);
				}
				else if (Parameters.SystemType == VSRG::TI_OSUMANIA) {
					ScoringType = ST_OSUMANIA;
					UsedTimingType = TT_TIME;
					if (TimingInfo->GetType() == VSRG::TI_OSUMANIA) {
						auto InfoOM = static_cast<VSRG::OsumaniaChartInfo*> (TimingInfo);
						PlayerScoreKeeper->setODWindows(InfoOM->OD);
					}
					else PlayerScoreKeeper->setODWindows(7);
				}
				else if (Parameters.SystemType == VSRG::TI_STEPMANIA) {
					disable_forced_release = true;
					UsedTimingType = TT_TIME;
					ScoringType = ST_DP;
					PlayerScoreKeeper->setSMJ4Windows();
				}
				else if (Parameters.SystemType == VSRG::TI_RAINDROP) {
					ScoringType = ST_EXP3;
					LifebarType = LT_STEPMANIA;
				}
			}
			else
			{
				Log::Printf("System picked was none - on purpose. Defaulting to raindrop.\n");
				Parameters.SystemType = VSRG::TI_RAINDROP;
				goto retry;
			}

			// Timing System is set up. Set up life bar
			if (Parameters.GaugeType == LT_AUTO) {
				using namespace VSRG;
				switch (Parameters.SystemType) {
				case TI_BMS:
				case TI_RAINDROP:
				case TI_RDAC:
					Parameters.GaugeType = LT_GROOVE;
					break;
				case TI_O2JAM:
					Parameters.GaugeType = LT_O2JAM;
					break;
				case TI_OSUMANIA:
				case TI_STEPMANIA:
					Parameters.GaugeType = LT_STEPMANIA;
					break;
				default:
					throw std::runtime_error("Invalid requested system.");
				}
			}

			switch (Parameters.GaugeType) {
			case LT_STEPMANIA:
				LifebarType = LT_STEPMANIA; // Needs no setup.
				break;

			case LT_O2JAM:
				if (TimingInfo->GetType() == VSRG::TI_O2JAM) {
					auto InfoO2 = static_cast<VSRG::O2JamChartInfo*> (TimingInfo);
					PlayerScoreKeeper->setO2LifebarRating(InfoO2->Difficulty);
				} // else by default
				LifebarType = LT_O2JAM; // By default, HX
				break;

			case LT_GROOVE:
			case LT_DEATH:
			case LT_EASY:
			case LT_EXHARD:
			case LT_SURVIVAL:
				if (TimingInfo->GetType() == VSRG::TI_BMS) { // Only needs setup if it's a BMS file
					auto Info = static_cast<VSRG::BMSChartInfo*> (TimingInfo);
					if (Info->IsBMSON)
						PlayerScoreKeeper->setLifeTotal(NAN, Info->GaugeTotal);
					else
						PlayerScoreKeeper->setLifeTotal(Info->GaugeTotal);
				}
				else // by raindrop defaults
					PlayerScoreKeeper->setLifeTotal(-1);
				LifebarType = (LifeType)Parameters.GaugeType;
				break;
			case LT_NORECOV:
				LifebarType = LT_NORECOV;
				break;
			default:
				throw std::runtime_error("Invalid gauge type recieved");
			}

			/*
				If we're on TT_BEATS we've got to recalculate all note positions to beats,
				and use mechanics that use TT_BEATS as its timing type.
			*/

			if (UsedTimingType == TT_TIME)
			{
				if (Parameters.SystemType == VSRG::TI_RDAC)
				{
					Log::Printf("RAINDROP ARCADE STAAAAAAAAART!\n");
					MechanicsSet = std::make_unique<RaindropArcadeMechanics>();
				}
				else {
					Log::Printf("Using raindrop mechanics set!\n");
					// Only forced release if not a bms or a stepmania chart.
					MechanicsSet = std::make_unique<RaindropMechanics>(!disable_forced_release);
				}
			}
			else if (UsedTimingType == TT_BEATS)
			{
				Log::Printf("Using o2jam mechanics set!\n");
				MechanicsSet = std::make_unique<O2JamMechanics>();
				NoteTransform::TransformToBeats(GetChannelCount(), ChartData.NotesByChannel, ChartData.BPS);
			}

			MechanicsSet->Setup(CurrentDiff.get(), PlayerScoreKeeper);
			MechanicsSet->HitNotify = std::bind(&PlayerContext::HitNote, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
			MechanicsSet->MissNotify = std::bind(&PlayerContext::MissNote, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
			MechanicsSet->IsLaneKeyDown = std::bind(&PlayerContext::GetGearLaneState, this, std::placeholders::_1);
			MechanicsSet->SetLaneHoldingState = std::bind(&PlayerContext::SetLaneHoldState, this, std::placeholders::_1, std::placeholders::_2);
			MechanicsSet->PlayNoteSoundEvent = PlayKeysound;
			// We're set - setup all of the variables that depend on mechanics, scoring etc.. to their initial values.
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

			if (KeyDown)
			{
				JudgeLane(GearIndex, GetChartTimeAt(Time));
				Gear.IsPressed[GearIndex] = true;
			}
			else
			{
				ReleaseLane(GearIndex, GetChartTimeAt(Time));
				Gear.IsPressed[GearIndex] = false;
			}
		}


		bool PlayerContext::IsFailEnabled() const
		{
			return !Parameters.NoFail;
		}

		bool PlayerContext::IsUpscrolling() const
		{
			return GetAppliedSpeedMultiplier(LastUpdateTime) < 0;
		}


		double PlayerContext::GetCurrentBPM() const
		{
			return ChartData.GetBpmAt(GetWarpedSongTime());
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
			return ChartData.ConnectedDifficulty->Duration;
		}

		double PlayerContext::GetBeatDuration() const
		{
			return ChartData.GetBeatAt(GetDuration());
		}

		int PlayerContext::GetChannelCount() const
		{
			return ChartData.ConnectedDifficulty->Channels;
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
			return ChartData.HasTurntable;
		}

		double PlayerContext::GetAppliedSpeedMultiplier(double Time) const
		{
			auto sm = ChartData.GetSpeedMultiplierAt(Time);
			if (Parameters.Upscroll)
				return -sm;
			else
				return sm;
		}

		double PlayerContext::GetCurrentBeat() const
		{
			return ChartData.GetBeatAt(LastUpdateTime);
		}

		double PlayerContext::GetUserMultiplier() const
		{
			return Parameters.SpeedMultiplier;
		}

		double PlayerContext::GetCurrentVerticalSpeed() const
		{
			return ChartData.GetDisplacementSpeedAt(LastUpdateTime);
		}

		double PlayerContext::GetWarpedSongTime() const
		{
			return ChartData.GetWarpedSongTime(LastUpdateTime);
		}

		void PlayerContext::SetupLua(LuaManager *Env)
		{
			assert(Env != nullptr);

			luabridge::getGlobalNamespace(Env->GetState())
				.beginClass<PlayerContext>("PlayerContext")
				.addProperty("Beat", &PlayerContext::GetCurrentBeat)
				.addProperty("Time", &PlayerContext::GetWarpedSongTime)
				.addProperty("Duration", &PlayerContext::GetDuration)
				.addProperty("BeatDuration", &PlayerContext::GetBeatDuration)
				.addProperty("Channels", &PlayerContext::GetChannelCount)
				.addProperty("BPM", &PlayerContext::GetCurrentBPM)
				.addProperty("CanFail", &PlayerContext::IsFailEnabled)
				.addProperty("HasFailed", &PlayerContext::HasFailed)
				.addProperty("Upscroll", &PlayerContext::IsUpscrolling)
				.addProperty("Speed", &PlayerContext::GetCurrentVerticalSpeed)
				.addProperty("JudgmentY", &PlayerContext::GetJudgmentY)
				.addProperty("Difficulty", &PlayerContext::GetDifficulty)
				.addProperty("Turntable", &PlayerContext::GetUsesTurntable)
				.addProperty("SpeedMultiplier", &PlayerContext::GetUserMultiplier, &PlayerContext::SetUserMultiplier)
				.addProperty("HasTurntable", &PlayerContext::GetUsesTurntable)
				.addProperty("LifebarPercent", &PlayerContext::GetLifePST)
				.addProperty("Difficulty", &PlayerContext::GetDifficulty)
				.addProperty("Number", &PlayerContext::GetPlayerNumber)
				.addProperty("Score", &PlayerContext::GetScore)
				.addProperty("Combo", &PlayerContext::GetCombo)
				.addFunction("GetPacemakerText", &PlayerContext::GetPacemakerText)
				.addFunction("GetPacemakerValue", &PlayerContext::GetPacemakerValue)
				.addFunction("IsHoldActive", &PlayerContext::GetIsHeldKey)
				.addFunction("GetClosestNoteTime", &PlayerContext::GetClosestNoteTime)
				.addProperty("Scorekeeper", &PlayerContext::GetScoreKeeper)
				.endClass();
		}

		double PlayerContext::GetScore() const
		{
			return PlayerScoreKeeper->getScore(ScoringType);
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

			auto CurrentDiff = ChartData.ConnectedDifficulty;

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
			int Judgment = PlayerScoreKeeper->hitNote(TimeOff);

			if (Animations) {
				if (Animations->GetEnv()->CallFunction("HitEvent", 6))
				{
					Animations->GetEnv()->PushArgument(Judgment);
					Animations->GetEnv()->PushArgument(TimeOff);
					Animations->GetEnv()->PushArgument((int)Lane + 1);
					Animations->GetEnv()->PushArgument(IsHold);
					Animations->GetEnv()->PushArgument(IsHoldRelease);
					Animations->GetEnv()->PushArgument(PlayerNumber);
					Animations->GetEnv()->RunFunction();
				}

				if (PlayerScoreKeeper->getMaxNotes() == PlayerScoreKeeper->getScore(ST_NOTES_HIT))
					Animations->DoEvent("OnFullComboEvent");
			}

			//OnHit(TimeOff, Lane, IsHold, IsHoldRelease);
		}

		void PlayerContext::MissNote(double TimeOff, uint32_t Lane, bool IsHold, bool dont_break_combo, bool early_miss)
		{
			PlayerScoreKeeper->missNote(dont_break_combo, early_miss);

			if (IsHold)
				Gear.HeldKey[Lane] = false;

			if (Animations) {
				if (Animations->GetEnv()->CallFunction("MissEvent", 4))
				{
					Animations->GetEnv()->PushArgument(TimeOff);
					Animations->GetEnv()->PushArgument((int)Lane + 1);
					Animations->GetEnv()->PushArgument(IsHold);
					Animations->GetEnv()->PushArgument(PlayerNumber);
					Animations->GetEnv()->RunFunction();
				}
			}

			//OnMiss(TimeOff, Lane, IsHold, dont_break_combo, early_miss);
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
			if (UsedTimingType == TT_BEATS) {
				return ChartData.GetBeatAt(time);
			}
			else if (UsedTimingType == TT_TIME) {
				return time;
			}
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
			auto &NotesByChannel = ChartData.NotesByChannel;

			for (auto k = 0U; k < CurrentDiff->Channels; k++)
			{
				for (auto m = NotesByChannel[k].begin(); m != NotesByChannel[k].end(); ++m) {
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
						RunAuto(&*m, usedTime, k);
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

			auto &NotesByChannel = ChartData.NotesByChannel;
			auto Start = NotesByChannel[Lane].begin();
			auto End = NotesByChannel[Lane].end();

			// Use this optimization when we can make sure vertical properly aligns up with time.
			if (UseNoteOptimization())
			{
				// In comparison to the regular compare function, since end times are what matter with holds (or lift events, where start == end)
				// this does the job as it should instead of comparing start times where hold tails would be completely ignored.
				auto LboundFunc = [](const TrackNote &A, const double &B) -> bool
				{
					return A.GetEndTime() < B;
				};
				auto HboundFunc = [](const double &A, const TrackNote &B) -> bool
				{
					return A < B.GetEndTime();
				};

				auto timeLower = (Time - (PlayerScoreKeeper->usesO2() ? PlayerScoreKeeper->getMissCutoffMS() : (PlayerScoreKeeper->getMissCutoffMS() / 1000.0)));
				auto timeHigher = (Time + (PlayerScoreKeeper->usesO2() ? PlayerScoreKeeper->getJudgmentCutoff() : (PlayerScoreKeeper->getJudgmentCutoff() / 1000.0)));

				Start = std::lower_bound(NotesByChannel[Lane].begin(), NotesByChannel[Lane].end(), timeLower, LboundFunc);

				// Locate the first hold that we can judge in this range (Pending holds. Similar to what was done when drawing.)
				auto rStart = std::reverse_iterator<std::vector<TrackNote>::iterator>(Start);
				for (auto i = rStart; i != NotesByChannel[Lane].rend(); ++i)
				{
					if (i->IsHold() && i->IsEnabled() && i->IsJudgable() && i->WasHit() && !i->FailedHit())
						Start = i.base() - 1;
				}

				End = std::upper_bound(NotesByChannel[Lane].begin(), NotesByChannel[Lane].end(), timeHigher, HboundFunc);

				if (End != NotesByChannel[Lane].end())
					++End;
			}

			for (auto m = Start; m != End; ++m)
			{
				if (!m->IsJudgable()) continue;
				if (MechanicsSet->OnReleaseLane(Time, &(*m), Lane)) // Are we done judging..?
					break;
			}
		}

		ScoreKeeper* PlayerContext::GetScoreKeeper() const
		{
			return PlayerScoreKeeper.get();
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
			ChartData.DisableNotesUntil(time);
		}

		int PlayerContext::GetCurrentGaugeType()
		{
			return LifebarType;
		}

		int PlayerContext::GetCurrentScoreType()
		{
			return ScoringType;
		}

		int PlayerContext::GetCurrentSystemType()
		{
			return UsedTimingType;
		}

		bool PlayerContext::UseNoteOptimization()
		{
			return !ChartData.Warps.size()  // The actual time is scrambled
				&& !ChartData.HasNegativeScroll; // Draw order may be incorrect
		}

		void PlayerContext::JudgeLane(uint32_t Lane, double Time)
		{
			GearKeyEvent(Lane, true);

			if (!CanJudge())
				return;

			auto &Notes = ChartData.NotesByChannel[Lane];

			auto Start = Notes.begin();
			auto End = Notes.end();

			// Use this optimization when we can make sure vertical properly aligns up with time, as with ReleaseLane.
			if (UseNoteOptimization())
			{
				auto timeLower = (Time - (PlayerScoreKeeper->usesO2() ? PlayerScoreKeeper->getMissCutoffMS() : (PlayerScoreKeeper->getMissCutoffMS() / 1000.0)));
				auto timeHigher = (Time + (PlayerScoreKeeper->usesO2() ? PlayerScoreKeeper->getJudgmentCutoff() : (PlayerScoreKeeper->getJudgmentCutoff() / 1000.0)));

				Start = std::lower_bound(Notes.begin(), Notes.end(), timeLower);
				End = std::upper_bound(Notes.begin(), Notes.end(), timeHigher);
			}

			bool notJudged = true;

			Gear.ClosestNoteMS[Lane] = MsDisplayMargin;

			for (auto m = Start; m != End; ++m)
			{
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
			return PlayerScoreKeeper->isStageFailed(LifebarType) && !Parameters.NoFail;
		}

		bool PlayerContext::HasDelayedFailure()
		{
			return PlayerScoreKeeper->hasDelayedFailure(LifebarType);
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
			Parameters.SpeedMultiplier = Multip;
		}

		void PlayerContext::SetSceneEnvironment(std::shared_ptr<SceneEnvironment> env)
		{
			Animations = env;
		}

		void PlayerContext::SetPlayableData(std::shared_ptr<VSRG::Difficulty> diff, double Drift,
			double DesiredDefaultSpeed, int Type)
		{
			CurrentDiff = diff;
			this->Drift = Drift;

			/*
			 * 		There are four kinds of speed modifiers:
			 * 		-CMod (Keep speed the same through the song, equal to a constant)
			 * 		-MMod (Find highest speed and set multiplier to such that the highest speed is equal to a constant)
			 *		-First (Find the first speed in the chart, and set multiplier to such that the first speed is equal to a constant)
			 *		-Mode (Find the speed that lasts the most in the chart, set multiplier based on that)
			 *
			 *		The calculations are done ahead, and while SpeedConstant = 0 either MMod or first are assumed
			 *		but only if there's a constant specified by the user.
			 */

			DesiredDefaultSpeed /= Parameters.Rate;

			

			if (DesiredDefaultSpeed)
			{
				if (Type == SPEEDTYPE_CMOD) // cmod
				{
					Parameters.SpeedMultiplier = 1;

					double spd = Parameters.GreenNumber ? 1000 : DesiredDefaultSpeed;

					ChartData = VSRG::GameChartData::FromDifficulty(CurrentDiff.get(), Drift, spd);
				} else
					ChartData = VSRG::GameChartData::FromDifficulty(CurrentDiff.get(), Drift);

				// if GN is true, Default Speed = GN!
				// Convert GN to speed.
				if (Parameters.GreenNumber) {
					double first_height = std::numeric_limits<float>::infinity();
					double first_time = std::numeric_limits<float>::infinity();
					for (int i = 0; i < MAX_CHANNELS; i++) {
						if (ChartData.NotesByChannel[i].size() > 0) {
							auto v = ChartData.NotesByChannel[i][0].GetVertical();
							auto t = ChartData.NotesByChannel[i][0].GetStartTime();
							if (v < first_height) {
								first_height = v;
								first_time = t;
							}
						}
					}

					// v0 is normal speed
					// (green number speed * green number time) / (normal speed * normal time)
					// equals
					// playfield distance / note distance
					
					/*auto v0 = ChartData.VSpeeds[0].Value;
					auto hratio = MeasureBaseSpacing / first_height;
					double new_speed = hratio * v0 * first_time / (DesiredDefaultSpeed / 1000);*/
					double new_speed = MeasureBaseSpacing / (DesiredDefaultSpeed / 1000);
					DesiredDefaultSpeed = new_speed;

					if (Type == SPEEDTYPE_CMOD) {
						Parameters.SpeedMultiplier = DesiredDefaultSpeed / 1000;
					}
				}

				if (Type == SPEEDTYPE_MMOD) // mmod
				{
					double speed_max = 0; // Find the highest speed
					for (auto i : ChartData.VSpeeds)
					{
						speed_max = std::max(speed_max, abs(i.Value));
					}

					double Ratio = DesiredDefaultSpeed / speed_max; // How much above or below are we from the maximum speed?
					Parameters.SpeedMultiplier = Ratio;
				}
				else if (Type == SPEEDTYPE_FIRST) // First speed.
				{
					double DesiredMultiplier = DesiredDefaultSpeed / ChartData.VSpeeds[0].Value;
					Parameters.SpeedMultiplier = DesiredMultiplier;
				}
				else if (Type == SPEEDTYPE_MODE) // Most lasting speed.
				{
					std::map <double, double> freq;
					for (auto i = ChartData.VSpeeds.begin(); i != ChartData.VSpeeds.end(); i++)
					{
						if (i + 1 != ChartData.VSpeeds.end())
						{
							freq[i->Value] += (i + 1)->Time - i->Time;
						}
						else freq[i->Value] += abs(CurrentDiff->Duration - i->Time);
					}
					auto max = -std::numeric_limits<float>::infinity();
					auto val = 1000.f;
					for (auto i : freq)
					{
						if (i.second > max)
						{
							max = i.second;
							val = i.first;
						}
					}

					Parameters.SpeedMultiplier = DesiredDefaultSpeed / val;
				}
				else if (Type != SPEEDTYPE_CMOD) // other cases
				{
					double bpsd = 4.0 / (ChartData.BPS[0].Value);
					double Speed = (MeasureBaseSpacing / bpsd);
					double DesiredMultiplier = DesiredDefaultSpeed / Speed;

					Parameters.SpeedMultiplier = DesiredMultiplier;
				}
			}
			else
				ChartData = VSRG::GameChartData::FromDifficulty(CurrentDiff.get(), Drift);

			if (Parameters.Random)
				NoteTransform::Randomize(ChartData.NotesByChannel, CurrentDiff->Channels, CurrentDiff->Data->Turntable);

			SetupMechanics();
		}

		std::vector<AutoplaySound> PlayerContext::GetBgmData()
		{
			CfgVar DisableKeysounds("DisableKeysounds");

			// Load up BGM events
			auto BGMs = CurrentDiff->Data->BGMEvents;
			if (DisableKeysounds)
				NoteTransform::MoveKeysoundsToBGM(CurrentDiff->Channels, ChartData.NotesByChannel, BGMs, Drift);

			return BGMs;
		}

		double PlayerContext::GetMeasureTime(double msr) const
		{
			double beat = 0;

			if (msr <= 0)
			{
				return 0;
			}

			auto whole = int(floor(msr));
			auto fraction = msr - double(whole);
			for (auto i = 0; i < whole; i++)
				beat += CurrentDiff->Data->Measures[i].Length;
			beat += CurrentDiff->Data->Measures[whole].Length * fraction;

			Log::Logf("Warping to measure measure %d at beat %f.\n", whole, beat);

			return ChartData.GetTimeAtBeat(beat);
		}

		double PlayerContext::HasSongFinished(double time) const
		{
			double wt = ChartData.GetWarpedSongTime(time);
			double cutoff;

			if (PlayerScoreKeeper->usesO2()) { // beat-based judgements 

				double curBPS = ChartData.GetBpsAt(time);
				double cutoffspb = 1 / curBPS;

				cutoff = cutoffspb * PlayerScoreKeeper->getMissCutoffMS();
			} 
			else // time-based judgments
				cutoff = PlayerScoreKeeper->getMissCutoffMS() / 1000.0;

			return wt > CurrentDiff->Duration + cutoff;
		}

		double PlayerContext::GetWaitingTime()
		{
			CfgVar WaitingTime("WaitingTime");
			return std::max(std::max(WaitingTime > 1.0? WaitingTime : 1.5, 0.0), CurrentDiff ? -CurrentDiff->Offset : 0);
		}


		void PlayerContext::GearKeyEvent(uint32_t Lane, bool KeyDown)
		{
			if (Animations) {
				if (Animations->GetEnv()->CallFunction("GearKeyEvent", 3))
				{
					Animations->GetEnv()->PushArgument((int)Lane + 1);
					Animations->GetEnv()->PushArgument(KeyDown);
					Animations->GetEnv()->PushArgument(PlayerNumber);

					Animations->GetEnv()->RunFunction();
				}
			}
		}

		void PlayerContext::Update(double SongTime)
		{
			PlayerNoteskin.Update(SongTime - LastUpdateTime, ChartData.GetBeatAt(ChartData.GetWarpedSongTime(SongTime)));
			LastUpdateTime = SongTime;
			RunMeasures(SongTime);
		}

		void PlayerContext::Render(double SongTime)
		{
			int rnc = DrawMeasures(SongTime);

		}

		double PlayerContext::GetLifePST() const
		{
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

		void PlayerContext::DrawBarlines(double CurrentVertical, double SpeedMultiplier)
		{
			for (auto i : ChartData.MeasureBarlines)
			{
				double realV = (CurrentVertical - i) * SpeedMultiplier +
					PlayerNoteskin.GetBarlineOffset() * sign(SpeedMultiplier) + GetJudgmentY();
				if (realV > 0 && realV < ScreenWidth)
				{
					Barline->SetLocation(Vec2(PlayerNoteskin.GetBarlineStartX(), realV),
						Vec2(PlayerNoteskin.GetBarlineStartX() + PlayerNoteskin.GetBarlineWidth(), realV));
					Barline->Render();
				}
			}
		}

		void PlayerContext::UpdateHidden(float AdjustmentSize, float FlashlightRatio)
		{
			/*
				Given the top of the screen being 1, the bottom being -1
				calculate the range for which the current hidden mode is defined.
			*/
			float Center;
			float JudgmentLinePos = GetJudgmentY();
			auto Upscroll = Parameters.Upscroll;

			// Hidden calc
			if (Parameters.HiddenMode)
			{
				float LimPos = -((JudgmentLinePos / ScreenHeight) * 2 - 1); // Frac. of screen
				float AdjustmentSize;

				if (Upscroll)
				{
					Center = -((((ScreenHeight - JudgmentLinePos) / 2 + JudgmentLinePos) / ScreenHeight) * 2 - 1);

					// AdjustmentSize = -( ((ScreenHeight - JudgmentLinePos) / 2 / ScreenHeight) - 1 ); // A quarter of the playing field.

					if (Parameters.HiddenMode == HM_HIDDEN)
					{
						Hidden.ClampHigh = Center;
						Hidden.ClampLow = -1 + AdjustmentSize;
					}
					else if (Parameters.HiddenMode == HM_SUDDEN)
					{
						Hidden.ClampHigh = LimPos - AdjustmentSize;
						Hidden.ClampLow = Center;
					}

					// Invert Hidden Mode.
					if (Parameters.HiddenMode == HM_SUDDEN) Hidden.Mode = HM_HIDDEN;
					else if (Parameters.HiddenMode == HM_HIDDEN) Hidden.Mode = HM_SUDDEN;
					else Hidden.Mode = (Game::VSRG::EHiddenMode)Parameters.HiddenMode;
				}
				else
				{
					Center = -((JudgmentLinePos / 2 / ScreenHeight) * 2 - 1);

					// AdjustmentSize = -( ((JudgmentLinePos) / 2 / ScreenHeight) - 1 ); // A quarter of the playing field.

					// Hidden/Sudden
					if (Parameters.HiddenMode == HM_HIDDEN)
					{
						Hidden.ClampHigh = 1 - AdjustmentSize;
						Hidden.ClampLow = Center;
					}
					else if (Parameters.HiddenMode == HM_SUDDEN)
					{
						Hidden.ClampHigh = Center;
						Hidden.ClampLow = LimPos + AdjustmentSize;
					}

					Hidden.Mode = (Game::VSRG::EHiddenMode)Parameters.HiddenMode;
				}

				if (Parameters.HiddenMode == HM_FLASHLIGHT) // Flashlight
				{
					Hidden.ClampLow = Center - FlashlightRatio;
					Hidden.ClampHigh = Center + FlashlightRatio;
					Hidden.ClampSum = -Center;
					Hidden.ClampFactor = 1 / FlashlightRatio;
				}
				else // Hidden/Sudden
				{
					Hidden.ClampSum = -Hidden.ClampLow;
					Hidden.ClampFactor = 1 / (Hidden.ClampHigh + Hidden.ClampSum);
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
			auto wt = ChartData.GetWarpedSongTime(song_time);

			// note Y displacement at song_time
			auto vert = ChartData.GetDisplacementAt(wt);

			// effective speed multiplier
			auto chartmul = GetAppliedSpeedMultiplier(song_time);
			auto effmul = chartmul * Parameters.SpeedMultiplier;

			// since + is downward, - is upward!
			bool upscrolling = effmul < 0;

			if (PlayerNoteskin.IsBarlineEnabled())
				DrawBarlines(vert, effmul);

			// Set some parameters...
			Renderer::SetShaderParameters(false, false, true, true, false, false, Hidden.Mode);

			// Sudden = 1, Hidden = 2, flashlight = 3 (Defined in the shader)
			if (Hidden.Mode)
			{
				Renderer::Shader::SetUniform(Renderer::DefaultShader::GetUniform(Renderer::U_HIDLOW), Hidden.ClampLow);
				Renderer::Shader::SetUniform(Renderer::DefaultShader::GetUniform(Renderer::U_HIDHIGH), Hidden.ClampHigh);
				Renderer::Shader::SetUniform(Renderer::DefaultShader::GetUniform(Renderer::U_HIDFAC), Hidden.ClampFactor);
				Renderer::Shader::SetUniform(Renderer::DefaultShader::GetUniform(Renderer::U_HIDSUM), Hidden.ClampSum);
			}

			Renderer::SetPrimitiveQuadVBO();
			auto &NotesByChannel = ChartData.NotesByChannel;
			auto jy = GetJudgmentY();

			for (auto k = 0U; k < CurrentDiff->Channels; k++)
			{
				// From the note's vertical StaticVert transform to position on screen.
				auto Locate = [&](double StaticVert) -> double
				{
					return (vert - StaticVert) * effmul + jy;
				};

				auto Start = NotesByChannel[k].begin();
				auto End = NotesByChannel[k].end();

				// We've got guarantees about our note locations.
				if (UseNoteOptimization())
				{
					/* Find the location of the first/next visible regular note */
					auto LocPredicate = [&](const TrackNote &A, double _) -> bool
					{
						if (!upscrolling)
							return _ < Locate(A.GetVertical());
						else // Signs are switched. We need to preserve the same order.
							return _ > Locate(A.GetVertical());
					};

					// Signs are switched. Doesn't begin by the first note closest to the lower edge, but the one closest to the higher edge.
					if (!upscrolling)
						Start = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), ScreenHeight + PlayerNoteskin.GetNoteOffset(), LocPredicate);
					else
						Start = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), 0 - PlayerNoteskin.GetNoteOffset(), LocPredicate);

					// Locate the first hold that we can draw in this range
					/*
						Since our object is on screen, our hold may be on screen but may not be this note
						since only head locations are used.
						Find this possible hold by checking if it intersects the screen.
					*/
					auto rStart = std::reverse_iterator<std::vector<TrackNote>::iterator>(Start);
					for (auto i = rStart; i != NotesByChannel[k].rend(); ++i)
					{
						if (i->IsHold() && i->IsVisible())
						{
							auto Vert = Locate(i->GetVertical());
							auto VertEnd = Locate(i->GetHoldEndVertical());
							if (IntervalsIntersect(0, ScreenHeight, std::min(Vert, VertEnd), std::max(Vert, VertEnd)))
							{
								Start = i.base() - 1;
								break;
							}
						}
					}

					// Find the note that is out of the drawing range
					// As before. Top becomes bottom, bottom becomes top.
					if (!upscrolling)
						End = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), 0 - PlayerNoteskin.GetNoteOffset(), LocPredicate);
					else
						End = std::lower_bound(NotesByChannel[k].begin(), NotesByChannel[k].end(), ScreenHeight + PlayerNoteskin.GetNoteOffset(), LocPredicate);
				}

				// Now, draw them.
				for (auto m = Start; m != End; ++m)
				{
					double Vertical = 0;
					double VerticalHoldEnd;

					// Don't attempt drawing this object if not visible.
					if (!m->IsVisible())
						continue;

					Vertical = Locate(m->GetVertical());
					VerticalHoldEnd = Locate(m->GetHoldEndVertical());

					// Old check method that doesn't rely on a correct vertical ordering.
					if (!UseNoteOptimization())
					{
						if (m->IsHold())
						{
							if (!IntervalsIntersect(0, ScreenHeight, std::min(Vertical, VerticalHoldEnd), std::max(Vertical, VerticalHoldEnd))) continue;
						}
						else
						{
							if (Vertical < -PlayerNoteskin.GetNoteOffset() || Vertical > ScreenHeight + PlayerNoteskin.GetNoteOffset()) continue;
						}
					}

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
						if (decrease_hold_size)
						{
							Pos = (VerticalHoldEnd + JudgeY) / 2;
							Size = VerticalHoldEnd - JudgeY;
						}
						else // We were failed, not being hit or were already hit
						{
							Pos = (VerticalHoldEnd + Vertical) / 2;
							Size = VerticalHoldEnd - Vertical;
						}

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
				fnt->Render(Utility::Format("NOTES RENDERED: %d\nN/O: %d\nRNG: %f to %f\nMULT/EFFECTIVEMULT/SPEED: %f/%f/%f",
					rnc,
					UseNoteOptimization(),
					vert, vert + ScreenHeight,
					chartmul, effmul, GetCurrentVerticalSpeed() * effmul), Vec2(0, 0));
			}
			return rnc;
		}

	}

}
