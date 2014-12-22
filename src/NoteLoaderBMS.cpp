#include <fstream>
#include <map>
#include <ctime>
#include <stack>

#include "GameGlobal.h"
#include "Logging.h"
#include "Song7K.h"
#include "NoteLoader7K.h"
#include "utf8.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

/*
	Source for implemented commands:
	http://hitkey.nekokan.dyndns.info/cmds.htm

	A huge lot of not-frequently-used commands are not going to be implemented.

	On 5K BMS, scratch is usually channel 16/26 for P1/P2
	17/27 for foot pedal- no exceptions
	keys 6 and 7 are 21/22 in bms
	keys 6 and 7 are 18/19, 28/29 in bme

	two additional extensions to BMS are planned for raindrop to introduce compatibility
	with SV changes:
	#SCROLLxx <value>
	#SPEEDxx <value> <duration>

	and are to be put under channel SV (base 36)

	Since most information is in japanese it's likely the implementation won't be perfect at the start.
*/


		/*
		switch (BmsChannel)
		{
		case 1:
			// BGM change.
			break;
		case 3:
			// BPM change.

			break;
		case 9:
			// Stop event.
			break;
		default:

			/* 
				Since we're working with constants..
				All in base 36
				(11 to 1Z visible p1 notes)
				(21 to 2Z visible p2 notes
				(31 to 3Z invisible p1 notes)
				(41 to 4Z invisible p2 notes)
			*/
		/*
			if (BmsChannel >= 37 && BmsChannel <= 71)
			{
				// Player 1
			}

			/* 
			
				51 to 5Z p1 longnotes
				61 to 6Z p2 longnotes

			*/

		/*
			if (BmsChannel >= 181 && BmsChannel <= 215)
			{
				// Player 1 Longnote
			}

			*/


