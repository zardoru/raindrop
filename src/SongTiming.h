/* SongDC Timing functions. */
/* note to self: Template syntax is ugly. */
#include <vector>

/* Assume these are sorted. */
// Return the (vector's index+1) Beat resides in.
uint32 SectionIndex(const TimingData &Timing, float Beat);

double BpmAtBeat(const TimingData &Timing, float Beat);

double TimeAtBeat(const TimingData &Timing, float Offset, float Beat);

double VerticalAtTime(const TimingData &Timing, float Time, float Drift = 0);

double StopTimeAtBeat(const TimingData &StopsTiming, float Beat);

double BeatAtTime(const TimingData &Timing, float Time, float Offset);

#define DifficultyDuration(MySong, Diff) \
	(Diff.Measures.size()) ? \
		TimeAtBeat(Diff.Timing, Diff.Offset, Diff.Measures.size() * MySong.MeasureLength); : \
	0

void GetTimingChangesInInterval(const TimingData &Timing, 
	double PointA, double PointB, 
	TimingData &Out);

void LoadTimingList(TimingData &Timing, String line, bool AllowZeros = false);
