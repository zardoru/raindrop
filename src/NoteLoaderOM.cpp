#include <fstream>
#include <stdlib.h>

#include "Global.h"
#include "NoteLoader7K.h"
#include "NoteLoader.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>

typedef SongInternal::Difficulty7K *SongDiff;
typedef std::vector<String> SplitResult;

struct OsuLoadInfo
{
	double SliderVelocity;
	int Version;
	int last_sound_index;
	Song7K *Song;
	std::map <String, int> Sounds;
	SongDiff Difficulty;
};

/* osu!mania loader. credits to wanwan159, woc2006, Zorori and the author of AIBat for helping me understand this. */

bool ReadGeneral (String line, OsuLoadInfo* Info)
{
	String Command = line.substr(0, line.find_first_of(" ")); // Lines are Information:<space>Content
	String Content = line.substr(line.find_first_of(" ") + 1, line.length() - line.find_first_of(" "));

	if (Command == "AudioFilename:")
	{
		if (Content == "virtual")
		{
			Info->Difficulty->IsVirtual = true;
			return true;
		}
		else
		{
#ifdef VERBOSE_DEBUG
			printf("Audio filename found: %s\n", Content.c_str());
#endif
			Info->Song->SongFilename = Info->Song->SongDirectory + "/" + Content;
			Info->Song->SongRelativePath = Content;
		}
	}else if (Command == "Mode:")
	{
		if (Content != "3") // It's not a osu!mania chart, so we can't use it.
			return false; // (What if we wanted to support taiko though?)
	}

	return true;
}

void ReadMetadata (String line, OsuLoadInfo* Info)
{
	String Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
	String Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

#ifdef VERBOSE_DEBUG
	printf("Command found: %s | Contents: %s\n", Command.c_str(), Content.c_str());
#endif

	if (Command == "Title")
	{
		Info->Song->SongName = Content;
	}else if (Command == "Artist")
	{
		Info->Song->SongAuthor = Content;
	}else if (Command == "Version")
	{
		Info->Difficulty->Name = Content;
	}
}

void ReadDifficulty (String line, OsuLoadInfo* Info)
{
	String Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
	String Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

	// We ignore everything but the key count!
	if (Command == "CircleSize")
	{
		Info->Difficulty->Channels = atoi(Content.c_str());

		for (int i = 0; i < Info->Difficulty->Channels; i++) // Push a single measure
			Info->Difficulty->Measures[i].push_back(SongInternal::Measure7K());

	}else if (Command == "SliderMultiplier")
	{
		Info->SliderVelocity = atof(Content.c_str()) * 100;
	}
}

void ReadEvents (String line, OsuLoadInfo* Info)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));

	if (Spl.size() > 1)
	{
		if (Spl[0] == "0" && Spl[1] == "0")
		{
			boost::replace_all(Spl[2], "\"", "");
			Info->Song->BackgroundRelativeDir = Spl[2];
			Info->Song->BackgroundDir = Info->Song->SongDirectory + "/" + Spl[2];
		}else if (Spl[0] == "5")
		{
			boost::replace_all(Spl[3], "\"", "");

			if (Info->Sounds.find(Spl[3]) == Info->Sounds.end())
			{
				Info->Sounds[Spl[3]] = Info->last_sound_index;
				Info->last_sound_index++;
			}

			double Time = atof(Spl[1].c_str()) / 1000.0;
			int Evt = Info->Sounds[Spl[3]];
			SongInternal::AutoplaySound New;
			New.Time = Time;
			New.Sound = Evt;

			Info->Difficulty->BGMEvents.push_back(New);
		}
	}
}

void ReadTiming (String line, OsuLoadInfo* Info)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));


	SongInternal::TimingSegment Time;
	Time.Time = atof(Spl[0].c_str()) / 1000.0;
	Time.Value = atof(Spl[1].c_str());

	if (Spl[6] == "1") // Non-inherited section
		Info->Difficulty->Timing.push_back(Time);
	else
	{
		// An inherited section would be added to a velocity changes vector which would later alter speeds.
		double OldValue = Time.Value;

		Time.Value = -100 / OldValue;

		Info->Difficulty->SpeedChanges.push_back(Time);
	}
}

int GetInterval(float Position, int Channels)
{
	float Step = 512.0 / Channels;

	return (int)(Position / Step);
}

#define NOTE_SLIDER 2
#define NOTE_HOLD 128
#define NOTE_NORMAL 1

