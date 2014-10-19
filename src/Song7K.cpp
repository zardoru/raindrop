#include <map>
#include <fstream>

#include "GameGlobal.h"
#include "Song7K.h"
#include "Configuration.h"
#include <algorithm>

using namespace VSRG;

Song::Song()
{
	PreviousDrift = 0;
	Processed = false;
	Mode = MODE_7K;
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

int tSort(const TimingSegment &i, const TimingSegment &j)
{
	return i.Time < j.Time;
}

int nSort (const TrackNote &i, const TrackNote &j)
{
	return i.GetStartTime() < j.GetStartTime();
}

void Song::ProcessVSpeeds(VSRG::Difficulty* Diff, double SpeedConstant)
{
	Diff->VerticalSpeeds.clear();

	if (SpeedConstant) // We're using a CMod, so further processing is pointless
	{
		TimingSegment VSpeed;
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
		TimingSegment VSpeed;

		if (Time->Value)
		{
			float spb = 1 / Time->Value;
			VerticalSpeed = MeasureBaseSpacing / (spb * 4);
		}else
			VerticalSpeed = 0;

		VSpeed.Value = VerticalSpeed;
		VSpeed.Time = Time->Time;

		Diff->VerticalSpeeds.push_back(VSpeed);
	}

	// Let first speed be not-null.
	if (Diff->VerticalSpeeds.size() && Diff->VerticalSpeeds[0].Value == 0)
	{
		for (TimingData::iterator i = Diff->VerticalSpeeds.begin(); 
			i != Diff->VerticalSpeeds.end();
			i++)
		{
			if (i->Value != 0)
				Diff->VerticalSpeeds[0].Value = i->Value;
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

	assert (0); // Never happens.
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
}

void Song::ProcessBPS(VSRG::Difficulty* Diff, double Drift)
{
	/* 
		Calculate BPS. The algorithm is basically the same as VSpeeds.
		BPS time is calculated applying the offset.
	*/

	Diff->BPS.clear();

	for(TimingData::iterator Time = Diff->Timing.begin();
		Time != Diff->Timing.end();
		Time++)
	{
		TimingSegment Seg;
		
		Seg.Time = TimeFromTimingKind(Diff->Timing, Diff->StopsTiming, *Time, Diff->BPMType, Diff->Offset, Drift);
		Seg.Value = BPSFromTimingKind(Time->Value, Diff->BPMType);

		Diff->BPS.push_back(Seg);
	}

	/* Sort for justice */
	std::sort(Diff->BPS.begin(), Diff->BPS.end(), tSort);

	if (!Diff->StopsTiming.size() || Diff->BPMType != VSRG::Difficulty::BT_Beat) // Stops only supported in Beat mode.
		return;

	/* Here on, just working with stops. */
	for(TimingData::iterator Time = Diff->StopsTiming.begin();
		Time != Diff->StopsTiming.end();
		Time++)
	{
		TimingSegment Seg;
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

		float speedRestore = bps(SectionValue(Diff->Timing, Time->Time));

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

void Song::ProcessSpeedVariations(VSRG::Difficulty* Diff, double Drift)
{
	TimingData tVSpeeds = Diff->VerticalSpeeds; // We need this to store what values to change

	std::sort( Diff->SpeedChanges.begin(), Diff->SpeedChanges.end(), tSort );

	for(TimingData::const_iterator Change = Diff->SpeedChanges.begin();
			Change != Diff->SpeedChanges.end();
			Change++)
	{
		TimingData::const_iterator NextChange = (Change+1);
		double ChangeTime = Change->Time + Drift + Diff->Offset;

		/* 
			Find all VSpeeds
			if there exists a speed change which is virtually happening at the same time as this VSpeed
			modify it to be this value * factor
		*/

		for(TimingData::iterator Time = Diff->VerticalSpeeds.begin();
			Time != Diff->VerticalSpeeds.end();
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

		Diff->VerticalSpeeds.push_back(VSpeed);

		/* 
			Theorically, if there were a VSpeed change after this one (such as a BPM change) we've got to modify them 
			if they're between this and the next speed change.

			Apparently, this behaviour is a "bug" since osu!mania reset SV changes
			after a BPM change.
		*/

		if (Diff->BPMType == VSRG::Difficulty::BT_Beatspace)
			goto next_speed;

		for(TimingData::iterator Time = Diff->VerticalSpeeds.begin();
			Time != Diff->VerticalSpeeds.end();
			Time++)
		{
			if (Time->Time > ChangeTime)
			{
				// Two options, between two speed changes, or the last one. Second case, NextChange == SpeedChanges.end().
				// Otherwise, just move on
				// Last speed change
				if (NextChange == Diff->SpeedChanges.end())
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

	std::sort(Diff->VerticalSpeeds.begin(), Diff->VerticalSpeeds.end(), tSort);
}

void Song::Process(VSRG::Difficulty* Which, VectorTN NotesOut, float Drift, double SpeedConstant)
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
	*/

	int ApplyDriftVirtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
	int ApplyDriftDecoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");
	double rDrift = Drift - PreviousDrift;

	/* For all difficulties */
	for (std::vector<VSRG::Difficulty*>::iterator Diff = Difficulties.begin(); Diff != Difficulties.end(); Diff++)
	{
		if (!(*Diff)->Timing.size() || Which != *Diff)
			continue;

		if ( (!ApplyDriftVirtual && (*Diff)->IsVirtual) || (!ApplyDriftDecoder && !(*Diff)->IsVirtual) )
			Drift = 0;
		else
			Drift = rDrift;

		ProcessBPS(*Diff, Drift);
		ProcessVSpeeds(*Diff, SpeedConstant);

		if (!SpeedConstant) // If there is a speed constant having speed changes is not what we want
			ProcessSpeedVariations(*Diff, Drift);


		// From here on, we'll just copy the notes out. Otherwise, just leave the processed data.
		if (!NotesOut)
			return;

		for (int KeyIndex = 0; KeyIndex < (*Diff)->Channels; KeyIndex++)
			NotesOut[KeyIndex].clear();

		/* For all channels of this difficulty */
		for (int KeyIndex = 0; KeyIndex < (*Diff)->Channels; KeyIndex++)
		{
			int MIdx = 0;

			/* For each measure of this channel */
			for (std::vector<VSRG::Measure>::iterator Msr = (*Diff)->Measures.begin(); 
				Msr != (*Diff)->Measures.end();
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

					Vec2 VerticalPosition( 0, IntegrateToTime((*Diff)->VerticalSpeeds, CurrentNote.StartTime + Drift) );
					Vec2 HoldEndPosition( 0, IntegrateToTime((*Diff)->VerticalSpeeds, CurrentNote.EndTime + Drift) );

					// if upscroll change minus for plus as well as matrix at screengameplay7k
					if (!CurrentNote.EndTime)
						NewNote.AssignPosition( -VerticalPosition);
					else
						NewNote.AssignPosition( -VerticalPosition, -HoldEndPosition);

					double cBeat = BeatAtTime((*Diff)->BPS, CurrentNote.StartTime, (*Diff)->Offset + Drift);
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

	Processed = true;
	PreviousDrift = Drift;
}

void Difficulty::GetMeasureLines(std::vector<float> &Out, float Drift)
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
		}else
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
	VSRG::MeasureVector MV;
	std::vector<AutoplaySound> BGM;
	std::vector <AutoplayBMP> BMP;
	std::vector <AutoplayBMP> BMPL;
	std::vector <AutoplayBMP> BMPL2;
	std::vector <AutoplayBMP> BMPM;

	Timing.swap(S1);
	SpeedChanges.swap(S2);
	StopsTiming.swap(S3);
	BGMEvents.swap(BGM);
	Measures.swap(MV);
	BMPEvents.swap(BMP);
	BMPEventsLayer.swap(BMPL);
	BMPEventsLayer2.swap(BMPL2);
	BMPEventsMiss.swap(BMPM);

	BMPList.clear();
	Filename.clear();
	SoundList.clear();
}
