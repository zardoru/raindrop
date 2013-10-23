/* Song Timing functions. */
#include "Global.h"
#include "Song.h"

using namespace SongInternal;
using std::vector;

float spb(float bpm)
{
	return 60 / bpm;
}

/* Assume these are sorted. */
uint32 SectionIndex(Difficulty &Diff, float Beat)
{
	vector<Difficulty::TimingSegment> &Timing = Diff.Timing;
	uint32 Index = 0;
	for (vector<Difficulty::TimingSegment>::iterator i = Timing.begin(); i != Timing.end(); i++)
	{
		if (Beat > i->Time)
			Index++;
		else
			break;
	}
	return Index;
}

double TimeAtBeat(Difficulty &Diff, float Beat)
{
	vector<Difficulty::TimingSegment> &Timing = Diff.Timing;
	uint32 CurrentIndex = SectionIndex(Diff, Beat);
	double Time = 0;

	for (uint32 i = 0; i < CurrentIndex; i++)
	{	
		if (i+1 < Timing.size()) // Get how long the current timing goes.
		{
			float BeatDurationOfSectionI = Timing[i+1].Time + Timing[i].Time; // Section lasts this much.
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
}
