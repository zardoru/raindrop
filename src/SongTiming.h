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

template <class T>
double DifficultyDuration(TSong<T> &MySong, SongInternal::TDifficulty<T> &Diff)
{
	if (Diff.Measures.size())
		return TimeAtBeat(Diff.Timing, Diff.Offset, Diff.Measures.size() * MySong.MeasureLength);
	return 0;
}

void GetTimingChangesInInterval(const TimingData &Timing, 
	double PointA, double PointB, 
	TimingData &Out);

void LoadTimingList(TimingData &Timing, String line, bool AllowZeros = false);
