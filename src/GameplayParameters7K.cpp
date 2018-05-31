#include "pch.h"
#include "Song7K.h"
#include "PlayerChartData.h"
#include "NoteTransformations.h"
#include "VSRGMechanics.h"
#include "ScoreKeeper7K.h"

namespace Game {
	namespace VSRG {

		PlayerChartState* PlayscreenParameters::Setup(
			double DesiredDefaultSpeed, 
			int Type, 
			double Drift, 
			std::shared_ptr<Game::VSRG::Difficulty> CurrentDiff)
		{
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

			PlayerChartState ChartState;

			if (DesiredDefaultSpeed)
			{
				DesiredDefaultSpeed /= Rate;

				if (Type == SPEEDTYPE_CMOD) // cmod
				{
					UserSpeedMultiplier = 1;

					double spd = GreenNumber ? 1000 : DesiredDefaultSpeed;

					ChartState = VSRG::PlayerChartState::FromDifficulty(CurrentDiff.get(), Drift, spd);
				}
				else
					ChartState = VSRG::PlayerChartState::FromDifficulty(CurrentDiff.get(), Drift);

				// if GN is true, Default Speed = GN!
				// Convert GN to speed.
				if (GreenNumber) {

					// v0 is normal speed
					// (green number speed * green number time) / (normal speed * normal time)
					// equals
					// playfield distance / note distance

					// simplifying that gets you dn / tn lol
					double new_speed = PLAYFIELD_SIZE / (DesiredDefaultSpeed / 1000);
					DesiredDefaultSpeed = new_speed;

					if (Type == SPEEDTYPE_CMOD) {
						UserSpeedMultiplier = DesiredDefaultSpeed / 1000;
					}
				}

				if (Type == SPEEDTYPE_MMOD) // mmod
				{
					double speed_max = 0; // Find the highest speed
					for (auto i : ChartState.ScrollSpeeds)
					{
						speed_max = std::max(speed_max, abs(i.Value));
					}

					double Ratio = DesiredDefaultSpeed / speed_max; // How much above or below are we from the maximum speed?
					UserSpeedMultiplier = Ratio;
				}
				else if (Type == SPEEDTYPE_FIRST) // First speed.
				{
					double DesiredMultiplier = DesiredDefaultSpeed / ChartState.ScrollSpeeds[0].Value;
					UserSpeedMultiplier = DesiredMultiplier;
				}
				else if (Type == SPEEDTYPE_MODE) // Most lasting speed.
				{
					std::map <double, double> freq;
					for (auto i = ChartState.ScrollSpeeds.begin(); i != ChartState.ScrollSpeeds.end(); i++)
					{
						if (i + 1 != ChartState.ScrollSpeeds.end())
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

					UserSpeedMultiplier = DesiredDefaultSpeed / val;
				}
				else if (Type != SPEEDTYPE_CMOD) // other cases
				{
					double bpsd = 4.0 / (ChartState.BPS[0].Value);
					double Speed = (UNITS_PER_MEASURE / bpsd);
					double DesiredMultiplier = DesiredDefaultSpeed / Speed;

					UserSpeedMultiplier = DesiredMultiplier;
				}
			}
			else
				ChartState = VSRG::PlayerChartState::FromDifficulty(CurrentDiff.get(), Drift);

			/*if (Random)
				NoteTransform::Randomize(ChartState.NotesByChannel, CurrentDiff->Channels, CurrentDiff->Data->Turntable);*/


			// Sinisterrr/fully negative charts fix.
			UserSpeedMultiplier = abs(UserSpeedMultiplier);
			return new PlayerChartState(ChartState);
		}

		std::unique_ptr<Game::VSRG::Mechanics> PlayscreenParameters::PrepareMechanicsSet(
			std::shared_ptr<VSRG::Difficulty> CurrentDiff, 
			std::shared_ptr<Game::VSRG::ScoreKeeper> PlayerScoreKeeper,
			double JudgeY)
		{
			std::unique_ptr<Game::VSRG::Mechanics> MechanicsSet = nullptr;

			// This must be done before setLifeTotal in order for it to work.
			PlayerScoreKeeper->setMaxNotes(CurrentDiff->Data->GetScoreItemsCount());

			PlayerScoreKeeper->setUseW0(UseW0);

			// JudgeScale, Stepmania and OD can't be run together - only one can be set.
			auto TimingInfo = CurrentDiff->Data->TimingInfo;

			// Pick a timing system
			if (SystemType == VSRG::TI_NONE) {
				if (TimingInfo) {
					// Automatic setup
					SystemType = TimingInfo->GetType();
				}
				else {
					// Log::Printf("Null timing info - assigning raindrop defaults.\n");
					SystemType = VSRG::TI_RAINDROP;
					// pick raindrop system for null Timing Info
				}
			}


			// If we got just assigned one or was already requested
			// unlikely: timing info type is none? what
			
			if (SystemType == VSRG::TI_NONE) {
				// Player didn't request a specific subsystem
				// Log::Printf("System picked was none - on purpose. Defaulting to raindrop.\n");
				SystemType = VSRG::TI_RAINDROP;
			}

			TimingType UsedTimingType = SetupGameSystem(TimingInfo, PlayerScoreKeeper.get());

			/*
			If we're on TT_BEATS we've got to recalculate all note positions to beats,
			and use mechanics that use TT_BEATS as its timing type.
			*/

			bool disable_forced_release = SystemType == VSRG::TI_BMS ||
				SystemType == VSRG::TI_RDAC ||
				SystemType == VSRG::TI_STEPMANIA;
			if (UsedTimingType == VSRG::TT_TIME)
			{
				if (SystemType == VSRG::TI_RDAC)
				{
					// Log::Printf("RAINDROP ARCADE STAAAAAAAAART!\n");
					MechanicsSet = std::make_unique<RaindropArcadeMechanics>();
				}
				else {
					// Log::Printf("Using raindrop mechanics set!\n");
					// Only forced release if not a bms or a stepmania chart.
					MechanicsSet = std::make_unique<RaindropMechanics>(!disable_forced_release);
				}
			}
			else if (UsedTimingType == TT_BEATS)
			{
				//Log::Printf("Using o2jam mechanics set!\n");
				MechanicsSet = std::make_unique<O2JamMechanics>();
			}

			MechanicsSet->Setup(CurrentDiff.get(), PlayerScoreKeeper);
			SetupGauge(TimingInfo, PlayerScoreKeeper.get());
			UpdateHidden(JudgeY);

			return MechanicsSet;
		}

		void PlayscreenParameters::UpdateHidden(double JudgeY)
		{
			/*
			Given the top of the screen being 1, the bottom being -1
			calculate the range for which the current hidden mode is defined.
			*/
			CfgVar HiddenSize("HiddenSize", "Hidden");
			CfgVar FLSize("FlashlightSize", "Hidden");
			CfgVar Threshold("Threshold", "Hidden");

			auto toYRange = [](float x) {
				x /= ScreenHeight; // [0,768] -> [0,1]
				x *= -2; // [0,1] -> [0, -2]
				x += 1; // [1, -1]
				return x;
			};

			float Center = toYRange(Threshold * PLAYFIELD_SIZE);

			// Hidden calc
			if (HiddenMode)
			{
				float pfCenter;

				Hidden.TransitionSize = HiddenSize * PLAYFIELD_SIZE / ScreenHeight;

				if (Upscroll)
				{
					pfCenter = toYRange(ScreenHeight - JudgeY + Threshold * PLAYFIELD_SIZE);
					Center = pfCenter;


					// Invert Hidden Mode.
					if (HiddenMode == HM_SUDDEN) Hidden.Mode = HM_HIDDEN;
					else if (HiddenMode == HM_HIDDEN) Hidden.Mode = HM_SUDDEN;
					else Hidden.Mode = (Game::VSRG::EHiddenMode)HiddenMode;
				}
				else
				{
					pfCenter = Center;
					Center = pfCenter;
					Hidden.Mode = (Game::VSRG::EHiddenMode)HiddenMode;
				}

				Hidden.CenterSize = FLSize * PLAYFIELD_SIZE / ScreenHeight;
			}
		}

		ScoreType PlayscreenParameters::GetScoringType() const
		{
			if (SystemType == TI_BMS || SystemType == TI_RDAC) {
				return ST_EX;
			}

			if (SystemType == VSRG::TI_O2JAM) {
				return ST_O2JAM;
			}

			if (SystemType == VSRG::TI_OSUMANIA) {
				return ST_OSUMANIA;
			}

			if (SystemType == VSRG::TI_STEPMANIA) {
				return ST_DP;
			}

			if (SystemType == VSRG::TI_RAINDROP) {
				return ST_EXP3;
			}

			return ST_EX;
		}

		int PlayscreenParameters::GetHiddenMode()
		{
			return Hidden.Mode;
		}

		float PlayscreenParameters::GetHiddenCenter()
		{
			return Hidden.Center;
		}

		float PlayscreenParameters::GetHiddenTransitionSize()
		{
			return Hidden.TransitionSize;
		}

		float PlayscreenParameters::GetHiddenCenterSize()
		{
			return Hidden.CenterSize;
		}

		TimingType PlayscreenParameters::SetupGameSystem(
			std::shared_ptr<ChartInfo> TimingInfo, 
			ScoreKeeper* PlayerScoreKeeper) {
			TimingType UsedTimingType = TT_TIME;

			if (SystemType == VSRG::TI_BMS || SystemType == VSRG::TI_RDAC) {
				UsedTimingType = TT_TIME;
				if (TimingInfo->GetType() == VSRG::TI_BMS) {
					auto Info = static_cast<VSRG::BMSChartInfo*> (TimingInfo.get());
					if (!Info->PercentualJudgerank)
						PlayerScoreKeeper->setJudgeRank(Info->JudgeRank);
					else
						PlayerScoreKeeper->setJudgeScale(Info->JudgeRank / 100.0);
				}
				else {
					PlayerScoreKeeper->setJudgeRank(2);
				}
			}
			else if (SystemType == VSRG::TI_O2JAM) {
				UsedTimingType = TT_BEATS;
				PlayerScoreKeeper->setJudgeRank(-100);
			}
			else if (SystemType == VSRG::TI_OSUMANIA) {
				UsedTimingType = TT_TIME;
				if (TimingInfo->GetType() == VSRG::TI_OSUMANIA) {
					auto InfoOM = static_cast<VSRG::OsumaniaChartInfo*> (TimingInfo.get());
					PlayerScoreKeeper->setODWindows(InfoOM->OD);
				}
				else PlayerScoreKeeper->setODWindows(7);
			}
			else if (SystemType == VSRG::TI_STEPMANIA) {
				UsedTimingType = TT_TIME;
				PlayerScoreKeeper->setSMJ4Windows();
			}
			else if (SystemType == VSRG::TI_RAINDROP) {
				// LifebarType = LT_STEPMANIA;
			}

			return UsedTimingType;
		}
		void PlayscreenParameters::SetupGauge(std::shared_ptr<ChartInfo> TimingInfo, ScoreKeeper* PlayerScoreKeeper)
		{
			if (GaugeType == LT_AUTO) {
				using namespace VSRG;
				switch (SystemType) {
				case TI_BMS:
				case TI_RAINDROP:
				case TI_RDAC:
					GaugeType = LT_GROOVE;
					break;
				case TI_O2JAM:
					GaugeType = LT_O2JAM;
					break;
				case TI_OSUMANIA:
				case TI_STEPMANIA:
					GaugeType = LT_STEPMANIA;
					break;
				default:
					throw std::runtime_error("Invalid requested system.");
				}
			}

			switch (GaugeType) {
			case LT_STEPMANIA:
				// LifebarType = LT_STEPMANIA; // Needs no setup.
				break;

			case LT_O2JAM:
				if (TimingInfo->GetType() == VSRG::TI_O2JAM) {
					auto InfoO2 = static_cast<VSRG::O2JamChartInfo*> (TimingInfo.get());
					PlayerScoreKeeper->setO2LifebarRating(InfoO2->Difficulty);
				} // else by default
				  // LifebarType = LT_O2JAM; // By default, HX
				break;

			case LT_GROOVE:
			case LT_DEATH:
			case LT_EASY:
			case LT_EXHARD:
			case LT_SURVIVAL:
				if (TimingInfo->GetType() == VSRG::TI_BMS) { // Only needs setup if it's a BMS file
					auto Info = static_cast<VSRG::BMSChartInfo*> (TimingInfo.get());
					if (Info->IsBMSON)
						PlayerScoreKeeper->setLifeTotal(NAN, Info->GaugeTotal / 100.0);
					else
						PlayerScoreKeeper->setLifeTotal(Info->GaugeTotal);
				}
				else // by raindrop defaults
					PlayerScoreKeeper->setLifeTotal(-1);
				// LifebarType = (LifeType)Parameters.GaugeType;
				break;
			case LT_NORECOV:
				// ...
				break;
			default:
				throw std::runtime_error("Invalid gauge type recieved");
			}

		}
	}
}