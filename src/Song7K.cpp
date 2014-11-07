#include <map>
#include <fstream>

#include "GameGlobal.h"
#include "Song7K.h"
#include "Configuration.h"
#include <algorithm>

using namespace VSRG;

TimingInfoType CustomTimingInfo::GetType()
{
	return Type;
}

Song::Song()
{
	Mode = MODE_VSRG;
}

Song::~Song()
{
	for (std::vector<VSRG::Difficulty*>::iterator i = Difficulties.begin();
		i != Difficulties.end();
		i++)
	{
		delete *i;
	}
}

VSRG::Difficulty* Song::GetDifficulty(uint32 i)
{
	if (i >= Difficulties.size())
		return NULL;
	else
		return Difficulties.at(i);
}

int tSort(const TimingSegment &i, const TimingSegment &j)
{
	return i.Time < j.Time;
}

int nSort (const TrackNote &i, const TrackNote &j)
{
	return i.GetStartTime() < j.GetStartTime();
}

void Difficulty::ProcessVSpeeds(TimingData& BPS, TimingData& VerticalSpeeds, double SpeedConstant)
{
	VerticalSpeeds.clear();

	if (SpeedConstant) // We're using a CMod, so further processing is pointless
	{
		TimingSegment VSpeed;
		VSpeed.Time = 0;
		VSpeed.Value = SpeedConstant;
		VerticalSpeeds.push_back(VSpeed);
		return;
	}

	// Calculate velocity at time based on BPM at time
	for (TimingData::iterator Time = BPS.begin();
		Time != BPS.end();
		Time++)
	{
		float VerticalSpeed;
		TimingSegment VSpeed;

		if (Time->Value)
		{
			float spb = 1 / Time->Value;
			VerticalSpeed = MeasureBaseSpacing / (spb * 4);
		}else
			VerticalSpeed = 0;

		VSpeed.Value = VerticalSpeed;
		VSpeed.Time = Time->Time;

		VerticalSpeeds.push_back(VSpeed);
	}

	// Let first speed be not-null.
	if (VerticalSpeeds.size() && VerticalSpeeds[0].Value == 0)
	{
		for (TimingData::iterator i = VerticalSpeeds.begin(); 
			i != VerticalSpeeds.end();
			i++)
		{
			if (i->Value != 0)
				VerticalSpeeds[0].Value = i->Value;
		}
	}
}

double TimeFromTimingKind(const TimingData &Timing, 
	const TimingData &StopsTiming,
	const TimingSegment& S, 
	VSRG::Difficulty::EBt TimingType, 
	float Offset, 
	float Drift)
{
	if (TimingType == VSRG::Difficulty::BT_Beat) // Time is in Beats
	{
		return TimeAtBeat(Timing, Drift + Offset, S.Time) + StopTimeAtBeat(StopsTiming, S.Time);
	}else if (TimingType == VSRG::Difficulty::BT_MS || TimingType == VSRG::Difficulty::BT_Beatspace) // Time is in MS
	{
		return S.Time + Drift + Offset;
	}

	assert (0); // Never happens. Must never happen ever ever.
	return 0;
}

double BPSFromTimingKind(float Value, VSRG::Difficulty::EBt TimingType)
{
	if (TimingType == VSRG::Difficulty::BT_Beat || TimingType == VSRG::Difficulty::BT_MS) // Time is in Beats
	{
		return bps (Value);
	}else if ( TimingType == VSRG::Difficulty::BT_Beatspace ) // Time in MS, and not using bpm, but ms per beat.
	{
		return bps (60000.0 / Value);
	}

	assert (0);
	return 0; // shut up compiler
}

