#pragma once

/* SongDC Timing functions. */

/* Song Timing */
inline double spb(double bpm) { return 60 / bpm; } // Return seconds per beat.
inline double bps(double bpm) { return bpm / 60; } // Return beats per second.

/* Assume these are sorted. */
// Return the vector's index Beat resides in.
int SectionIndex(const TimingData &Timing, double Beat);

/*
	Find the value of the interval function defined by Timing at the value Beat
*/
double SectionValue(const TimingData &Timing, double Beat);

/*
	Find the time at Beat given a constant function defined in intervals by Timing
*/
double TimeAtBeat(const TimingData &Timing, float Offset, double Beat, bool Abs = false);

/*
	Find the antiderivative of an interval defined function through the Timing variable
	from 0 to Time.
*/
double IntegrateToTime(const TimingData &Timing, double Time, float Drift = 0);

/*
	Find the sum of all stops that have happened in [0, Beat)
*/
double StopTimeAtBeat(const TimingData &StopsTiming, double Beat);

#define DifficultyDuration(MySong, Diff) \
	(Diff.Measures.size()) ? \
		TimeAtBeat(Diff.Timing, Diff.Offset, Diff.Measures.size() * MySong.MeasureLength); : \
	0

void GetTimingChangesInInterval(const TimingData &Timing, 
	double PointA, double PointB, 
	TimingData &Out);

void LoadTimingList(TimingData &Timing, std::string line, bool AllowZeros = false);


// Quantizes fraction to a beat's maximum resolution (1/48th of a beat)
double QuantizeFractionBeat(double Frac);

// Quantizes fraction to a measure's maximum resolution (1/192nd of a measure)
double QuantizeFractionMeasure(double Frac);

// Quantizes beat to a beat's maximum resolution (1/48th of a beat)
double QuantizeBeat(double Beat);
