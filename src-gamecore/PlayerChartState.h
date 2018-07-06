#pragma once



namespace Game {
	namespace VSRG {
		struct PlayerChartState {
			TimingData           ScrollSpeeds;
			TimingData		     BPS;
			TimingData		     Warps;
			VectorInterpolatedSpeedMultipliers   InterpoloatedSpeedMultipliers;
			VectorTN       Notes;
			VectorTNP       NotesVerticallyOrdered;
			VectorTNP       NotesTimeOrdered;
			std::vector<double>	 MeasureBarlines;
			bool HasNegativeScroll;
			bool HasTurntable;

			double WaitTime;
			Difficulty* ConnectedDifficulty;

			PlayerChartState();

			// Chart data functions
			double GetWarpedSongTime(double SongTime) const;
			double GetWarpAmount(double Time) const;
			bool IsWarpingAt(double start_time) const;
			std::vector<double> GetMeasureLines() const;
			double GetSpeedMultiplierAt(double Time) const; // Time in unwarped song time
			double GetBpmAt(double Time) const;
			double GetBpsAt(double Time) const;
			double GetBeatAt(double Time) const;
			double GetChartDisplacementAt(double Time) const;
			double GetDisplacementSpeedAt(double Time) const; // in unwarped song time
			double GetTimeAtBeat(double beat) const;
			double GetOffset() const;
			double GetMeasureTime(double msr) const;

			std::map<int, std::string> GetSoundList() const;
			ChartType GetChartType() const;
			bool IsBmson() const;
			bool IsVirtual() const;
			bool HasTimingData() const;
			SliceContainer GetSliceData() const;

			void PrepareOrderedNotes();
			void DisableNotesUntil(double Time);
			void ResetNotes();

			// Drift is an offset to apply to _everything_.
			// Speed is a constant to set the speed to.
			static PlayerChartState FromDifficulty(Difficulty *diff, double Speed = 0);
		};
	}
}