void Difficulty::ProcessBPS(TimingData& BPS, double Drift)
{
	/* 
		Calculate BPS. The algorithm is basically the same as VSpeeds.
		BPS time is calculated applying the offset.
	*/

	BPS.clear();

	for(TimingData::iterator Time = Timing.begin();
		Time != Timing.end();
		Time++)
	{
		TimingSegment Seg;
		
		Seg.Time = TimeFromTimingKind(Timing, StopsTiming, *Time, BPMType, Offset, Drift);
		Seg.Value = BPSFromTimingKind(Time->Value, BPMType);

		BPS.push_back(Seg);
	}

	/* Sort for justice */
	std::sort(BPS.begin(), BPS.end(), tSort);

	if (!StopsTiming.size() || BPMType != VSRG::Difficulty::BT_Beat) // Stops only supported in Beat mode.
		return;

	/* Here on, just working with stops. */
	for(TimingData::iterator Time = StopsTiming.begin();
		Time != StopsTiming.end();
		Time++)
	{
		TimingSegment Seg;
		float TValue = TimeAtBeat(Timing, Offset + Drift, Time->Time) + StopTimeAtBeat(StopsTiming, Time->Time);
		float TValueN = TimeAtBeat(Timing, Offset + Drift, Time->Time) + StopTimeAtBeat(StopsTiming, Time->Time) + Time->Value;

		/* Initial Stop */
		Seg.Time = TValue;
		Seg.Value = 0;

		/* First, eliminate collisions. */
		for (TimingData::iterator k = BPS.begin(); k != BPS.end(); k++)
		{
			if ( abs(k->Time - TValue) < 0.000001 ) /* Too close? Remove the collision, leaving only the 0 in front. */
			{
				k = BPS.erase(k);

				if (k == BPS.end())
					break;
			}
		}

		BPS.push_back(Seg);

		float speedRestore = bps(SectionValue(Timing, Time->Time));

		for (TimingData::iterator k = BPS.begin(); k != BPS.end(); k++)
		{
			if (k->Time > TValue && k->Time < TValueN)
			{
				speedRestore = k->Value; /* This is the last speed change in the interval that the stop lasts. We'll use it. */

				/* Eliminate this since we're not going to use it. */
				k = BPS.erase(k);

				if (k == BPS.end())
					break;
			}
		}

		/* Restored speed after stop */
		Seg.Time = TValueN;
		Seg.Value = speedRestore;
		BPS.push_back(Seg);
	}

	std::sort(BPS.begin(), BPS.end(), tSort);
}

void Difficulty::ProcessSpeedVariations(TimingData& BPS, TimingData& VerticalSpeeds, double Drift)
{
	TimingData tVSpeeds = VerticalSpeeds; // We need this to store what values to change

	std::sort( SpeedChanges.begin(), SpeedChanges.end(), tSort );

	for(TimingData::const_iterator Change = SpeedChanges.begin();
			Change != SpeedChanges.end();
			Change++)
	{
		TimingData::const_iterator NextChange = (Change+1);
		double ChangeTime = Change->Time + Drift + Offset;

		/* 
			Find all VSpeeds
			if there exists a speed change which is virtually happening at the same time as this VSpeed
			modify it to be this value * factor
		*/

		for(TimingData::iterator Time = VerticalSpeeds.begin();
			Time != VerticalSpeeds.end();
			Time++)
		{
			if ( abs(ChangeTime - Time->Time) < 0.00001)
			{
				Time->Value *= Change->Value;
				goto next_speed;
			}
		}

		/* 
			There are no collisions- insert a new speed at this time
		*/

		if (ChangeTime < 0)
			goto next_speed;

		float SpeedValue;
		SpeedValue = SectionValue(tVSpeeds, ChangeTime) * Change->Value;

		TimingSegment VSpeed;

		VSpeed.Time = ChangeTime;
		VSpeed.Value = SpeedValue;

		VerticalSpeeds.push_back(VSpeed);

		/* 
			Theorically, if there were a VSpeed change after this one (such as a BPM change) we've got to modify them 
			if they're between this and the next speed change.

			Apparently, this behaviour is a "bug" since osu!mania reset SV changes
			after a BPM change.
		*/

		if (BPMType == VSRG::Difficulty::BT_Beatspace) // Okay, we're an osu!mania chart, leave the resetting.
			goto next_speed;

		// We're not an osu!mania chart, so it's time to do what should be done.
		for(TimingData::iterator Time = VerticalSpeeds.begin();
			Time != VerticalSpeeds.end();
			Time++)
		{
			if (Time->Time > ChangeTime)
			{
				// Two options, between two speed changes, or the last one. Second case, NextChange == SpeedChanges.end().
				// Otherwise, just move on
				// Last speed change
				if (NextChange == SpeedChanges.end())
				{
					Time->Value = Change->Value * SectionValue(tVSpeeds, Time->Time);
				}else
				{
					if (Time->Time < NextChange->Time) // Between speed changes
						Time->Value = Change->Value * SectionValue(tVSpeeds, Time->Time);
				}
			}
		}

		next_speed: (void)0;
	}

	std::sort(VerticalSpeeds.begin(), VerticalSpeeds.end(), tSort);
}

