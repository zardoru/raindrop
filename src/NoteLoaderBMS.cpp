#include <fstream>

#include "Global.h"
#include "NoteLoader7K.h"
#include "NoteLoader.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>


/*
	Source for implemented commands:
	http://hitkey.nekokan.dyndns.info/cmds.htm

	A huge lot of not-frequently-used commands are not going to be implemented.

	On 5K BMS, scratch is usually channel 16/26 for P1/P2
	17/27 for foot pedal- no exceptions
	keys 6 and 7 are 21/22 in bms
	keys 6 and 7 are 18/19, 28/29 in bme

	two additional extensions to BMS are planned for dotcur to introduce compatibility
	with SV changes:
	#SCROLLxx <value>
	#SPEEDxx <value> <duration>

	and are to be put under channel DC (base 36)

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

// From base 36 they're 01, 02, 03 and 09.
#define CHANNEL_BGM 1
#define CHANNEL_METER 2
#define CHANNEL_BPM 3
#define CHANNEL_STOPS 9

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


typedef std::map<int, String> FilenameListIndex;
typedef std::map<int, double> BpmListIndex;
typedef std::vector<TrackNote> NoteVector;
typedef std::map<int, BMSMeasure> MeasureList;

int fromBase36(const char *txt)
{
	return strtoul(txt, NULL, 36);
}

const int startChannel = fromBase36("11");
const int endChannel   = fromBase36("1Z");

/*
template <char* s> int b36()
{
	return fromBase36(s);
}
*/

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
	Song7K* Song;
	SongInternal::Difficulty7K *Difficulty;
};

String CommandSubcontents (const String Command, const String Line)
{
	uint32 len = Command.length();
	return Line.substr(len);
}

void ParseEvents(BmsLoadInfo *Info, const int Measure, const int BmsChannel, const String Command)
{
	int CommandLength = Command.length() / 2;

	if ( BmsChannel != CHANNEL_BGM && Info->Measures[Measure].Events.find(BmsChannel) != Info->Measures[Measure].Events.end())
	{
		// Can we skip it if it already exists?
		// Or should we overwrite it?

		return; // Skip.
	}

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

void CalculateBPMs(BmsLoadInfo *Info)
{
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(CHANNEL_BPM) != i->second.Events.end()) // there are bms events in here, get chopping
		{
			for (BMSEventList::iterator ev = i->second.Events[CHANNEL_BPM].begin(); ev != i->second.Events[CHANNEL_BPM].end(); ev++)
			{
				if (Info->BPMs.find(ev->Event) == Info->BPMs.end()) continue; // No BPM associated to this event

				double BPM = Info->BPMs[ev->Event];
				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

				SongInternal::TimingSegment New;
				New.Time = Beat;
				New.Value = BPM;

				Info->Difficulty->Timing.push_back(New);
			}
		}
	}
}

void CalculateStops(BmsLoadInfo *Info)
{/*
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		if (i->second.Events.find(CHANNEL_STOPS) != i->second.Events.end())
		{
			for (BMSEventList::iterator ev = i->second.Events[CHANNEL_STOPS].begin(); ev != i->second.Events[CHANNEL_STOPS].end(); ev++)
			{
				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first);
				double StopDuration = Info->Stops[ev->Event]; // A value of 1 is... a 192nd of the measure? Or a 192nd of a beat? Or (192 / (4 * meter)) * spb? I'm not sure...
			}
		}
	}
	*/
}

int translateTrackBME(int Channel)
{
	int relTrack = Channel - fromBase36("11");

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
	case 6: // foot pedal is ignored
		return 10;
	case 7:
		return 6;
	case 8:
		return 7;
	default: // Undefined
		return relTrack + 1;
	}
}

int translateTrackBMS(int Channel)
{
	int relTrack = Channel - fromBase36("11");

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
	default: // Undefined
		return relTrack + 1;
	}
}

int translateTrackDDR(int Channel)
{
	return Channel - fromBase36("11");
}

int translateTracko2Mania(int Channel)
{
	return Channel == fromBase36("16") ? 0 : (Channel - fromBase36("11") + 1);
}

int evsort(const SongInternal::AutoplaySound &i, const SongInternal::AutoplaySound &j)
{
	return i.Time < j.Time;
}