/* literally pasted from wikipedia */
GString tob36(long unsigned int value)
{
	const char base36[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // off by 1 lol
	char buffer[14];
	unsigned int offset = sizeof(buffer);
 
	buffer[--offset] = '\0';
	do {
		buffer[--offset] = base36[value % 36];
	} while (value /= 36);
 
	return GString(&buffer[offset]);
}

int fromBase36(const char *txt)
{
	return strtoul(txt, NULL, 36);
}

int fromBase16(const char *txt)
{
	return strtoul(txt, NULL, 16);
}

int chanScratch = fromBase36("16");

// From base 36 they're 01, 02, 03 and 09.
#define CHANNEL_BGM 1
#define CHANNEL_METER 2
#define CHANNEL_BPM 3
#define CHANNEL_BGABASE 4
#define CHANNEL_BGAPOOR 6
#define CHANNEL_BGALAYER 7
#define CHANNEL_EXBPM 8
#define CHANNEL_STOPS 9
#define CHANNEL_BGALAYER2 10
#define CHANNEL_SCRATCH chanScratch 

struct BMSEvent
{
	int Event;
	float Fraction;
};

typedef std::vector<BMSEvent> BMSEventList;

struct BMSMeasure
{
	// first argument is channel, second is event list
	std::map<int, BMSEventList> Events;
	float BeatDuration;

	BMSMeasure()
	{
		BeatDuration = 1; // Default duration
	}
};

using namespace VSRG;

typedef std::map<int, GString> FilenameListIndex;
typedef std::map<int, bool> FilenameUsedIndex;
typedef std::map<int, double> BpmListIndex;
typedef std::vector<NoteData> NoteVector;
typedef std::map<int, BMSMeasure> MeasureList;

// End channels are usually xZ where X is the start (1, 2, 3 all the way to Z)

// Left side channels
const int startChannelP1 = fromBase36("11");
const int startChannelLNP1 = fromBase36("51");
const int startChannelInvisibleP1 = fromBase36("31");
const int startChannelMinesP1 = fromBase36("D1");

// Right side channels
const int startChannelP2 = fromBase36("21");
const int startChannelLNP2 = fromBase36("61");
const int startChannelInvisibleP2 = fromBase36("41");
const int startChannelMinesP2 = fromBase36("E1");

// The first wav will always be WAV01, not WAV00 since 00 is a reserved value for "nothing"
// Pretty fitting, in my opinion.



struct BmsLoadInfo
{
	FilenameListIndex Sounds;
	FilenameListIndex BMP;

	FilenameUsedIndex UsedSounds;

	BpmListIndex BPMs;
	BpmListIndex Stops;

	/*
		Used channels will be bound to this list.
		The first integer is the channel.
		Second integer is the actual measure
		Syntax in other words is
		Measures[Measure].Events[Channel].Stuff
	*/
	MeasureList Measures; 
	Song* song;
	Difficulty *difficulty;

	int LowerBound, UpperBound;
	
	float startTime[MAX_CHANNELS];

	NoteData *LastNotes[MAX_CHANNELS];

	int LNObj;
	int SideBOffset;

	uint32 RandomStack[16]; // Up to 16 nested levels.
	uint8 CurrentNestedLevel;
	uint8 SkipNestedLevel;
	bool Skip;

	bool IsPMS;

	bool HasBMPEvents;
	bool UsesTwoSides;

	BmsLoadInfo()
	{
		for (int k = 0; k < MAX_CHANNELS; k++)
		{
			startTime[k] = -1;
			LastNotes[k] = NULL;
		}
		difficulty = NULL;
		song = NULL;
		LNObj = 0;
		IsPMS = false;
		HasBMPEvents = false;
		Skip = false;
		CurrentNestedLevel = 0;
		UsesTwoSides = false;

		memset (RandomStack, 0, sizeof(RandomStack));
	}
};

GString CommandSubcontents (const GString Command, const GString Line)
{
	uint32 len = Command.length();
	return Line.substr(len);
}

void ParseEvents(BmsLoadInfo *Info, const int Measure, const int BmsChannel, const GString Command)
{
	int CommandLength = Command.length() / 2;

	if (BmsChannel != CHANNEL_METER)
	{
		size_t cSize = Info->Measures[Measure].Events[BmsChannel].size();
		Info->Measures[Measure].Events[BmsChannel].reserve(CommandLength);

		if (BmsChannel == CHANNEL_BGABASE || BmsChannel == CHANNEL_BGALAYER
			|| BmsChannel == CHANNEL_BGALAYER2 || BmsChannel == CHANNEL_BGAPOOR)
			Info->HasBMPEvents = true;

		for (int i = 0; i < CommandLength; i++)
		{
			const char *EventPtr = (Command.c_str() + i*2);
			char CharEvent [3];
			int Event;
			double Fraction = (double)i / (double)CommandLength;

			strncpy(CharEvent, EventPtr, 2); // Obtuse, but functional.
			CharEvent[2] = 0;

			Event = fromBase36(CharEvent);

			if (Event == 0) // Nothing to see here?
				continue; 
			
			BMSEvent New;

			New.Event = Event;
			New.Fraction = Fraction;

			Info->Measures[Measure].Events[BmsChannel].push_back(New);
		}
	}else // Channel 2 is a measure length event.
	{
		double Event = latof(Command);

		Info->Measures[Measure].BeatDuration = Event;
	}
}	

static double BeatForMeasure(BmsLoadInfo *Info, const int Measure)
{
	double Beat = 0;

	for (int i = 0; i < Measure; i++)
	{
		Beat += Info->Measures[i].BeatDuration * 4;
	}

	return Beat;
}

int ts_sort( const TimingSegment &A, const TimingSegment &B )
{
	return A.Time < B.Time;
}

void CalculateBMP (BmsLoadInfo *Info, std::vector<AutoplayBMP> &BMPEvents, int Channel)
{
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(Channel) != i->second.Events.end())
		{
			for (BMSEventList::iterator ev = i->second.Events[Channel].begin(); ev != i->second.Events[Channel].end(); ev++)
			{
				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first);
				int BMP = ev->Event;
				double Time = TimeAtBeat(Info->difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->difficulty->Data->StopsTiming, Beat);

				AutoplayBMP New;
				New.BMP = BMP;
				New.Time = Time;

				BMPEvents.push_back(New);
			}
		}
	}
}

void CalculateBPMs(BmsLoadInfo *Info)
{
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(CHANNEL_BPM) != i->second.Events.end()) // there are bms events in here, get chopping
		{
			for (BMSEventList::iterator ev = i->second.Events[CHANNEL_BPM].begin(); ev != i->second.Events[CHANNEL_BPM].end(); ev++)
			{
				double BPM;
				BPM = fromBase16(tob36(ev->Event).c_str());

				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first);

				TimingSegment New;
				New.Time = Beat;
				New.Value = BPM;

				Info->difficulty->Timing.push_back(New);
			}
		}
	}

	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(CHANNEL_EXBPM) != i->second.Events.end())
		{
			for (BMSEventList::iterator ev = i->second.Events[CHANNEL_EXBPM].begin(); ev != i->second.Events[CHANNEL_EXBPM].end(); ev++)
			{
				double BPM;
				if (Info->BPMs.find(ev->Event) != Info->BPMs.end())
					BPM = Info->BPMs[ev->Event];
				else 
					continue;

				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first);

				TimingSegment New;
				New.Time = Beat;
				New.Value = BPM;

				Info->difficulty->Timing.push_back(New);
			}
		}
	}

	// Make sure ExBPM events are in front using stable_sort.
	std::stable_sort(Info->difficulty->Timing.begin(), Info->difficulty->Timing.end(), ts_sort);
}