void Difficulty::Process(VectorTN NotesOut, TimingData &BPS, TimingData& VerticalSpeeds, float Drift, double SpeedConstant)
{
	/* 
		We'd like to build the notes' position from 0 to infinity, 
		however the real "zero" position would be the judgment line
		in other words since "up" is negative relative to 0
		and 0 is the judgment line
		position would actually be
		judgeline - positiveposition
		and positiveposition would just be
		measure * measuresize + fraction * fractionsize

		In practice, since we use a ms-based model rather than a beat one,
		we just do regular integration of 
		position = sum(speed_i * duration_i) + speed_current * (time_current - speed_start_time)
	*/

	int ApplyDriftVirtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
	int ApplyDriftDecoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");
	double rDrift = Drift;

	if ((!ApplyDriftVirtual && IsVirtual) || (!ApplyDriftDecoder && !IsVirtual))
		Drift = 0;
	else
		Drift = rDrift;

	ProcessBPS(BPS, Drift);
	ProcessVSpeeds(BPS, VerticalSpeeds, SpeedConstant);

	if (!SpeedConstant) // If there is a speed constant having speed changes is not what we want
		ProcessSpeedVariations(BPS, VerticalSpeeds, Drift);


	// From here on, we'll just copy the notes out. Otherwise, just leave the processed data.
	if (!NotesOut)
		return;

	for (int KeyIndex = 0; KeyIndex < Channels; KeyIndex++)
		NotesOut[KeyIndex].clear();

	/* For all channels of this difficulty */
	for (int KeyIndex = 0; KeyIndex < Channels; KeyIndex++)
	{
		int MIdx = 0;

		/* For each measure of this channel */
		for (std::vector<VSRG::Measure>::iterator Msr = Measures.begin();
			Msr != Measures.end();
			Msr++)
		{
			/* For each note in the measure... */
			size_t total_notes = Msr->MeasureNotes[KeyIndex].size();
			for (uint32 Note = 0; Note < total_notes; Note++)
			{
				/*
					Calculate position. (Change this to TrackNote instead of processing?)
					issue is not having the speed change data there.
					*/
				NoteData &CurrentNote = (*Msr).MeasureNotes[KeyIndex][Note];
				TrackNote NewNote;

				NewNote.AssignNotedata(CurrentNote);
				NewNote.AddTime(Drift);

				float VerticalPosition = IntegrateToTime(VerticalSpeeds, CurrentNote.StartTime + Drift);
				float HoldEndPosition = IntegrateToTime(VerticalSpeeds, CurrentNote.EndTime + Drift);

				// if upscroll change minus for plus as well as matrix at screengameplay7k
				if (!CurrentNote.EndTime)
					NewNote.AssignPosition(-VerticalPosition);
				else
					NewNote.AssignPosition(-VerticalPosition, -HoldEndPosition);

				// Okay, now we want to know what fraction of a beat we're dealing with
				// this way we can display colored (a la Stepmania) notes.
				double cBeat = BeatAtTime(BPS, CurrentNote.StartTime, Offset + Drift);
				double iBeat = floor(cBeat);
				double dBeat = cBeat - iBeat;

				NewNote.AssignFraction(dBeat);
				NotesOut[KeyIndex].push_back(NewNote);
			}

			std::sort(NotesOut[KeyIndex].begin(), NotesOut[KeyIndex].end(), nSort);
			MIdx++;
		}
	}
}

void Difficulty::GetMeasureLines(std::vector<float> &Out, TimingData& VerticalSpeeds, float Drift)
{
	float Last = 0;

	Out.reserve(Measures.size());

	for (std::vector<VSRG::Measure>::iterator Msr = Measures.begin();
		Msr != Measures.end();
		Msr++)
	{
		float PositionOut = 0;

		if (BPMType == BT_Beat)
		{
			PositionOut = IntegrateToTime(VerticalSpeeds, TimeAtBeat(Timing, Offset, Last) + StopTimeAtBeat(StopsTiming, Last));
		}
		else
		{
			// PositionOut = IntegrateToTime(VerticalSpeeds, /* ???? */);
		}

		Out.push_back(PositionOut);
		Last += Msr->MeasureLength;
	}
}

void Difficulty::Destroy()
{
	// Do the swap to try and force memory release.
	TimingData S1, S2, S3;
	
	std::vector<AutoplaySound> BGM;

	Timing.swap(S1);
	SpeedChanges.swap(S2);
	StopsTiming.swap(S3);
	BGMEvents.swap(BGM);
	
	Author.clear();
	Filename.clear();
	SoundList.clear();

	delete TimingInfo;
	delete BMPEvents;
	TimingInfo = NULL;
	BMPEvents = NULL;

	DestroyNotes();
}

void Difficulty::DestroyNotes()
{
	VSRG::MeasureVector MV;
	Measures.swap(MV);
}