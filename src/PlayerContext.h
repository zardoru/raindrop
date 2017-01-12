#pragma once

#include "pch.h"
#include "SongTiming.h"
#include "Song7K.h"
#include "VSRGMechanics.h"
#include "PlayerChartData.h"
#include "Noteskin.h"

class Line;

/*
	Usage of a PlayerContext requires several steps.
	* If you're using a SceneEnvironment, register the context's type via SetupLua.
	* Setup of playable data, along with player parameters.
	* The difficulty object must have the "Data" structure non-null.
	After that the difficulty's data can be released.
	
	(optional unless you want to display stuff)
	* Then, that the callbacks for keysound stuff is set up 
		* OnMiss, OnHit, PlayKeysound are the required functions.

	* A SceneEnvironment must be active so that it can send the PC data to it on private events.
	* At this point you can Validate() and mechanics and stuff will be set up internally for legit usage.

*/

namespace Game {
	namespace VSRG {

		class ScoreKeeper;

		class PlayerContext {
		public:
			;
		private:
			GameChartData ChartData;

			double LastUpdateTime; // seconds, song time

			std::shared_ptr<VSRG::Difficulty>	 CurrentDiff;

			std::unique_ptr<Game::VSRG::Mechanics> MechanicsSet;
			std::shared_ptr<ScoreKeeper> PlayerScoreKeeper;

			std::unique_ptr<Line> Barline;

			double MsDisplayMargin;
			double Drift;
			bool JudgeNotes;

			Parameters Parameters;

			struct SHiddenData {
				EHiddenMode		 Mode;
				float            ClampLow, ClampHigh, ClampFactor;
				float            ClampSum;
			} Hidden;

			LifeType           LifebarType;
			ScoreType          ScoringType;


			struct SGearState {
				std::map<int, int> Bindings;
				VSRG::TrackNote*   CurrentKeysounds[VSRG::MAX_CHANNELS];
				bool IsPressed[VSRG::MAX_CHANNELS]; //  Whether the lane is pressed
				bool HeldKey[VSRG::MAX_CHANNELS]; //  Whether a hold note is active
				int  ClosestNoteMS[VSRG::MAX_CHANNELS];
				/*
					HeldKey is only active if there's a hold right now.
					IsPressed is active any time the key for that lane is down.
				*/
				bool TurntableEnabled;
			} Gear;

			// Whether to use time-based binary search on notes
			bool UseNoteOptimization();

			void DrawBarlines(double cur_vertical, double smult);
			int DrawMeasures(double song_time); // returns rendered note count 

			std::shared_ptr<SceneEnvironment> Animations;
			Noteskin PlayerNoteskin;
			int PlayerNumber;
			int UsedTimingType;

			void SetupMechanics();
			void RunMeasures(double time);
			void PlayLaneKeysound(uint32_t Lane);
			void RunAuto(TrackNote *m, double usedTime, uint32_t k);
			void UpdateHidden(float AdjustmentSize, float FlashlightRatio);
		public:
			PlayerContext(int pn, Game::VSRG::Parameters par = Game::VSRG::Parameters());
			void Init();
			void Validate();
			void Update(double songTime);
			void Render(double songTime);

			std::function<void(int)> PlayKeysound;
			std::function<void(double, uint32_t, bool, bool)> OnHit;
			std::function<void(double, uint32_t, bool, bool, bool)> OnMiss;

			/*
				About this pointer's lifetime:
				PlayerContext requires the song/difficulty pointer to stay valid until it's destroyed.
			*/
			void SetPlayableData(std::shared_ptr<VSRG::Difficulty> difficulty, double Drift = 0, 
								double DesiredDefaultSpeed = 0, int Type = SPEEDTYPE_DEFAULT);
			const GameChartData &GetPlayerState();

			// Getters (Lua)
			bool IsFailEnabled() const;
			bool IsUpscrolling() const;
			bool GetUsesTurntable() const;

			double GetAppliedSpeedMultiplier(double Time) const;
			double GetCurrentBeat() const;
			double GetUserMultiplier() const;
			double GetCurrentVerticalSpeed() const;
			double GetWarpedSongTime() const;
			double GetCurrentBPM() const;
			double GetJudgmentY() const;
			double GetLifePST() const;
			std::string GetPacemakerText(bool bm) const;
			int GetPacemakerValue(bool bm) const;

			double GetChartTimeAt(double time) const;
			int GetChannelCount() const;
			int GetPlayerNumber() const;
			bool GetIsHeldKey(int Lane) const;
			double GetMeasureTime(double Msr) const;
			double HasSongFinished(double time) const;


			VSRG::Difficulty* GetDifficulty() const;

			double GetDuration() const;
			double GetBeatDuration() const;
			/*
				"So why is this returning a raw pointer?"
				Lua binding. The answer is lua binding.
			*/
			ScoreKeeper* GetScoreKeeper() const;

			double GetClosestNoteTime(int Lane) const;

			// Setters
			void SetUserMultiplier(float Multip);
			void SetSceneEnvironment(std::shared_ptr<SceneEnvironment> env);

			// Only if Difficulty->Data is not null.
			std::vector<AutoplaySound> GetBgmData();

			static void SetupLua(LuaManager *Env);

			double GetScore() const;
			int GetCombo() const;

			void HitNote(double TimeOff, uint32_t Lane, bool IsHold, bool IsHoldRelease = false);
			void MissNote(double TimeOff, uint32_t Lane, bool IsHold, bool dont_break_combo, bool early_miss);
			void GearKeyEvent(uint32_t Lane, bool KeyDown);
			void JudgeLane(uint32_t Lane, double Time);
			void ReleaseLane(uint32_t Lane, double Time);
			void TranslateKey(int32_t K, bool KeyDown, double Time);
			void SetLaneHoldState(uint32_t Lane, bool NewState);
			// true if holding down key
			bool GetGearLaneState(uint32_t Lane);   
			bool BindKeysToLanes(bool UseTurntable);

			void SetCanJudge(bool canjudge);
			bool CanJudge();

			void SetUnwarpedTime(double time);

			// Whether the player has actually failed or not
			bool HasFailed() const;

			// Whether failure is delayed until the screen is over
			bool HasDelayedFailure();
		};
	}
}
