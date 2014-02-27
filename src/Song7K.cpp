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

void Song7K::ProcessVSpeeds(SongInternal::Difficulty7K* Diff, double SpeedConstant)
{
	if (SpeedConstant) // We're using a CMod, so further processing is pointless
	{
		SongInternal::TimingSegment VSpeed;
		VSpeed.Time = 0;
		VSpeed.Value = SpeedConstant;
		Diff->VerticalSpeeds.push_back(VSpeed);
		return;
	}

	// Calculate velocity at time based on BPM at time
	for (TimingData::iterator Time = Diff->BPS.begin();
		Time != Diff->BPS.end();
		Time++)
	{
		float VerticalSpeed;
		SongInternal::TimingSegment VSpeed;

		if (Time->Value)
		{
			float spb = 1 / Time->Value;
			VerticalSpeed = MeasureBaseSpacing / (spb * MeasureLength);
		}else
			VerticalSpeed = 0;

		VSpeed.Value = VerticalSpeed;
		VSpeed.Time = Time->Time;

		Diff->VerticalSpeeds.push_back(VSpeed);
	}
}


void Song7K::ProcessBPS(SongInternal::Difficulty7K* Diff, double Drift)
{
	/* 
		Calculate BPS. The algorithm is basically the same as VSpeeds, so there's probably a better way to do it
		that is not repeating the same thing using different values.
	*/

	if (Diff->Offset > 0 || Drift || BPMType == BT_Beatspace) /* We have to set up a speed during this time, otherwise it'll be 0. */
	{
		SongInternal::TimingSegment Seg;

		Seg.Time = 0;

		if (BPMType != BT_Beatspace)
		{
			Seg.Value = bps (Diff->Timing[0].Value);
		}else
			Seg.Value = bps ( 60000.0 / Diff->Timing[0].Value );

		Diff->BPS.push_back(Seg);
	}

	for(TimingData::iterator Time = Diff->Timing.begin();
		Time != Diff->Timing.end();
		Time++)
	{
		SongInternal::TimingSegment Seg;

		float FTime;

		if (BPMType == BT_Beat) // Time is in Beats
		{
			FTime = bps (Time->Value);
			Seg.Time = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time);
		}
		else if (BPMType == BT_MS) // Time is in MS
		{
			FTime = bps (Time->Value);
			Seg.Time = Time->Time + Drift;
		}else if ( BPMType == BT_Beatspace ) // Time in MS, and not using bpm, but ms per beat.
		{
			FTime = bps (60000.0 / Time->Value);
			Seg.Time = Time->Time + Drift;
		}

		Seg.Value = FTime;
		Diff->BPS.push_back(Seg);
	}

	/* Sort for justice */
	std::sort(Diff->BPS.begin(), Diff->BPS.end(), tSort);

	if (!Diff->StopsTiming.size() || BPMType != BT_Beat) // Stops only supported in Beat mode.
		return;

	/* Here on, just working with stops. */
	for(TimingData::iterator Time = Diff->StopsTiming.begin();
		Time != Diff->StopsTiming.end();
		Time++)
	{
		SongInternal::TimingSegment Seg;
		float TValue = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time);
		float TValueN = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time) + Time->Value;

		/* Initial Stop */
		Seg.Time = TValue;
		Seg.Value = 0;

		/* First, eliminate collisions. */
		for (TimingData::iterator k = Diff->BPS.begin(); k != Diff->BPS.end(); k++)
		{
			if ( abs(k->Time - TValue) < 0.000001 ) /* Too close? Remove the collision, leaving only the 0 in front. */
			{
				k = Diff->BPS.erase(k);

				if (k == Diff->BPS.end())
					break;
			}
		}

		Diff->BPS.push_back(Seg);

		float speedRestore = bps(BpmAtBeat(Diff->Timing, Time->Time));

		for (TimingData::iterator k = Diff->BPS.begin(); k != Diff->BPS.end(); k++)
		{
			if (k->Time > TValue && k->Time < TValueN)
			{
				speedRestore = k->Value; /* This is the last speed change in the interval that the stop lasts. We'll use it. */

				/* Eliminate this since we're not going to use it. */
				k = Diff->BPS.erase(k);

				if (k == Diff->BPS.end())
					break;
			}
		}

		/* Restored speed after stop */
		Seg.Time = TValueN;
		Seg.Value = speedRestore;
		Diff->BPS.push_back(Seg);
	}

	std::sort(Diff->BPS.begin(), Diff->BPS.end(), tSort);
}

