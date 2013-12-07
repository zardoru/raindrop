#include "Global.h"
#include "TrackNote.h"

TrackNote::TrackNote()
{
	Track = StartTime = EndTime = Measure = Fraction = 0;
}

TrackNote::~TrackNote()
{
}

void TrackNote::AssignTrack(int T)
{
	Track = T;
}

void TrackNote::AssignTime(float Start, float End)
{
	StartTime = Start;
	EndTime = End;
}

void TrackNote::AssignSongPosition(int _Measure, int _Fraction)
{
	Measure = _Measure;
	Fraction = _Fraction;
}