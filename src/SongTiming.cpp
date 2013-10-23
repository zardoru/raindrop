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