void CalculateStops(BmsLoadInfo *Info)
{
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(CHANNEL_STOPS) != i->second.Events.end())
		{
			for (BMSEventList::iterator ev = i->second.Events[CHANNEL_STOPS].begin(); ev != i->second.Events[CHANNEL_STOPS].end(); ev++)
			{
				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first);
				double StopTimeBeats = Info->Stops[ev->Event] / 48;
				double SectionValueStop = SectionValue(Info->difficulty->Timing, Beat);
				double SPBSection = spb(SectionValueStop);
				double StopDuration = StopTimeBeats * SPBSection; // A value of 1 is... a 192nd of the measure? Or a 192nd of a beat? Or (192 / (4 * meter)) * spb? I'm not sure...

				TimingSegment New;
				New.Time = Beat;
				New.Value = StopDuration;

				Info->difficulty->Data->StopsTiming.push_back(New);
			}
		}
	}
}

int translateTrackBME(int Channel, int relativeTo)
{
	int relTrack = Channel - relativeTo;

	switch (relTrack)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		return relTrack + 1;
	case 5:
		return 0;
	case 6: // foot pedal is ignored.
		return MAX_CHANNELS + 1;
	case 7:
		return 6;
	case 8:
		return 7;
	default: // Undefined
		return relTrack + 1;
	}
}

int translateTrackPMS(int Channel, int relativeTo)
{
	int relTrack = Channel - relativeTo;

	if (relativeTo == startChannelP1 || relativeTo == startChannelLNP1)
	{
		switch (relTrack)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			return relTrack;
		case 5:
			return 7;
		case 6:
			return 8;
		case 7:
			return 5;
		case 8:
			return 6;
		default: // Undefined
			return relTrack;
		}
	}else
	{
		switch (relTrack)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			return relTrack;
		default: // Undefined
			return relTrack;
		}
	}
}

void measureCalculate(BmsLoadInfo *Info, MeasureList::iterator &i);

int evsort(const AutoplaySound &i, const AutoplaySound &j)
{
	return i.Time < j.Time;
}

int evtSort(const BMSEvent &A, const BMSEvent &B)
{
	return A.Fraction < B.Fraction;
}

