#include <fstream>
#include <map>

#include "Global.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

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
std::string tob36(long unsigned int value)
{
	const char base36[37] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"; // off by 1 lol
	char buffer[14];
	unsigned int offset = sizeof(buffer);
 
	buffer[--offset] = '\0';
	do {
		buffer[--offset] = base36[value % 36];
	} while (value /= 36);
 
	return std::string(&buffer[offset]);
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
#define CHANNEL_EXBPM 8
#define CHANNEL_STOPS 9
#define CHANNEL_SCRATCH chanScratch

struct BMSEvent
{
	int Event;
	double Fraction;
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

typedef std::map<int, String> FilenameListIndex;
typedef std::map<int, double> BpmListIndex;
typedef std::vector<NoteData> NoteVector;
typedef std::map<int, BMSMeasure> MeasureList;

const int startChannel = fromBase36("11");
const int endChannel   = fromBase36("1Z");
const int startChannelLN = fromBase36("51");

// The first wav will always be WAV01, not WAV00 since 00 is a reserved value for "nothing"
// Pretty fitting, in my opinion.



struct BmsLoadInfo
{
	FilenameListIndex Sounds;
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
	Song* Song;
	Difficulty *Difficulty;

	int LowerBound, UpperBound;
	
	float startTime[MAX_CHANNELS];

	NoteData *LastNotes[MAX_CHANNELS];

	int LNObj;

	BmsLoadInfo()
	{
		for (int k = 0; k < MAX_CHANNELS; k++)
		{
			startTime[k] = -1;
			LastNotes[k] = NULL;
		}
		Difficulty = NULL;
		Song = NULL;
		LNObj = 0;
	}
};

String CommandSubcontents (const String Command, const String Line)
{
	uint32 len = Command.length();
	return Line.substr(len);
}

void ParseEvents(BmsLoadInfo *Info, const int Measure, const int BmsChannel, const String Command)
{
	int CommandLength = Command.length() / 2;

	if (BmsChannel != CHANNEL_METER)
	{

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
		double Event = atof(Command.c_str());

		Info->Measures[Measure].BeatDuration = Event;
	}
}	

double BeatForMeasure(BmsLoadInfo *Info, const int Measure)
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

void CalculateBPMs(BmsLoadInfo *Info, int Chan = CHANNEL_BPM);

void CalculateBPMs(BmsLoadInfo *Info, int Chan)
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

				Info->Difficulty->Timing.push_back(New);
			}
		}
	}

	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(CHANNEL_EXBPM) != i->second.Events.end()) // there are bms events in here, get chopping
		{
			for (BMSEventList::iterator ev = i->second.Events[CHANNEL_EXBPM].begin(); ev != i->second.Events[CHANNEL_EXBPM].end(); ev++)
			{
				double BPM;
				if (Info->BPMs.find(ev->Event) != Info->BPMs.end())
					BPM = Info->BPMs[ev->Event];
				else continue;

				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first);

				TimingSegment New;
				New.Time = Beat;
				New.Value = BPM;

				Info->Difficulty->Timing.push_back(New);
			}
		}
	}

	std::sort(Info->Difficulty->Timing.begin(), Info->Difficulty->Timing.end(), ts_sort);
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
				double StopDuration = Info->Stops[ev->Event] / 48 * spb (SectionValue(Info->Difficulty->Timing, Beat)); // A value of 1 is... a 192nd of the measure? Or a 192nd of a beat? Or (192 / (4 * meter)) * spb? I'm not sure...

				TimingSegment New;
				New.Time = Beat;
				New.Value = StopDuration;

				Info->Difficulty->StopsTiming.push_back(New);
			}
		}
	}
	
}

int translateTrackBME(int Channel, int relativeTo = startChannel);

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

void measureCalculate(BmsLoadInfo *Info, MeasureList::iterator &i);

int evsort(const AutoplaySound &i, const AutoplaySound &j)
{
	return i.Time < j.Time;
}

void CalculateObjects(BmsLoadInfo *Info)
{
	int usedChannels[MAX_CHANNELS];

	for (uint8 i = 0; i < MAX_CHANNELS; i++)
		usedChannels[i] = 0;

	Info->LowerBound = -1;
	Info->UpperBound = 0;
	
	/* Autodetect channel count */
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		for (uint8 curChannel = startChannel; curChannel <= (startChannel + MAX_CHANNELS); curChannel++)
		{
			if (i->second.Events.find(curChannel) != i->second.Events.end())
				usedChannels[translateTrackBME(curChannel)] = 1;
		}
	}

	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		for (uint8 curChannel = startChannelLN; curChannel <= (startChannelLN + MAX_CHANNELS); curChannel++)
		{
			if (i->second.Events.find(curChannel) != i->second.Events.end())
				usedChannels[translateTrackBME(curChannel, startChannelLN)] = 1;
		}
	}

	int usedChannelCNT = 0;
	for (int i = 0; i < MAX_CHANNELS; i++)
		if (usedChannels[i] != 0)
		{
			if (Info->LowerBound == -1) // Lowest channel being used. Used for translation back to track 0.
				Info->LowerBound = i;

			Info->UpperBound = i;
		}

	// We pick the range of channels we're going to use.
	Info->Difficulty->Channels = Info->UpperBound - Info->LowerBound + 1;

	/* At this point we know the channel count from 1 to MAX_CHANNELS. We should be able to use it now..*/

	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		measureCalculate(Info, i);
	}
}

