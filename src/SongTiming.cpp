#include "Global.h"
#include "Song.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>

uint32 SectionIndex(const TimingData &Timing, float Beat)
{
	uint32 Index = 0;
	for (TimingData::const_iterator i = Timing.begin(); i != Timing.end(); i++)
	{
		if (Beat >= i->Time)
			Index++;
		else
			break;
	}
	return Index;
}

double BpmAtBeat(const TimingData &Timing, float Beat)
{
	return Timing[SectionIndex(Timing,Beat)-1].Value;
}

double TimeAtBeat(const TimingData &Timing, float Offset, float Beat)
{
	uint32 CurrentIndex = SectionIndex(Timing, Beat);
	double Time = Offset;

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

void GetTimingChangesInInterval(const TimingData &Timing, 
	double PointA, double PointB, 
	TimingData &Out)
{
	Out.clear();

	for (TimingData::const_iterator i = Timing.begin(); i != Timing.end(); i++)
	{
		if (i->Time >= PointA && i->Time < PointB)
		{
			Out.push_back(*i);
		}
	}
}

void LoadTimingList(TimingData &Timing, String line, bool AllowZeros)
{
	String ListString = line.substr(line.find_first_of(":") + 1);
	std::vector< String > SplitResult;
	SongInternal::TimingSegment Segment;

	// Remove whitespace.
	boost::replace_all(ListString, "\n", "");
	boost::split(SplitResult, ListString, boost::is_any_of(",")); // Separate List of BPMs.
	BOOST_FOREACH(String ValueString, SplitResult)
	{
		std::vector< String > SplitResultPair;
		boost::split(SplitResultPair, ValueString, boost::is_any_of("=")); // Separate Time=Value pairs.

		if (SplitResultPair.size() == 1) // Assume only one BPM on the whole list.
		{
			Segment.Value = atof(SplitResultPair[0].c_str());
			Segment.Time = 0;
		}else // Multiple BPMs.
		{
			Segment.Time = atof(SplitResultPair[0].c_str());
			Segment.Value = atof(SplitResultPair[1].c_str());
		}

		if (AllowZeros || Segment.Value)
			Timing.push_back(Segment);
	}
}

double StopTimeAtBeat(const TimingData &StopsTiming, float Beat)
{
	double Time = 0;

	if (Beat == 0 || !StopsTiming.size()) return Time;

	for (uint32 i = 0; i < StopsTiming.size(); i++)
	{	
		if (StopsTiming.at(i).Time < Beat)
			Time += StopsTiming.at(i).Value;
	}

	return Time;
}

double VerticalAtTime(const TimingData &Timing, float Time, float Drift)
{
	uint32 Section = SectionIndex(Timing, Time) - 1;
	double Out = 0;

	for (uint32 i = 0; i < Section; i++)
	{
		Out += (Timing[i+1].Time - Timing[i].Time) * Timing[i].Value;
	}

	Out += (Time - Timing[Section].Time + Drift) * Timing[Section].Value;

	return Out;
}

double BeatAtTime(const TimingData &Timing, float Time, float Offset)
{
	double sDelta = Timing[0].Value * Offset;
	return VerticalAtTime(Timing, Time) - sDelta;
}