void measureCalculate(BmsLoadInfo *Info, MeasureList::iterator &i)
{
	int usedChannels = Info->Difficulty->Channels;
	SongInternal::Measure7K Msr[MAX_CHANNELS];

	for (int curChannel = startChannel; curChannel <= endChannel; curChannel++)
	{
		if (i->second.Events.find(curChannel) != i->second.Events.end()) // there are bms events for this channel.
		{
			int Track = 0;
			switch (usedChannels)
			{
			case 8: // Classic BME
			case 9:
				Track = translateTrackBME(curChannel);
				break;
			case 6: // Classic BMS
				Track = translateTrackBMS(curChannel);
				break;
			default: // o2mania
				Track = translateTracko2Mania(curChannel);
			}

			for (BMSEventList::iterator ev = i->second.Events[curChannel].begin(); ev != i->second.Events[curChannel].end(); ev++)
			{
				int Event = ev->Event;

				if (!Event || Track >= usedChannels) continue; // UNUSABLE event

				double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

				double Time = TimeAtBeat(Info->Difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->Difficulty->StopsTiming, Beat);
				TrackNote Note;

				Info->Difficulty->Duration = std::max((double)Info->Difficulty->Duration, Time);

				Note.AssignTime(Time);
				Note.AssignSound(Event);

				Note.AssignTrack(Track);
				Info->Difficulty->TotalScoringObjects++;
				Info->Difficulty->TotalNotes++;
				Info->Difficulty->TotalObjects++;

				Msr[Note.GetTrack()].MeasureNotes.push_back(Note);
			}
		}
	}

	for (int k = 0; k < usedChannels; k++)
	{
		if (Msr[k].MeasureNotes.size())
			Info->Difficulty->Measures[k].push_back(Msr[k]);
	}

	if (i->second.Events[CHANNEL_BGM].size()) // There are some BGM events?
	{
		for (BMSEventList::iterator ev = i->second.Events[CHANNEL_BGM].begin(); ev != i->second.Events[CHANNEL_BGM].end(); ev++)
		{
			int Event = ev->Event;

			if (!Event) continue; // UNUSABLE event

			double Beat = ev->Fraction * 4 * i->second.BeatDuration + BeatForMeasure(Info, i->first); // 4 = measure length in beats. todo: calculate appropietly!

			double Time = TimeAtBeat(Info->Difficulty->Timing, 0, Beat) + StopTimeAtBeat(Info->Difficulty->StopsTiming, Beat);

			SongInternal::AutoplaySound New;
			New.Time = Time;
			New.Sound = Event;

			// printf("Event %i, %f, %f..\n", Event, Beat, Time);

			Info->Difficulty->BGMEvents.push_back(New);
		}

		std::sort(Info->Difficulty->BGMEvents.begin(), Info->Difficulty->BGMEvents.end(), evsort);
	}
}

void CalculateObjects(BmsLoadInfo *Info)
{
	int usedChannels = 0;
	
	/* Autodetect channel count */
	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		for (int curChannel = startChannel; curChannel <= endChannel; curChannel++)
		{
			if (i->second.Events.find(curChannel) != i->second.Events.end())
				usedChannels = std::max(startChannel, curChannel) - startChannel + 1;
		}
	}

	if (usedChannels == 9) // Move PMS to BME unless PMS flag is set..
		usedChannels = 8; 

	Info->Difficulty->Channels = usedChannels;


	for (MeasureList::iterator i = Info->Measures.begin(); i != Info->Measures.end(); i++)
	{
		measureCalculate(Info, i);
	}
}

void CompileBMS(BmsLoadInfo *Info)
{
	/* To be done. */
	CalculateBPMs(Info);
	CalculateStops(Info);
	CalculateObjects(Info);
}

void NoteLoaderBMS::LoadObjectsFromFile(String filename, String prefix, Song7K *Out)
{
	std::ifstream filein (filename.c_str());
	SongInternal::Difficulty7K *Difficulty = new SongInternal::Difficulty7K();
	BmsLoadInfo *Info = new BmsLoadInfo();

	Info->Song = Out;
	Info->Difficulty = Difficulty;

	// BMS uses beat-based locations for stops and BPM. (Though the beat must be calculated.)
	Out->BPMType = Song7K::BT_Beat;

	if (!filein.is_open())
	{
		delete Difficulty;
		delete Info;
		return;
	}

	Difficulty->TotalNotes = 0;
	Difficulty->TotalHolds = 0;
	Difficulty->TotalObjects = 0;
	Difficulty->TotalScoringObjects = 0;
	Difficulty->IsVirtual = true;
	Difficulty->Offset = 0;
	Difficulty->Duration = 0;

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


	Difficulty->Channels = 8; 

	String Line;
	while (filein)
	{
		std::getline(filein, Line);

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
			SongInternal::TimingSegment Seg;
			Seg.Time = 0;
			Seg.Value = atof(CommandContents.c_str());
			Difficulty->Timing.push_back(Seg);

			continue;
		}

		OnCommand(#STAGEFILE)
		{
			Out->BackgroundRelativeDir = CommandContents;
			Out->BackgroundDir = prefix + "/" + CommandContents;
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

			Difficulty->Name = dName;
		}

		OnCommand(#BACKBMP)
		{
			Out->BackgroundRelativeDir = CommandContents;
			Out->BackgroundDir = prefix + "/" + CommandContents;
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
	Difficulty->SoundList = Info->Sounds;

	Out->Difficulties.push_back(Difficulty);
	delete Info;
}
