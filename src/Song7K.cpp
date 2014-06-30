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
	UseSeparateTimingData = false;
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


void Song::ProcessBPS(VSRG::Difficulty* Diff, double Drift)
{
	/* 
		Calculate BPS. The algorithm is basically the same as VSpeeds, so there's probably a better way to do it
		that is not repeating the same thing using different values.
	*/
	Diff->BPS.clear();

	for(TimingData::iterator Time = Diff->Timing.begin();
		Time != Diff->Timing.end();
		Time++)
	{
		TimingSegment Seg;

		float FTime;

		if (Diff->BPMType == VSRG::Difficulty::BT_Beat) // Time is in Beats
		{
			FTime = bps (Time->Value);
			Seg.Time = TimeAtBeat(Diff->Timing, Diff->Offset + Drift, Time->Time) + StopTimeAtBeat(Diff->StopsTiming, Time->Time);
		}
		else if (Diff->BPMType == VSRG::Difficulty::BT_MS) // Time is in MS
		{
			FTime = bps (Time->Value);
			Seg.Time = Time->Time + Drift;
		}else if ( Diff->BPMType == VSRG::Difficulty::BT_Beatspace ) // Time in MS, and not using bpm, but ms per beat.
		{
			FTime = bps (60000.0 / Time->Value);
			Seg.Time = Time->Time + Drift;
		}

		Seg.Value = FTime;
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
		double ChangeTime = Change->Time + Drift;

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

			Apparently, this behaviour is a "bug" since both stepmania and osu!mania reset SV changes
			after a BPM change.
		*/

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
		however the real "zero" position would be the judgement line
		in other words since "up" is negative relative to 0
		and 0 is the judgement line
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


/*

	The convention for writing cache data is

	Sounds list:
	index,filename

	Timing:
	time,value

	Speed Changes:
	time,value

	Stops:
	time,value

	BGM events:
	time,value

	Notes: (measure separator: @)
	track,start,end,sound


	separated by a newline only writing the character $.
*/

using std::vector;

void WriteTimingData(const TimingData &Data, std::fstream& out)
{
	for (TimingData::const_iterator i = Data.begin(); i != Data.end(); i++)
		out << i->Time << "," << i->Value << std::endl;
}

#define SEPARATOR "$" << std::endl
#define MSSEPARATOR "@" << std::endl
bool Difficulty::SaveCache(String filename)
{
	if (/*!IsLoaded || */ParseAgain)
		return false;

#ifndef WIN32
	std::fstream out(filename.c_str(), std::ios::out);
#else
	std::fstream out(Utility::Widen(filename).c_str(), std::ios::out);
#endif

	for (std::map<int, String>::iterator i = SoundList.begin();
		i != SoundList.end();
		i++)
	{
		out << i->first << "," << i->second << std::endl;
	}

	out << SEPARATOR;
	WriteTimingData(Timing, out);
	out << SEPARATOR;
	WriteTimingData(SpeedChanges, out);
	out << SEPARATOR;
	WriteTimingData(StopsTiming, out);
	out << SEPARATOR;

	for (std::vector<AutoplaySound>::iterator i = BGMEvents.begin(); i != BGMEvents.end(); i++)
		out << i->Time << "," << i->Sound << std::endl;

	out << SEPARATOR;

	for (MeasureVector::iterator i = Measures.begin(); i != Measures.end(); i++)
	{
		out << MSSEPARATOR;

		for (char k = 0; k < MAX_CHANNELS; k++)
			for (std::vector<NoteData>::iterator n = i->MeasureNotes[k].begin(); n != i->MeasureNotes[k].end(); n++)
			{
				out << (int)k << "," << n->StartTime << "," << n->EndTime << "," << n->Sound << std::endl;
			}
	}

	return true;
}

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

using boost::split;
using boost::lexical_cast;

void LoadTimingData(TimingData& Timing, std::ifstream &in)
{
	String line;
	while (std::getline(in, line))
	{
		if (line == "$")
			break;
		else
		{
			vector<String> res;
			split(res, line, boost::is_any_of(","));
			double time = lexical_cast<double> (res[0]);
			double value = lexical_cast<double> (res[1]);
			TimingSegment S;
			S.Time = time;
			S.Value = value;
			Timing.push_back(S);
		}
	}
}

bool Difficulty::LoadCache(String filename)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream in (filename.c_str());
#else
	std::ifstream in (Utility::Widen(filename).c_str());
#endif

	if (!in.is_open())
		return false;

	String line;
	while (std::getline(in, line))
	{
		if (line == "$")
			break;
		else
		{
			vector<String> res;
			split(res, line, boost::is_any_of(","));
			int index = lexical_cast<int>(res[0].c_str());
			SoundList[index] = res[1];
		}
	}

	LoadTimingData(Timing, in);
	LoadTimingData(SpeedChanges, in);
	LoadTimingData(StopsTiming, in);

	while (std::getline(in, line))
	{
		if (line == "$")
			break;
		else
		{
			vector<String> res;
			split(res, line, boost::is_any_of(","));
			double Time = lexical_cast<double>(res[0]);
			int Value = lexical_cast<int>(res[1]);
			AutoplaySound S;
			S.Sound = Value;
			S.Time = Time;
			BGMEvents.push_back(S);
		}
	}

	while (std::getline(in, line))
	{
		if (line == "$")
			break;
		else
		{
			if (line == "@")
			{
				Measure M;
				Measures.push_back(M);
			}else
			{
				vector<String> res;
				split(res, line, boost::is_any_of(","));
				int Track = lexical_cast<int> (res[0]);
				double startTime = lexical_cast<double>(res[1]);
				double endTime = lexical_cast<double>(res[2]);
				int Sound = lexical_cast<int> (res[3]);
				NoteData N;
				N.Sound = Sound;
				N.StartTime = startTime;
				N.EndTime = endTime;
				Measures.back().MeasureNotes[Track].push_back(N);
			}
		}
	}

	return true;
}

void Difficulty::Destroy()
{
	SoundList.clear();
	Timing.clear();
	SpeedChanges.clear();
	StopsTiming.clear();
	BGMEvents.clear();
	Measures.clear();
	Filename.clear();
}

#include "FileManager.h"

String Song::DifficultyCacheFilename(VSRG::Difficulty * Diff)
{
	std::string dfName = Diff->Name;
	Utility::RemoveFilenameIllegalCharacters(dfName, true);

	String fnA = FileManager::GetCacheFilename(Diff->Filename, dfName);
	Utility::RemoveFilenameIllegalCharacters(fnA);
	return fnA;
}