void measureCalculateSide(BmsLoadInfo *Info, MeasureList::iterator &i, int TrackOffset, int startChannel, int startChannelLN, int startChannelMines, int startChannelInvisible, Measure &Msr)
{
	int usedChannels = Info->difficulty->Channels;

	// Standard events
	for (uint8 curChannel = startChannel; curChannel <= (startChannel+MAX_CHANNELS); curChannel++)
	{
		if (i->second.Events.find(curChannel) != i->second.Events.end()) // there are bms events for this channel.
		{
			int Track = 0;

			if (!Info->IsPMS)
				Track = translateTrackBME(curChannel, startChannel) - Info->LowerBound + TrackOffset;
			else
				Track = translateTrackPMS(curChannel, startChannel) + TrackOffset;

			if (!(Track >= 0 && Track < MAX_CHANNELS)) Utility::DebugBreak();

			if (Info->LNObj)
				std::sort(i->second.Events[curChannel].begin(), i->second.Events[curChannel].end(), evtSort);

			for (BMSEventList::iterator ev = i->second.Events[curChannel].begin(); ev != i->second.Events[curChannel].end(); ev++)
			{
				if (!ev->Event || Track >= usedChannels) continue; // UNUSABLE event

				double Beat = ev->Fraction * Msr.MeasureLength + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

				double Time = TimeAtBeat(Info->difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->difficulty->Data->StopsTiming, Beat);

				Info->difficulty->Duration = std::max((double)Info->difficulty->Duration, Time);

				if (!Info->LNObj || (Info->LNObj != ev->Event))
				{
degradetonote:
					NoteData Note;

					Note.StartTime = Time;
					Note.Sound = ev->Event;
					Info->UsedSounds[ev->Event] = true;
					
					Info->difficulty->TotalScoringObjects++;
					Info->difficulty->TotalNotes++;
					Info->difficulty->TotalObjects++;

					Msr.MeasureNotes[Track].push_back(Note);
					if (Msr.MeasureNotes[Track].size())
						Info->LastNotes[Track] = &Msr.MeasureNotes[Track].back();
				}else if (Info->LNObj && (Info->LNObj == ev->Event))
				{
					if (Info->LastNotes[Track]) 
					{
						Info->difficulty->TotalHolds++;
						Info->difficulty->TotalScoringObjects++;
						Info->LastNotes[Track]->EndTime = Time;
						Info->LastNotes[Track] = NULL;
					}
					else
						goto degradetonote;
				}
			}
		}
	}

	// LN events
	for (uint8 curChannel = startChannelLN; curChannel <= (startChannelLN+MAX_CHANNELS); curChannel++)
	{
		if (i->second.Events.find(curChannel) != i->second.Events.end()) // there are bms events for this channel.
		{
			int Track = 0;

			if (!Info->IsPMS)
				Track = translateTrackBME(curChannel, startChannelLN) - Info->LowerBound + TrackOffset;
			else
				Track = translateTrackPMS(curChannel, startChannelLN) + TrackOffset;

			for (BMSEventList::iterator ev = i->second.Events[curChannel].begin(); ev != i->second.Events[curChannel].end(); ev++)
			{
				if (Track >= usedChannels || Track < 0) continue;

				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

				double Time = TimeAtBeat(Info->difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->difficulty->Data->StopsTiming, Beat);

				if (Info->startTime[Track] == -1)
				{
					Info->startTime[Track] = Time;
				}else
				{
					NoteData Note;

					Info->difficulty->Duration = std::max((double)Info->difficulty->Duration, Time);

					Note.StartTime = Info->startTime[Track];
					Note.EndTime = Time;

					Note.Sound = ev->Event;
					Info->UsedSounds[ev->Event] = true;

					Info->difficulty->TotalScoringObjects += 2;
					Info->difficulty->TotalHolds++;
					Info->difficulty->TotalObjects++;

					Msr.MeasureNotes[Track].push_back(Note);

					Info->startTime[Track] = -1;
				}
			}
		}
	}
}

void measureCalculate(BmsLoadInfo *Info, MeasureList::iterator &i)
{
	Measure Msr;

	Msr.MeasureLength = 4 * i->second.BeatDuration;

	// see both sides, p1 and p2
	if (!Info->IsPMS) // or BME-type PMS
	{
		measureCalculateSide(Info, i, 0, startChannelP1, startChannelLNP1, startChannelMinesP1, startChannelInvisibleP1, Msr);
		measureCalculateSide(Info, i, Info->SideBOffset, startChannelP2, startChannelLNP2, startChannelMinesP2, startChannelInvisibleP2, Msr);
	}else
	{
		measureCalculateSide(Info, i, 0, startChannelP1, startChannelLNP1, startChannelMinesP1, startChannelInvisibleP1, Msr);
		measureCalculateSide(Info, i, 5, startChannelP2+1, startChannelLNP2+1, startChannelMinesP2+1, startChannelInvisibleP2+1, Msr);
	}

	// insert it into the difficulty structure
	Info->difficulty->Data->Measures.push_back(Msr);

	for (uint8 k = 0; k < MAX_CHANNELS; k++)
	{
		// Our old pointers are invalid by now since the Msr structures are going to go out of scope
		// Which means we must renew them, and that's better done here.

		MeasureVector::reverse_iterator q = Info->difficulty->Data->Measures.rbegin();
		
		while (q != Info->difficulty->Data->Measures.rend())
		{
			if ((*q).MeasureNotes[k].size()) {
				Info->LastNotes[k] = &((*q).MeasureNotes[k].back());
				goto next_chan;
			}
			q++;
		}
		next_chan:;
	}

	if (i->second.Events[CHANNEL_BGM].size()) // There are some BGM events?
	{
		for (BMSEventList::iterator ev = i->second.Events[CHANNEL_BGM].begin(); ev != i->second.Events[CHANNEL_BGM].end(); ev++)
		{
			int Event = ev->Event;

			if (!Event) continue; // UNUSABLE event

			double Beat = ev->Fraction * Msr.MeasureLength + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

			double Time = TimeAtBeat(Info->difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->difficulty->Data->StopsTiming, Beat);

			AutoplaySound New;
			New.Time = Time;
			New.Sound = Event;

			Info->UsedSounds[New.Sound] = true;

			Info->difficulty->Data->BGMEvents.push_back(New);
		}

		std::sort(Info->difficulty->Data->BGMEvents.begin(), Info->difficulty->Data->BGMEvents.end(), evsort);
	}
}