int evtSort(const BMSEvent &A, const BMSEvent &B)
{
	return A.Fraction < B.Fraction;
}

void measureCalculate(BmsLoadInfo *Info, MeasureList::iterator &i)
{
	int usedChannels = Info->Difficulty->Channels;
	Measure Msr;

	Msr.MeasureLength = 4 * i->second.BeatDuration;

	for (uint8 curChannel = startChannel; curChannel <= (startChannel+MAX_CHANNELS); curChannel++)
	{
		if (i->second.Events.find(curChannel) != i->second.Events.end()) // there are bms events for this channel.
		{
			int Track = 0;
			Track = translateTrackBME(curChannel) - Info->LowerBound;

			if (Info->LNObj)
				std::sort(i->second.Events[curChannel].begin(), i->second.Events[curChannel].end(), evtSort);

			for (BMSEventList::iterator ev = i->second.Events[curChannel].begin(); ev != i->second.Events[curChannel].end(); ev++)
			{
				if (!ev->Event || Track >= usedChannels) continue; // UNUSABLE event

				double Beat = ev->Fraction * Msr.MeasureLength + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

				double Time = TimeAtBeat(Info->Difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->Difficulty->StopsTiming, Beat);

				Info->Difficulty->Duration = std::max((double)Info->Difficulty->Duration, Time);

				if (!Info->LNObj || (Info->LNObj != ev->Event))
				{
degradetonote:
					NoteData Note;

					Note.StartTime = Time;
					Note.Sound = ev->Event;
					
					Info->Difficulty->TotalScoringObjects++;
					Info->Difficulty->TotalNotes++;
					Info->Difficulty->TotalObjects++;

					Msr.MeasureNotes[Track].push_back(Note);
					if (Msr.MeasureNotes[Track].size())
						Info->LastNotes[Track] = &Msr.MeasureNotes[Track].back();
				}else if (Info->LNObj && (Info->LNObj == ev->Event))
				{
					if (Info->LastNotes[Track]) 
					{
						Info->Difficulty->TotalHolds++;
						Info->Difficulty->TotalScoringObjects++;
						Info->LastNotes[Track]->EndTime = Time;
						Info->LastNotes[Track] = NULL;
					}
					else
						goto degradetonote;
				}
			}
		}
	}

	for (uint8 curChannel = startChannelLN; curChannel <= (startChannelLN+MAX_CHANNELS); curChannel++)
	{
		if (i->second.Events.find(curChannel) != i->second.Events.end()) // there are bms events for this channel.
		{
			int Track = 0;

			Track = translateTrackBME(curChannel, startChannelLN) - Info->LowerBound;

			for (BMSEventList::iterator ev = i->second.Events[curChannel].begin(); ev != i->second.Events[curChannel].end(); ev++)
			{
				if (Track >= usedChannels || Track < 0) continue;

				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

				double Time = TimeAtBeat(Info->Difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->Difficulty->StopsTiming, Beat);

				if (Info->startTime[Track] == -1)
				{
					Info->startTime[Track] = Time;
				}else
				{
					NoteData Note;

					Info->Difficulty->Duration = std::max((double)Info->Difficulty->Duration, Time);

					Note.StartTime = Info->startTime[Track];
					Note.EndTime = Time;

					Note.Sound = ev->Event;

					Info->Difficulty->TotalScoringObjects += 2;
					Info->Difficulty->TotalHolds++;
					Info->Difficulty->TotalObjects++;

					Msr.MeasureNotes[Track].push_back(Note);

					Info->startTime[Track] = -1;
				}
			}
		}
	}

	Info->Difficulty->Measures.push_back(Msr);

	for (uint8 k = 0; k < MAX_CHANNELS; k++)
	{
		// Our old pointers are invalid by now since the Msr structures are going to go out of scope
		// Which means we must renew them, and that's better done here.

		MeasureVector::reverse_iterator q = Info->Difficulty->Measures.rbegin();
		
		while (q != Info->Difficulty->Measures.rend())
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

			double Time = TimeAtBeat(Info->Difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->Difficulty->StopsTiming, Beat);

			AutoplaySound New;
			New.Time = Time;
			New.Sound = Event;

			// printf("Event %i, %f, %f..\n", Event, Beat, Time);

			Info->Difficulty->BGMEvents.push_back(New);
		}

		std::sort(Info->Difficulty->BGMEvents.begin(), Info->Difficulty->BGMEvents.end(), evsort);
	}
}


void CompileBMS(BmsLoadInfo *Info)
{
	/* To be done. */
	CalculateBPMs(Info);
	CalculateStops(Info);
	CalculateObjects(Info);
}