String GetSampleFilename(SplitResult &Spl, int NoteType, int Hitsound)
{
	int SampleSet, SampleSetAddition, CustomSample, Volume;
	String SampleFilename;

	size_t SplSize = Spl.size();

	if (NoteType & NOTE_HOLD)
	{
		if (SplSize > 5 && Spl[5].length())
			return Spl[5];

		if (Spl.size() == 4)
		{
			SampleSet = atoi(Spl[1].c_str());
			SampleSetAddition = atoi(Spl[2].c_str());
			CustomSample = atoi(Spl[3].c_str());
		}else
		{
			SampleSet = atoi(Spl[0].c_str());
			SampleSetAddition = atoi(Spl[1].c_str());
			CustomSample = atoi(Spl[2].c_str());
		}

		/*
		if (SplCnt > 4)
			Volume = atoi(Spl[4].c_str()); // ignored lol
			*/
	}else if (NoteType & NOTE_NORMAL)
	{
		if (SplSize > 4 && Spl[4].length())
			return Spl[4];

		SampleSet = atoi(Spl[0].c_str());
		SampleSetAddition = atoi(Spl[1].c_str());
		CustomSample = atoi(Spl[2].c_str());

		/*
		if (SplCnt > 3)
			Volume = atoi(Spl[3].c_str()); // ignored
			*/
	}else if (NoteType & NOTE_SLIDER)
	{
		SampleSet = SampleSetAddition = CustomSample = 0;
	}

	String SampleSetString;

	if (SampleSet)
	{
		// translate sampleset int into samplesetstring
		SampleSetString = "normal";
	}else
	{
		// get sampleset string from sampleset active at starttime
		SampleSetString = "normal";
	}

	String CustomSampleString;

	if (CustomSample)
	{
		char dst[16];
		itoa(CustomSample, dst, 10);
		CustomSampleString = dst;
	}

	String HitsoundString;

	if (Hitsound)
	{
		switch (Hitsound)
		{
		case 1:
			HitsoundString = "normal";
		case 2:
			HitsoundString = "whistle";
		case 4:
			HitsoundString = "finish";
		case 8:
			HitsoundString = "clap";
		default:
			break;
		}
	}else
		HitsoundString = "normal";

	if (CustomSample)
	{
		SampleFilename = SampleSetString + "-hit" + HitsoundString + CustomSampleString + ".wav";
	}
	else
		SampleFilename = SampleSetString + "-hit" + HitsoundString + ".wav";

	return SampleFilename;
}

void ReadObjects (String line, OsuLoadInfo* Info)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));

	int Track = GetInterval(atof(Spl[0].c_str()), Info->Difficulty->Channels);
	int Hitsound;
	TrackNote Note;
	Note.AssignTrack(Track);

	SplitResult Spl2;

	/* 
		A few of these "ifs" are just since v11 and v12 store hold endtimes in different locations.
	*/
	int splitType = 5;
	if (Spl.size() == 7)
		splitType = 6;
	
	boost::split(Spl2, Spl[splitType], boost::is_any_of(":"));


	double startTime = atof(Spl[2].c_str()) / 1000.0;
	int NoteType = atoi(Spl[3].c_str());

	if (NoteType & NOTE_HOLD)
	{
		float endTime;
		if (splitType == 5)
			endTime = atof(Spl2[0].c_str()) / 1000.0;
		else
			endTime = atof(Spl[5].c_str()) / 1000.0;

		if (startTime > endTime)
			printf("o!m loader warning: object at track %d has startTime > endTime (%f and %f)\n", Track, startTime, endTime);

		Note.AssignTime( startTime, endTime );

		Info->Difficulty->TotalScoringObjects += 2;
		Info->Difficulty->TotalHolds++;
	}else if (NoteType & NOTE_NORMAL)
	{
		Note.AssignTime( startTime );
		Info->Difficulty->TotalNotes++;
		Info->Difficulty->TotalScoringObjects++;

	}else if (NoteType & NOTE_SLIDER)
	{
		// 6=repeats 7=length
		float sliderRepeats = atof(Spl[6].c_str());
		float sliderLength = atof(Spl[7].c_str());

		float Multiplier = 1;

		if (Info->Difficulty->SpeedChanges.size())
		{
			if (startTime >= Info->Difficulty->SpeedChanges.at(0).Time)
				Multiplier = SectionValue(Info->Difficulty->SpeedChanges, startTime);
		}

		float finalSize = sliderLength * sliderRepeats * Multiplier;
		float beatDuration = (finalSize / Info->SliderVelocity); 
		float bpm = (60000.0 / SectionValue(Info->Difficulty->Timing, startTime));
		float finalLength = beatDuration * spb(bpm);

		if (startTime > finalLength + startTime)
			printf("o!m loader warning: object at track %d has startTime > endTime (%f and %f)\n", Track, startTime, finalLength + startTime);

		Note.AssignTime( startTime, finalLength + startTime );

		Info->Difficulty->TotalScoringObjects += 2;
		Info->Difficulty->TotalHolds++;
	}

	Hitsound = atoi(Spl[4].c_str());

	String Sample = GetSampleFilename(Spl2, NoteType, Hitsound);

	if (Sample.length())
	{
		if (Info->Sounds.find(Sample) == Info->Sounds.end())
		{
			Info->Sounds[Sample] = Info->last_sound_index;
			Info->last_sound_index++;
		}

		Note.AssignSound(Info->Sounds[Sample]);
	}

	Info->Difficulty->TotalObjects++;
	Info->Difficulty->Measures[Track].at(0).MeasureNotes.push_back(Note);

	Info->Difficulty->Duration = std::max((double)Info->Difficulty->Duration, (double)Note.GetTimeFinal());
}