void AutodetectChannelCountSide(BmsLoadInfo *Info, int offset, int usedChannels[MAX_CHANNELS], int startChannel, int startChannelLN, int startChannelMines, int startChannelInvisible)
{
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		// normal channels
		for (int curChannel = startChannel; curChannel <= (startChannel + MAX_CHANNELS - offset); curChannel++)
		{
			if (i->second.Events.find(curChannel) != i->second.Events.end())
			{
				int offs;
				
				if (!Info->IsPMS)
					offs = translateTrackBME(curChannel, startChannel)+offset;
				else
					offs = translateTrackPMS(curChannel, startChannel)+offset;

				if (offs < MAX_CHANNELS) // A few BMSes use the foot pedal, so we need to not overflow the array.
					usedChannels[offs] = 1;
			}
		}

		// LN channels
		for (int curChannel = startChannelLN; curChannel <= (startChannelLN + MAX_CHANNELS - offset); curChannel++)
		{
			if (i->second.Events.find(curChannel) != i->second.Events.end())
			{
				int offs;
				
				if (!Info->IsPMS)
					offs = translateTrackBME(curChannel, startChannelLN)+offset;
				else
					offs = translateTrackPMS(curChannel, startChannelLN)+offset;

				if (offs < MAX_CHANNELS)
					usedChannels[offs] = 1;
			}
		}
	}
}

int AutodetectChannelCount(BmsLoadInfo *Info)
{
	int usedChannels[MAX_CHANNELS] = {};
	int usedChannelsB[MAX_CHANNELS] = {};

	if (Info->IsPMS)
		return 9;

	Info->LowerBound = -1;
	Info->UpperBound = 0;
	
	/* Autodetect channel count based off channel information */
	AutodetectChannelCountSide(Info, 0, usedChannels, startChannelP1, startChannelLNP1, startChannelMinesP1, startChannelInvisibleP1);

	/* Find the last channel we've used's index */
	int FirstIndex = -1;
	int LastIndex = 0;
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (usedChannels[i] != 0)
		{
			if (FirstIndex == -1) // Lowest channel being used. Used for translation back to track 0.
				FirstIndex = i;

			LastIndex = i;
		}
	}

	// Use that information to add the p2 side right next to the p1 side and have a continuous thing.
	AutodetectChannelCountSide(Info, 0, usedChannelsB, startChannelP2, startChannelLNP2, startChannelMinesP2, startChannelInvisibleP2);

	// Correct if second side starts at an offset different from zero.
	int sideBIndex = -1;

	for (int i = 0; i < MAX_CHANNELS; i++)
		if (usedChannelsB[i])
		{
			sideBIndex = i;
			break;
		}

	// We found where it starts; append that starting point to the end of the left side.

	if (sideBIndex >= 0)
	{
		for (int i = LastIndex+1; i < MAX_CHANNELS; i++)
		{
			int startSideB = (sideBIndex - Info->SideBOffset) + 1;

			usedChannels[i] |= usedChannelsB[i-LastIndex-1];
		}
	}

	if (FirstIndex >= 0 && sideBIndex >= 0)
		Info->UsesTwoSides = true; // This means, when working with the second side, add the offset to the current track.

	/* Find new boundaries for used channels. This means the first channel will be the Lower Bound. */
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (usedChannels[i] != 0)
		{
			if (Info->LowerBound == -1) // Lowest channel being used. Used for translation back to track 0.
				Info->LowerBound = i;

			Info->UpperBound = i;
		}
	}

	// We pick the range of channels we're going to use.
	int Range = Info->UpperBound - Info->LowerBound + 1;

	// This means, Side B offset starts from here.
	// If the last index was 7 for instance, and the first was 0, our side B offset would be 8, first channel of second side.
	// If the last index was 5 and the first was 0, side B offset would be 6.
	// While other cases would really not make much sense, they're theorically supported, anyway.
	Info->SideBOffset = LastIndex + 1;

	// We modify it for completey unused key modes to not appear..
	if (Range < 4) // 1, 2, 3
		Range = 6;

	if (Range > 9 && Range < 12) // 10, 11
		Range = 12;

	if (Range > 12 && Range < 16) // 13, 14, 15
		Range = 16;

	return Range;
}