void Song7K::ProcessSpeedVariations(SongInternal::Difficulty7K* Diff, double Drift)
{
	TimingData tVSpeeds = Diff->VerticalSpeeds; // We need this to store what values to change

	for(TimingData::iterator Change = Diff->SpeedChanges.begin();
			Change != Diff->SpeedChanges.end();
			Change++)
	{
		/* 
			Find all VSpeeds
			if there exists a speed change which is virtually happening at the same time as this VSpeed
			modify it to be this value * factor
		*/

		Change->Time += Drift;

		for(TimingData::iterator Time = Diff->VerticalSpeeds.begin();
			Time != Diff->VerticalSpeeds.end();
			Time++)
		{
			if ( abs(Change->Time - Time->Time) < 0.00001)
			{
				Time->Value *= Change->Value;
				goto next_speed;
			}
		}

		/* 
			There are no collisions- insert a new speed at this time
		*/

		if (Change->Time < 0)
			goto next_speed;

		float SpeedValue;
		SpeedValue = BpmAtBeat(tVSpeeds, Change->Time) * Change->Value;

		SongInternal::TimingSegment VSpeed;
		float SpeedAtBeat;

		VSpeed.Time = Change->Time;
		VSpeed.Value = SpeedValue;

		SpeedAtBeat = BpmAtBeat(Diff->VerticalSpeeds, Change->Time);

		if (SpeedAtBeat != SpeedValue) // No redundant speeds
			Diff->VerticalSpeeds.push_back(VSpeed);
#ifndef NDEBUG
			/*
		else
		{
			printf("Redundant speed change while processing (%f / %f) Old value at time is %f, Original value is %f\n", Change->Time, SpeedValue, SpeedAtBeat, BpmAtBeat(tVSpeeds, Change->Time));
		}
		*/
#endif

		next_speed: (void)0;
	}

	std::sort(Diff->VerticalSpeeds.begin(), Diff->VerticalSpeeds.end(), tSort);
}

void Song7K::Process(float Drift, double SpeedConstant)
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
	for (std::vector<SongInternal::Difficulty7K*>::iterator Diff = Difficulties.begin(); Diff != Difficulties.end(); Diff++)
	{
		if (!(*Diff)->Timing.size())
			continue;

		ProcessBPS(*Diff, Drift);
		ProcessVSpeeds(*Diff, SpeedConstant);

		if (!SpeedConstant) // If there is a speed constant having speed changes is not what we want
			ProcessSpeedVariations(*Diff, Drift);

		/* For all channels of this difficulty */
		for (int KeyIndex = 0; KeyIndex < (*Diff)->Channels; KeyIndex++)
		{
			int MIdx = 0;

			/* For each measure of this channel */
			for (std::vector<SongInternal::Measure7K>::iterator Measure = (*Diff)->Measures[KeyIndex].begin(); 
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

					Vec2 VerticalPosition( 0, VerticalAtTime((*Diff)->VerticalSpeeds, CurrentNote.GetStartTime()) );
					Vec2 HoldEndPosition( 0, VerticalAtTime((*Diff)->VerticalSpeeds, CurrentNote.GetTimeFinal()) );

					// if upscroll change minus for plus as well as matrix at screengameplay7k
					if (!CurrentNote.IsHold())
						CurrentNote.AssignPosition( -VerticalPosition);
					else
						CurrentNote.AssignPosition( -VerticalPosition, -HoldEndPosition);
				}
				MIdx++;
			}
		}
	}

	PreviousDrift = Drift;
	Processed = true;
}