void NoteLoaderOM::LoadObjectsFromFile(String filename, String prefix, Song7K *Out)
{
	std::ifstream filein (filename.c_str());
	SongDiff Difficulty = new SongInternal::Difficulty7K();
	OsuLoadInfo Info;

	Info.Song = Out;
	Info.SliderVelocity = 1.4;
	Info.Difficulty = Difficulty;
	Info.last_sound_index = 1;

	// osu! stores bpm information as the time in ms that a beat lasts.
	Out->BPMType = Song7K::BT_Beatspace;
	Out->SongDirectory = prefix;

	if (!filein.is_open())
	{
		delete Difficulty;
		return;
	}

	Out->SongDirectory = prefix;
	Difficulty->TotalNotes = Difficulty->TotalHolds = Difficulty->TotalObjects = Difficulty->TotalScoringObjects = 0;
	Difficulty->Duration = 0;

	/* 
		Just like BMS, osu!mania charts have timing data separated by files
		and a set is implied using folders.
	*/
	Out->UseSeparateTimingData = true;

	String Line;

	enum 
	{
		RNotKnown,
		RGeneral,
		RMetadata,
		RDifficulty,
		REvents,
		RTiming,
		RHitobjects
	} ReadingMode = RNotKnown, ReadingModeOld = RNotKnown;

	while (filein)
	{
		std::getline(filein, Line);
		boost::replace_all(Line, "\r", "");

		if (!Line.length())
			continue;

		if (Line == "[General]")
		{
			ReadingMode = RGeneral;
		}else if (Line == "[Metadata]")
		{
			ReadingMode = RMetadata;
		}else if (Line == "[Difficulty]")
		{
			ReadingMode = RDifficulty;
		}else if (Line == "[Events]")
		{
			ReadingMode = REvents;
		}else if (Line == "[TimingPoints]")
		{
			ReadingMode = RTiming;
		}else if (Line == "[HitObjects]")
		{
			ReadingMode = RHitobjects;
		}else if (Line[0] == '[')
			ReadingMode = RNotKnown;

		if (ReadingMode != ReadingModeOld || ReadingMode == RNotKnown) // Skip this line since it changed modes, or it's not a valid section yet
		{
			ReadingModeOld = ReadingMode;
			continue;
		}

		switch (ReadingMode)
		{
		case RGeneral: if (!ReadGeneral(Line, &Info))  // don't load charts that we can't work with
					   {
						   delete Difficulty; 
						   return;
					   } 
					   break;
		case RMetadata: ReadMetadata(Line, &Info); break;
		case RDifficulty: ReadDifficulty(Line, &Info); break;
		case REvents: ReadEvents(Line, &Info); break;
		case RTiming: ReadTiming(Line, &Info); break;
		case RHitobjects: ReadObjects(Line, &Info); break;
		default: break;
		}
	}

	for (std::map<String, int>::iterator i = Info.Sounds.begin(); i != Info.Sounds.end(); i++)
	{
		Difficulty->SoundList[i->second] = i->first;
	}

	Out->Difficulties.push_back(Difficulty);
}