void CompileBMS(BmsLoadInfo *Info)
{
	/* To be done. */
	MeasureList &m = Info->Measures;
	CalculateBPMs(Info);
	CalculateStops(Info);

	if (Info->HasBMPEvents)
	{
		BMPEventsDetail *BMP = new BMPEventsDetail;
		CalculateBMP(Info, BMP->BMPEventsLayerBase, CHANNEL_BGABASE);
		CalculateBMP(Info, BMP->BMPEventsLayerMiss, CHANNEL_BGAPOOR);
		CalculateBMP(Info, BMP->BMPEventsLayer, CHANNEL_BGALAYER);
		CalculateBMP(Info, BMP->BMPEventsLayer2, CHANNEL_BGALAYER2);
		Info->difficulty->Data->BMPEvents = BMP;
	}

	Info->difficulty->Channels = AutodetectChannelCount(Info);
	if (Info->difficulty->Channels == 9) // Assume pop'n
		Info->IsPMS = true;
	
	for (MeasureList::iterator i = m.begin(); i != m.end(); i++)
		measureCalculate(Info, i);
}

bool InterpStatement(GString Command, GString Contents, BmsLoadInfo *Info)
{
	bool IsControlFlowCommand = false;

	// Starting off with the basics.

	do {
		if (Command == "#SETRANDOM")
		{
			Info->RandomStack[Info->CurrentNestedLevel] = atoi(Contents.c_str());
		}else if (Command == "#RANDOM")
		{
			IsControlFlowCommand = true;

			if (Info->Skip)
				break;

			int Limit = atoi(Contents.c_str());

			assert(Info->CurrentNestedLevel < 16);
			assert(Limit > 1);

			Info->RandomStack[Info->CurrentNestedLevel] = rand() % Limit + 1;

		}else if (Command == "#IF")
		{
			IsControlFlowCommand = true;
			Info->CurrentNestedLevel++;

			if (Info->Skip)
				break;

			int Var = atoi(Contents.c_str());

			assert (Var > 0);

			if (Var != Info->RandomStack[Info->CurrentNestedLevel-1])
			{
				Info->Skip = 1;
				Info->SkipNestedLevel = Info->CurrentNestedLevel-1; // Once we reach this nested level, we end skipping.
			}

		}else if (Command == "#ENDIF")
		{
			IsControlFlowCommand = true;
			Info->CurrentNestedLevel--;

			if (Info->Skip)
			{
				if (Info->CurrentNestedLevel == Info->SkipNestedLevel)
					Info->Skip = 0;
			}
		}

	} while (0);

	return !IsControlFlowCommand && !Info->Skip;
}

bool ShouldUseU8(const char* Line)
{
	bool IsU8 = false;
	int i = strstr(Line, "\x0A") - Line;

	if (!utf8::starts_with_bom(Line, Line + 1024))
	{
		if (utf8::is_valid(Line, Line + 1024))
		{
			IsU8 = true;
			for (int i = 0; i < 1024; i++)
			{
				if (Line[i] && 0x80) // high bit is set
				{
					IsU8 = false;
					break;
				}
			}
		}
	}
	else
		IsU8 = true;
	return IsU8;
}

bool isany(char x, const char* p, int len)
{
	for (int i = 0; i < len; i++)
		if (x == p[i]) return true;

	return false;
}

// Returns: Out: a vector with all the subtitles, GString: The title without the subtitles.
GString GetSubtitles(GString SLine, std::vector<GString> &Out)
{
	const char* EntryChars = "({[";
	const char* ExitChars = ")}]";
	const char* Line = SLine.c_str();
	size_t len = SLine.length();

	GString CurrentParse;
	std::stack<char> Paren;

	for (size_t i = 0; i < len; i++)
	{
		if (isany(Line[i], EntryChars, 3))
			Paren.push(Line[i]);
		else if (isany(Line[i], ExitChars, 3))
		{
			if (Paren.size())
			{
				Paren.pop();
				Out.push_back(CurrentParse);
				CurrentParse = "";
			}
		}
		else
		{
			if (Paren.size())
				CurrentParse += Line[i];
		}
	}

	return SLine.substr(0, SLine.find_first_of(EntryChars));
}

GString DifficultyNameFromSubtitles(std::vector<GString> &Subs)
{
	for (std::vector<GString>::iterator i = Subs.begin();
		i != Subs.end();
		i++)
	{
		GString Current = *i;
		boost::to_lower(Current);
		const char* s = Current.c_str();

		if (strstr(s, "another"))
			return "Another";
		else if (strstr(s, "ex"))
			return "EX";
		else if (strstr(s, "hyper"))
			return "Hyper";
		else if (strstr(s, "hard"))
			return "Hyper";
		else if (strstr(s, "normal"))
			return "Normal";
		else if (strstr(s, "light"))
			return "Light";
		else if (strstr(s, "beginner"))
			return "Beginner";
	}

	if (Subs.size()) // Okay then, join them together.
	{
		GString sub = boost::join(Subs, " / ");
		return sub;
	}

	// Oh, we failed then..
	return "";
}

