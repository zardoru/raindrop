/* SongDC Timing functions. */
/* note to self: Template syntax is ugly. */
#include <vector>
using std::vector;

/* Assume these are sorted. */
// Return the (vector's index+1) Beat resides in.
template <class T>
uint32 SectionIndex(SongInternal::TDifficulty<T> &Diff, float Beat)
{
	vector<SongInternal::TDifficulty<T>::TimingSegment> &Timing = Diff.Timing;
	uint32 Index = 0;
	for (vector<SongInternal::TDifficulty<T>::TimingSegment>::iterator i = Timing.begin(); i != Timing.end(); i++)
	{
		if (Beat >= i->Time)
			Index++;
		else
			break;
	}
	return Index;
}

template <class T>
double BpmAtBeat(SongInternal::TDifficulty<T> &Diff, float Beat)
{
	return Diff.Timing[SectionIndex(Diff,Beat)-1].Value;
}

template <class T>
double TimeAtBeat(SongInternal::TDifficulty<T> &Diff, float Beat)
{
	vector<SongInternal::TDifficulty<T>::TimingSegment> &Timing = Diff.Timing;
	uint32 CurrentIndex = SectionIndex(Diff, Beat);
	double Time = Diff.Offset;

	if (Beat == 0) return Time;

	for (uint32 i = 0; i < CurrentIndex; i++)
	{	
		if (i+1 < CurrentIndex) // Get how long the current timing goes.
		{
			float BeatDurationOfSectionI = Timing[i+1].Time - Timing[i].Time; // Section lasts this much.
			float SectionDuration = BeatDurationOfSectionI * spb ( Timing[i].Value );

			if (Beat < Timing[i+1].Time && Beat > Timing[i].Time)
			{
				// If this is our interval, stop summing time of previous intervals before this one
				SectionDuration = (Beat - Timing[i].Time) * spb ( Timing[i].Value );
			}
			Time += SectionDuration;
		}else
			Time += (Beat - Timing[i].Time) * spb ( Timing[i].Value );
	}
	return Time;
}

template <class T>
double DifficultyDuration(TSong<T> &MySong, SongInternal::TDifficulty<T> &Diff)
{
	if (Diff.Measures.size())
		return TimeAtBeat(Diff, Diff.Measures.size() * MySong.MeasureLength);
	return 0;
}

template <typename T>
void GetTimingChangesInInterval(std::vector<typename SongInternal::TDifficulty<T>::TimingSegment> Timing, 
	double PointA, double PointB, 
	std::vector<typename SongInternal::TDifficulty<T>::TimingSegment> &Out)
{
	Out.clear();

	for (vector<SongInternal::TDifficulty<T>::TimingSegment>::iterator i = Timing.begin(); i != Timing.end(); i++)
	{
		if (i->Time >= PointA && i->Time < PointB)
		{
			Out.push_back(*i);
		}
	}
}