void NoteLoaderBMS::LoadObjectsFromFile(String filename, String prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif
	Difficulty *Diff = new Difficulty();
	BmsLoadInfo *Info = new BmsLoadInfo();

	Info->Song = Out;
	Info->Difficulty = Diff;

	// BMS uses beat-based locations for stops and BPM. (Though the beat must be calculated.)
	Out->BPMType = Song::BT_Beat;
	Diff->LMT = Utility::GetLMT(filename);

	if (!filein.is_open())
	{
		delete Diff;
		delete Info;
		return;
	}

	Diff->IsVirtual = true;

	Out->SongDirectory = prefix;

	/* 
		BMS files are separated always one file, one difficulty, so it'd make sense
		that every BMS 'set' might have different timing information per chart.
		While BMS specifies no 'set' support it's usually implied using folders.
	*/
	Out->UseSeparateTimingData = true;

	/* 
		The default BME specifies is 8 channels when #PLAYER is unset, however
		the modern BMS standard specifies to ignore #PLAYER and try to figure it out
		from the amount of used channels.

		And that's what we're going to try to do.
	*/

	String Line;
	while (filein)
	{
		std::getline(filein, Line);

		boost::replace_all(Line, "\r", "");

		if ( Line.length() == 0 || Line[0] != '#' )
			continue;

		String command = Line.substr(Line.find_first_of("#"), Line.find_first_of(" ") - Line.find_first_of("#"));

		boost::replace_all(command, "\n", "");

#define OnCommand(x) if(command == #x)
#define OnCommandSub(x) if(command.substr(0, strlen(#x)) == #x)

		String CommandContents = Line.substr(Line.find_first_of(" ") + 1);

		OnCommand(#GENRE)
		{
			// stub
		}

		OnCommand(#TITLE)
		{
			Out->SongName = CommandContents;
		}
		
		OnCommand(#ARTIST)
		{
			Out->SongAuthor = CommandContents;
		}

		OnCommand(#BPM)
		{
			TimingSegment Seg;
			Seg.Time = 0;
			Seg.Value = atof(CommandContents.c_str());
			Diff->Timing.push_back(Seg);

			continue;
		}

		OnCommand(#STAGEFILE)
		{
			Out->BackgroundFilename = CommandContents;
		}

		OnCommand(#LNOBJ)
		{
			Info->LNObj = fromBase36(CommandContents.c_str());
		}

		OnCommand(#DIFFICULTY)
		{
			int Kind = atoi(CommandContents.c_str());
			String dName;

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

			Diff->Name = dName;
		}

		OnCommand(#BACKBMP)
		{
			Out->BackgroundFilename = CommandContents;
		}

		OnCommandSub(#WAV)
		{
			String IndexStr = CommandSubcontents("#WAV", command);
			int Index = fromBase36(IndexStr.c_str());
			Info->Sounds[Index] = CommandContents;
		}

		OnCommandSub(#BPM)
		{
			String IndexStr = CommandSubcontents("#BPM", command);
			int Index = fromBase36(IndexStr.c_str());
			Info->BPMs[Index] = atof(CommandContents.c_str());
		}

		OnCommandSub(#STOP)
		{
			String IndexStr = CommandSubcontents("#STOP", command);
			int Index = fromBase36(IndexStr.c_str());
			Info->Stops[Index] = atof(CommandContents.c_str());
		}

		/* Do we need this?... */
		OnCommandSub(#EXBPM)
		{
			String IndexStr = CommandSubcontents("#EXBPM", command);
			int Index = fromBase36(IndexStr.c_str());
			Info->BPMs[Index] = atof(CommandContents.c_str());
		}

		/* Else... */
		String MeasureCommand = Line.substr(Line.find_first_of(":")+1);
		String MainCommand = Line.substr(1, 5);

		if (Utility::IsNumeric(MainCommand.c_str())) // We've got work to do.
		{
			int Measure = atoi(MainCommand.substr(0,3).c_str());
			int Channel = fromBase36(MainCommand.substr(3,2).c_str());

			ParseEvents(Info, Measure, Channel, MeasureCommand);
		}

	}

	/* When all's said and done, "compile" the bms. */
	CompileBMS(Info);
	Diff->SoundList = Info->Sounds;

	if (Diff->Name.length() == 0)
	{
		size_t startBracket = filename.find_first_of("[");
		size_t endBracket = filename.find_last_of("]");

		if (startBracket != std::string::npos && endBracket != std::string::npos)
			Diff->Name = filename.substr(startBracket + 1, endBracket - startBracket - 1);

		// No brackets? Okay then, let's use the filename.
		if (Diff->Name.length() == 0)
		{
			size_t last_slash = filename.find_last_of("/");
			size_t last_dslash = filename.find_last_of("\\");
			size_t last_dir = std::max( last_slash != std::string::npos ? last_slash : 0, last_dslash != std::string::npos ? last_dslash : 0 );
			Diff->Name = filename.substr(last_dir + 1, filename.length() - last_dir - 5);
		}
	}

	Out->Difficulties.push_back(Diff);
	delete Info;
}
