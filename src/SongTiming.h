/* SongDC Timing functions. */

/* Song Timing */
inline float spb(float bpm) { return 60 / bpm; } // Return seconds per beat.
inline float bps(float bpm) { return bpm / 60; } // Return beats per second.

/* Assume these are sorted. */
// Return the (vector's index+1) Beat resides in.
uint32 SectionIndex(const TimingData &Timing, float Beat);

/*
	Find the value of the interval function defined by Timing at the value Beat
*/
double SectionValue(const TimingData &Timing, float Beat);

/*
	Find the time at Beat given a constant function defined in intervals by Timing
*/
double TimeAtBeat(const TimingData &Timing, float Offset, float Beat);

/*
	Find the antiderivative of an interval defined function through the Timing variable
	from 0 to Time.
*/
double IntegrateToTime(const TimingData &Timing, float Time, float Drift = 0);

/*
	Find the sum of all stops that have happened in [0, Beat)
*/
double StopTimeAtBeat(const TimingData &StopsTiming, float Beat);

double BeatAtTime(const TimingData &Timing, float Time, float Offset);

#define DifficultyDuration(MySong, Diff) \
	(Diff.Measures.size()) ? \
		TimeAtBeat(Diff.Timing, Diff.Offset, Diff.Measures.size() * MySong.MeasureLength); : \
	0

void GetTimingChangesInInterval(const TimingData &Timing, 
	double PointA, double PointB, 
	TimingData &Out);

void LoadTimingList(TimingData &Timing, GString line, bool AllowZeros = false);


// Quantizes fraction to a beat's maximum resolution (1/48th of a beat)
double QuantizeFractionBeat(float Frac);

// Quantizes fraction to a measure's maximum resolution (1/192nd of a measure)
double QuantizeFractionMeasure (float Frac);

// Quantizes beat to a beat's maximum resolution (1/48th of a beat)
double QuantizeBeat (float Beat);
