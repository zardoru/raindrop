#include <map>

#include "GameGlobal.h"
#include "Song7K.h"
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
}

VSRG::Difficulty* Song::GetDifficulty(uint32 i)
{
	if (i >= Difficulties.size())
		return nullptr;
	else
		return Difficulties.at(i).get();
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
	for (auto Time = BPS.begin();
		Time != BPS.end();
		++Time)
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
		VSpeed.Time = Time->Time; // We blindly take the BPS time that had offset and drift applied.

		VerticalSpeeds.push_back(VSpeed);
	}

	// Let first speed be not-null.
	if (VerticalSpeeds.size() && VerticalSpeeds[0].Value == 0)
	{
		for (auto i = VerticalSpeeds.begin(); 
			i != VerticalSpeeds.end();
		     ++i)
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
		BPS time is calculated applying the offset and drift.
	*/
	assert(Data != NULL);

	TimingData &StopsTiming = Data->StopsTiming;

	BPS.clear();

	for(auto Time = Timing.begin();
		Time != Timing.end();
	    ++Time)
	{
		TimingSegment Seg;
		
		Seg.Time = TimeFromTimingKind(Timing, StopsTiming, *Time, BPMType, Offset, Drift);
		Seg.Value = BPSFromTimingKind(Time->Value, BPMType);

		BPS.push_back(Seg);
	}

	/* Sort for justice */
	std::sort(BPS.begin(), BPS.end());

	if (!StopsTiming.size() || BPMType != VSRG::Difficulty::BT_Beat) // Stops only supported in Beat mode.
		return;

	/* Here on, just working with stops. */
	for(auto Time = StopsTiming.begin();
		Time != StopsTiming.end();
		++Time)
	{
		TimingSegment Seg;
		double TValue = TimeAtBeat(Timing, Offset + Drift, Time->Time) + StopTimeAtBeat(StopsTiming, Time->Time);
		double TValueN = TimeAtBeat(Timing, Offset + Drift, Time->Time) + StopTimeAtBeat(StopsTiming, Time->Time) + Time->Value;

		/* Initial Stop */
		Seg.Time = TValue;
		Seg.Value = 0;

		/* First, eliminate collisions. */
		for (auto k = BPS.begin(); k != BPS.end();)
		{
			if ( k->Time == TValue ) /* Equal? Remove the collision, leaving only the 0 in front. */
			{
				k = BPS.erase(k);

				if (k == BPS.end())
					break;
				else continue;
			}

			++k;
		}

		// Okay, the collision is out. Let's push our 0-speeder.
		BPS.push_back(Seg);

		// Now we find what bps to restore to.
		float bpsRestore = bps(SectionValue(Timing, Time->Time));

		for (TimingData::iterator k = BPS.begin(); k != BPS.end(); )
		{
			if (k->Time > TValue && k->Time <= TValueN) // So wait, there's BPM changes in between? Holy shit.
			{
				bpsRestore = k->Value; /* This is the last speed change in the interval that the stop lasts. We'll use it. */

				/* Eliminate this since we're not going to use it. */
				k = BPS.erase(k);

				if (k == BPS.end())
					break;
				else continue;
			}

			k++;
		}

		/* Restored speed after stop */
		Seg.Time = TValueN;
		Seg.Value = bpsRestore;
		BPS.push_back(Seg);
	}

	std::sort(BPS.begin(), BPS.end());
}

void Difficulty::ProcessSpeedVariations(TimingData& BPS, TimingData& VerticalSpeeds, double Drift)
{
	assert(Data != NULL);

	TimingData tVSpeeds = VerticalSpeeds; // We need this to store what values to change
	TimingData &SpeedChanges = Data->SpeedChanges;

	std::sort( SpeedChanges.begin(), SpeedChanges.end() );

	for(TimingData::const_iterator Change = SpeedChanges.begin();
			Change != SpeedChanges.end();
	    ++Change)
	{
		TimingData::const_iterator NextChange = (Change+1);
		double ChangeTime = Change->Time + Drift + Offset;

		/* 
			Find all VSpeeds
			if there exists a speed change which is virtually happening at the same time as this VSpeed
			modify it to be this value * factor
		*/

		bool MoveOn = false;
		for(auto Time = VerticalSpeeds.begin();
			Time != VerticalSpeeds.end();
		    ++Time)
		{
			if ( abs(ChangeTime - Time->Time) < 0.00001)
			{
				Time->Value *= Change->Value;
				MoveOn = true;
			}
		}

		if (MoveOn) continue;

		/* 
			There are no collisions- insert a new speed at this time
		*/

		if (ChangeTime < 0)
			continue;

		float SpeedValue;
		SpeedValue = SectionValue(tVSpeeds, ChangeTime) * Change->Value;

		TimingSegment VSpeed;

		VSpeed.Time = ChangeTime;
		VSpeed.Value = SpeedValue;

		VerticalSpeeds.push_back(VSpeed);

		/* 
			Theorically, if there were a VSpeed change after this one (such as a BPM change) we've got to modify them 
			if they're between this and the next speed change.

			Apparently, this behaviour is a "bug" since osu!mania resets SV changes
			after a BPM change.
		*/

		if (BPMType == VSRG::Difficulty::BT_Beatspace) // Okay, we're an osu!mania chart, leave the resetting.
			continue;

		// We're not an osu!mania chart, so it's time to do what should be done.
		for(auto Time = VerticalSpeeds.begin();
			Time != VerticalSpeeds.end();
		    ++Time)
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
	}

	std::sort(VerticalSpeeds.begin(), VerticalSpeeds.end());
}