void NoteLoaderBMS::LoadObjectsFromFile(GString filename, GString prefix, Song *Out)
{
#if (!defined _WIN32)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein(Utility::Widen(filename).c_str());
#endif

	Difficulty *Diff = new Difficulty();
	BmsLoadInfo *Info = new BmsLoadInfo();
	DifficultyLoadInfo *LInfo = new DifficultyLoadInfo();

	Diff->Filename = filename;
	Diff->Data = LInfo;
	Info->song = Out;
	Info->difficulty = Diff;

	if (Utility::Widen(filename).find(L"pms") != std::wstring::npos)
		Info->IsPMS = true;

	// BMS uses beat-based locations for stops and BPM. (Though the beat must be calculated.)
	Diff->BPMType = VSRG::Difficulty::BT_Beat;

	if (!filein.is_open())
	{
		Log::Printf("NoteLoaderBMS: Couldn't open file %s!", filename.c_str());
		delete Diff;
		delete Info;
		return;
	}

	srand(time(0));
	Diff->IsVirtual = true;

	VSRG::BmsTimingInfo *TimingInfo = new VSRG::BmsTimingInfo;
	Diff->Data->TimingInfo = TimingInfo;

	/*
		BMS files are separated always one file, one difficulty, so it'd make sense
		that every BMS 'set' might have different timing information per chart.
		While BMS specifies no 'set' support it's usually implied using folders.

		The default BME specifies is 8 channels when #PLAYER is unset, however
		the modern BMS standard specifies to ignore #PLAYER and try to figure it out
		from the amount of used channels.

		And that's what we're going to try to do.
	*/

	std::vector<GString> Subs; // Subtitle list
	GString Line;
	bool IsU8 = false;
	char* TestU8 = new char[1025];

	int cnt = filein.readsome(TestU8, 1024);
	TestU8[cnt] = 0;

	// Sonorous UTF-8 extension
	IsU8 = ShouldUseU8(TestU8);
	delete TestU8;
	filein.seekg(0);

	while (filein)
	{
		std::getline(filein, Line);

		boost::replace_all(Line, "\r", "");

		if ( Line.length() == 0 || Line[0] != '#' )
			continue;

		GString command = Line.substr(Line.find_first_of("#"), Line.find_first_of(" ") - Line.find_first_of("#"));

		boost::to_upper(command);
		boost::replace_all(command, "\n", "");

#define OnCommand(x) if(command == #x)
#define OnCommandSub(x) if(command.substr(0, strlen(#x)) == #x)

		GString CommandContents = Line.substr(Line.find_first_of(" ") + 1);
		if (!IsU8)
			CommandContents = Utility::SJIStoU8(CommandContents);

		if (InterpStatement(command, CommandContents, Info)) 
		{

			OnCommand(#GENRE)
			{
				// stub
			}

			OnCommand(#SUBTITLE)
			{
				Subs.push_back(CommandContents);
			}

			OnCommand(#TITLE)
			{
				Out->SongName = CommandContents;
				// ltrim the GString
				size_t np = Out->SongName.find_first_not_of(" ");
				if (np != GString::npos)
					Out->SongName = Out->SongName.substr(np);
			}

			OnCommand(#ARTIST)
			{
				Out->SongAuthor = CommandContents;

				size_t np = Out->SongAuthor.find_first_not_of(" ");

				if (np != GString::npos)
					Out->SongAuthor = Out->SongAuthor.substr(np);
			}

			OnCommand(#BPM)
			{
				TimingSegment Seg;
				Seg.Time = 0;
				Seg.Value = latof(CommandContents.c_str());
				Diff->Timing.push_back(Seg);

				continue;
			}

			OnCommand(#STAGEFILE)
			{
				Diff->Data->StageFile = CommandContents;
			}

			OnCommand(#LNOBJ)
			{
				Info->LNObj = fromBase36(CommandContents.c_str());
			}

			OnCommand(#DIFFICULTY)
			{
				GString dName;
				if (Utility::IsNumeric(CommandContents.c_str()))
				{
					int Kind = atoi(CommandContents.c_str());

					switch (Kind)
					{
					case 1:
						dName = "Beginner";
						break;
					case 2:
						dName = "Normal";
						break;
					case 3:
						dName = "Hard";
						break;
					case 4:
						dName = "Another";
						break;
					case 5:
						dName = "Another+";
						break;
					default:
						dName = "???";
					}
				}
				else
				{
					dName = CommandContents;
				}

				Diff->Name = dName;
			}

			OnCommand(#BACKBMP)
			{
				Diff->Data->StageFile = CommandContents;
			}

			OnCommand(#PREVIEW)
			{
				Out->SongPreviewSource = CommandContents;
			}

			OnCommand(#TOTAL)
			{
				TimingInfo->life_total = latof(CommandContents);
			}

			OnCommand(#PLAYLEVEL)
			{
				Diff->Level = atoi(CommandContents.c_str());
			}

			OnCommand(#RANK)
			{
				TimingInfo->judge_rank = latof(CommandContents);
			}

			OnCommandSub(#WAV)
			{
				GString IndexStr = CommandSubcontents("#WAV", command);
				int Index = fromBase36(IndexStr.c_str());
				Info->Sounds[Index] = CommandContents;
			}

			OnCommandSub(#BMP)
			{
				GString IndexStr = CommandSubcontents("#BMP", command);
				int Index = fromBase36(IndexStr.c_str());
				Info->BMP[Index] = CommandContents.c_str();
			}

			OnCommandSub(#BPM)
			{
				GString IndexStr = CommandSubcontents("#BPM", command);
				int Index = fromBase36(IndexStr.c_str());
				Info->BPMs[Index] = latof(CommandContents.c_str());
			}

			OnCommandSub(#STOP)
			{
				GString IndexStr = CommandSubcontents("#STOP", command);
				int Index = fromBase36(IndexStr.c_str());
				Info->Stops[Index] = latof(CommandContents.c_str());
			}

			/* Do we need this?... */
			OnCommandSub(#EXBPM)
			{
				GString IndexStr = CommandSubcontents("#EXBPM", command);
				int Index = fromBase36(IndexStr.c_str());
				Info->BPMs[Index] = latof(CommandContents.c_str());
			}

			/* Else... */
			GString MeasureCommand = Line.substr(Line.find_first_of(":")+1);
			GString MainCommand = Line.substr(1, 5);

			if (Utility::IsNumeric(MainCommand.c_str())) // We've got work to do.
			{
				int Measure = atoi(MainCommand.substr(0,3).c_str());
				int Channel = fromBase36(MainCommand.substr(3,2).c_str());

				ParseEvents(Info, Measure, Channel, MeasureCommand);
			}

		}
	}

	/* When all's said and done, "compile" the bms. */
	CompileBMS(Info);

	/* Copy only used sounds to the sound list */
	for (auto i = Info->Sounds.begin(); i != Info->Sounds.end(); i++)
	{
		if (Info->UsedSounds.find(i->first) != Info->UsedSounds.end() && Info->UsedSounds[i->first]) // This sound is used.
		{
			Diff->SoundList[i->first] = i->second;
		}
	}

	if (Info->HasBMPEvents)
	{
		Diff->Data->BMPEvents->BMPList = Info->BMP;
	}

	// First try to find a suiting subtitle
	GString NewTitle = GetSubtitles(Out->SongName, Subs);
	if (Diff->Name.length() == 0)
	{
		Diff->Name = DifficultyNameFromSubtitles(Subs);
	}

	// If we've got a title that's usuable then why not use it.
	if (NewTitle.length() > 0)
	{
		Out->SongName = NewTitle.substr(0, NewTitle.find_last_not_of(" ")+1);
	}

	// Okay, we didn't find a fitting subtitle, let's try something else.
	// Get actual filename instead of full path.
	filename = Utility::RelativeToPath(filename);
	if (Diff->Name.length() == 0)
	{
		size_t startBracket = filename.find_first_of("[");
		size_t endBracket = filename.find_last_of("]");

		if (startBracket != GString::npos && endBracket != GString::npos)
			Diff->Name = filename.substr(startBracket + 1, endBracket - startBracket - 1);

		// No brackets? Okay then, let's use the filename.
		if (Diff->Name.length() == 0)
		{
			size_t last_slash = filename.find_last_of("/");
			size_t last_dslash = filename.find_last_of("\\");
			size_t last_dir = std::max( last_slash != GString::npos ? last_slash : 0, last_dslash != GString::npos ? last_dslash : 0 );
			Diff->Name = filename.substr(last_dir + 1, filename.length() - last_dir - 5);
		}
	}

	Out->Difficulties.push_back(Diff);
	delete Info;
}

