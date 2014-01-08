#include "Global.h"
#include "Song.h"
#include <algorithm>

Song7K::Song7K()
{
	MeasureLength = 4;
	LeadInTime = 1.5;
	UseSeparateTimingData = false;
	Processed = false;
}

Song7K::~Song7K()
{
}

int tSort(const SongInternal::TimingSegment &i, const SongInternal::TimingSegment &j)
{
	return i.Time < j.Time;
}

void Song7K::ProcessVSpeeds(SongInternal::TDifficulty<TrackNote>* Diff, double Drift)
{
	/* Calculate VSpeeds. */

	if (Diff->Offset > 0 || Drift) /* We have to set up a speed during this time, otherwise it'll be 0. */
	{
		SongInternal::TimingSegment VSpeed;
		float FTime = (spb (Diff->Timing.at(0).Value) * (float)MeasureLength);

		VSpeed.Time = 0;
		VSpeed.Value = MeasureBaseSpacing / FTime;

		Diff->VerticalSpeeds.push_back(VSpeed);
	}

	for(TimingData::iterator Time = Diff->Timing.begin();
		Time != Diff->Timing.end();
		Time++)
	{
		SongInternal::TimingSegment VSpeed;


		float FTime;

		if (BPMType == BT_Beat) // Time is in Beats
		{
			FTime = (spb (Time->Value) * (float)MeasureLength);
			VSpeed.Time = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time);
		}
		else if (BPMType == BT_MS) // Time is in MS
		{
			FTime = (spb (Time->Value) * (float)MeasureLength);
			VSpeed.Time = Time->Time + Drift;
		}else if ( BPMType == BT_Beatspace ) // Time in MS, and not using bpm, but ms per beat.
		{
			FTime = Time->Value / 1000.0 * MeasureLength;
			VSpeed.Time = Time->Time + Drift;
		}

		VSpeed.Value = MeasureBaseSpacing / FTime;
		Diff->VerticalSpeeds.push_back(VSpeed);
	}

	/* Sort for justice */
	std::sort(Diff->VerticalSpeeds.begin(), Diff->VerticalSpeeds.end(), tSort);

	if (!Diff->StopsTiming.size() || BPMType != BT_Beat) // Stops only supported in Beat mode.
		return;

	/* Here on, just working with stops. */
	for(TimingData::iterator Time = Diff->StopsTiming.begin();
		Time != Diff->StopsTiming.end();
		Time++)
	{
		SongInternal::TimingSegment VSpeed;
		float TValue = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time);
		float TValueN = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time) + Time->Value;

		/* Initial Stop */
		VSpeed.Time = TValue;
		VSpeed.Value = 0;

		/* First, eliminate collisions. */
		for (TimingData::iterator k = Diff->VerticalSpeeds.begin(); k != Diff->VerticalSpeeds.end(); k++)
		{
			if ( abs(k->Time - TValue) < 0.000001 ) /* Too close? Remove the collision, leaving only the 0 in front. */
			{
				k = Diff->VerticalSpeeds.erase(k);

				if (k == Diff->VerticalSpeeds.end())
					break;
			}
		}

		Diff->VerticalSpeeds.push_back(VSpeed);

		float speedRestore = MeasureBaseSpacing / (spb(BpmAtBeat(Diff->Timing, Time->Time)) * MeasureLength);

		/* 
			Find speeds between TValue and TValueN, use the last one as the speed we're going to use. 
			There's quite the count of simfiles that use overlapping stops and bpm changes, and I'm not really
			sure how to handle them from a vertical speeds perspective.
			That said, here's my try.
		*/

		for (TimingData::iterator k = Diff->VerticalSpeeds.begin(); k != Diff->VerticalSpeeds.end(); k++)
		{
			if (k->Time > TValue && k->Time < TValueN)
			{
				speedRestore = k->Value; /* This is the last speed change in the interval that the stop lasts. We'll use it. */

				/* Eliminate this since we're not going to use it. */
				k = Diff->VerticalSpeeds.erase(k);

				if (k == Diff->VerticalSpeeds.end())
					break;
			}
		}

		/* Restored speed after stop */
		VSpeed.Time = TValueN;
		VSpeed.Value = speedRestore;
		Diff->VerticalSpeeds.push_back(VSpeed);
	}

	std::sort(Diff->VerticalSpeeds.begin(), Diff->VerticalSpeeds.end(), tSort);
}

void Song7K::Process(float Drift)
{
	/* 
		We'd like to build the notes' position from 0 to infinity, 
		however the real "zero" position would be the judgement line
		in other words since "up" is negative relative to 0
		and 0 is the judgement line
		position would actually be
		judgeline - positiveposition
		and positiveposition would just be
		measure * measuresize + fraction * fractionsize
	*/

	if (Processed)
		return;

	/* For all difficulties */
	for (std::vector<SongInternal::TDifficulty<TrackNote>*>::iterator Diff = Difficulties.begin(); Diff != Difficulties.end(); Diff++)
	{
		if (!(*Diff)->Timing.size())
			continue;

		ProcessVSpeeds(*Diff, Drift);

		/* For all channels of this difficulty */
		for (int KeyIndex = 0; KeyIndex < (*Diff)->Channels; KeyIndex++)
		{
			glm::vec2 BasePosition (KeyIndex * (GearWidth / (*Diff)->Channels), 0);
			int MIdx = 0;

			/* For each measure of this channel */
			for (std::vector<SongInternal::Measure<TrackNote> >::iterator Measure = (*Diff)->Measures[KeyIndex].begin(); 
				Measure != (*Diff)->Measures[KeyIndex].end();
				Measure++)
			{
				/* For each note in the measure... */
				for (uint32 Note = 0; Note < Measure->MeasureNotes.size(); Note++)
				{
					/* 
					    Calculate position. (Change this to TrackNote instead of processing?)
					    issue is not having the speed change data there.
					*/
					TrackNote &CurrentNote = (*Measure).MeasureNotes[Note];
					/*int NoteMeasure = CurrentNote.GetMeasure();
					float MeasureVerticalD = MeasureBaseSpacing * NoteMeasure;
					float FractionVerticalD = 1.0f / float((*Diff)->Measures[KeyIndex][NoteMeasure].Fraction) * float(CurrentNote.GetFraction()) * MeasureBaseSpacing;
					*/
					CurrentNote.AddTime (Drift);

					glm::vec2 VerticalPosition( 0, VerticalAtTime((*Diff)->VerticalSpeeds, CurrentNote.GetStartTime()) );

					// if upscroll change minus for plus as well as matrix at screengameplay7k
					CurrentNote.AssignPosition(BasePosition - VerticalPosition);
				}
				MIdx++;
			}
		}
	}

	PreviousDrift = Drift;
	Processed = true;
}