double Difficulty::GetWarpAmountAtTime(double Time)
{
	double wAmt = 0;
	for (auto warp : Data->Warps)
	{
		if (warp.Time < Time)
			wAmt += warp.Value;
	}

	return wAmt;
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

	assert(Data != NULL);

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
		for (auto Msr = Data->Measures.begin();
			Msr != Data->Measures.end();
			++Msr)
		{
			/* For each note in the measure... */
			ptrdiff_t total_notes = Msr->MeasureNotes[KeyIndex].size();

			for (auto Note = 0; Note < total_notes; Note++)
			{
				/*
					Calculate position. (Change this to TrackNote instead of processing?)
					issue is not having the speed change data there.
				*/
				NoteData &CurrentNote = (*Msr).MeasureNotes[KeyIndex][Note];
				TrackNote NewNote;

				NewNote.AssignNotedata(CurrentNote);
				NewNote.AddTime(Drift);

				float VerticalPosition = IntegrateToTime(VerticalSpeeds, CurrentNote.StartTime);
				float HoldEndPosition = IntegrateToTime(VerticalSpeeds, CurrentNote.EndTime);

				// if upscroll change minus for plus as well as matrix at screengameplay7k
				if (!CurrentNote.EndTime)
					NewNote.AssignPosition(VerticalPosition);
				else
					NewNote.AssignPosition(VerticalPosition, HoldEndPosition);

				// Okay, now we want to know what fraction of a beat we're dealing with
				// this way we can display colored (a la Stepmania) notes.
				double cBeat = IntegrateToTime(BPS, CurrentNote.StartTime);
				double iBeat = floor(cBeat);
				double dBeat = (cBeat - iBeat);

				NewNote.AssignFraction(dBeat);

				double Wamt = -GetWarpAmountAtTime(CurrentNote.StartTime);
				NewNote.AddTime(Wamt);
				NotesOut[KeyIndex].push_back(NewNote);
			}

			MIdx++;
		}

		// done with the channel - sort it
		std::stable_sort(NotesOut[KeyIndex].begin(), NotesOut[KeyIndex].end(), [](const VSRG::TrackNote &A, const VSRG::TrackNote &B) -> bool { return A.GetVertical() < B.GetVertical(); });
	}
}

void BPStoSPB(TimingData &BPS)
{
	auto BPSCopy = BPS;
	for (auto i = BPS.begin(); i != BPS.end(); ++i)
	{
		double valueBPS = i->Value;
		i->Value = 1 / valueBPS;
		i->Time = IntegrateToTime(BPSCopy, i->Time); // Find time in beats based off beats in time
	}
}

void Difficulty::GetMeasureLines(std::vector<float> &Out, TimingData& VerticalSpeeds, double WaitTime)
{
	double Last = 0;
	TimingData SPB;
	ProcessBPS(SPB, 0);
	BPStoSPB(SPB);

	if (!Data)
		return;

	if (!Timing.size())
		return;

	if (!Data->Measures.size())
		return;

	Out.reserve(Data->Measures.size());

	// Add lines before offset, and during waiting time...
	double BPS = BPSFromTimingKind(Timing[0].Value, BPMType);
	double PreTime = WaitTime + Offset;
	double PreTimeBeats = BPS * PreTime;
	int TotMeasures = PreTimeBeats / Data->Measures[0].MeasureLength;
	double MeasureTime = 1 / BPS * Data->Measures[0].MeasureLength;
	
	for (int i = 0; i < TotMeasures; i++)
	{
		float PositionOut;
		PositionOut = IntegrateToTime(VerticalSpeeds, Offset - MeasureTime * i);
		Out.push_back(PositionOut);
	}


	for (auto Msr = Data->Measures.begin();
		Msr != Data->Measures.end();
		++Msr)
	{
		float PositionOut = 0;

		if (BPMType == BT_Beat) // VerticalSpeeds already has drift applied, so we don't need to apply it again here.
		{
			PositionOut = IntegrateToTime(VerticalSpeeds, TimeAtBeat(Timing, Offset, Last) + StopTimeAtBeat(Data->StopsTiming, Last));
		}
		else if (BPMType == BT_Beatspace)
		{
			double TargetTime = 0;

			TargetTime = IntegrateToTime(SPB, Last) + Offset;
			PositionOut = IntegrateToTime(VerticalSpeeds, TargetTime);
		}

		Out.push_back(PositionOut);
		Last += Msr->MeasureLength;
	}
}

void Difficulty::Destroy()
{
	if (Data)
		Data = nullptr;
	
	Timing.clear(); Timing.shrink_to_fit();
	Author.clear(); Author.shrink_to_fit();
	Filename.clear(); Filename.shrink_to_fit();

	SoundList.clear();
}