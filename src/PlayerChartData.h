#pragma once

#include "GameGlobal.h"

namespace Game {
	namespace VSRG {
		struct PlayerChartData {
			TimingData           VSpeeds;
			TimingData		     BPS;
			TimingData		     Warps;
			VectorSpeeds   Speeds;
			VectorTN       NotesByChannel;
			std::vector<double>	 MeasureBarlines;
			bool HasNegativeScroll;
			bool HasTurntable;

			double Drift;
			double WaitTime;
			Difficulty* ConnectedDifficulty;

			PlayerChartData();

			// Chart data functions
			double GetWarpedSongTime(double SongTime) const;
			double GetWarpAmount(double Time) const;
			bool IsWarpingAt(double start_time) const;
			std::vector<double> PlayerChartData::GetMeasureLines() const;
			double PlayerChartData::GetSpeedMultiplierAt(double Time) const; // Time in unwarped song time
			double GetBpmAt(double Time) const;
			double GetBpsAt(double Time) const;
			double GetBeatAt(double Time) const;
			double GetDisplacementAt(double Time) const;
			double GetTimeAtBeat(double beat, double drift = 0) const;
			double GetOffset() const;
			std::map<int, std::string> GetSoundList() const;
			ChartType GetChartType() const;
			bool IsBmson() const;
			bool IsVirtual() const;
			bool HasTimingData() const;
			SliceContainer GetSliceData() const;

			void DisableNotesUntil(double Time);
			void ResetNotes();

			// Drift is an offset to apply to _everything_.
			// Speed is a constant to set the speed to.
			static PlayerChartData FromDifficulty(Difficulty *diff, double Drift = 0, double Speed = 0);
		};
	}
}