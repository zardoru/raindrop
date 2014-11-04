#include <fstream>
#include <map>
#include <stdlib.h>

#include "Global.h"
#include "Logging.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>

typedef std::vector<GString> SplitResult;

using namespace VSRG;

#define SAMPLESET_NORMAL 1
#define SAMPLESET_SOFT 2
#define SAMPLESET_DRUM 3

#define HITSOUND_NORMAL 0
#define HITSOUND_WHISTLE 2
#define HITSOUND_FINISH 4
#define HITSOUND_CLAP 8

struct HitsoundSectionData
{
	float Time;
	int Sampleset;
	int Volume;
	int Custom;
};

struct OsuLoadInfo
{
	double SliderVelocity;
	int Version;
	int last_sound_index;
	VSRG::Song *OsuSong;
	VSRG::OsuManiaTimingInfo *TimingInfo;
	std::map <GString, int> Sounds;
	std::vector<HitsoundSectionData> HitsoundSections;
	Difficulty *Diff;
	GString DefaultSampleset;
};

/* osu!mania loader. credits to wanwan159, woc2006, Zorori and the author of AIBat for helping me understand this. */

bool ReadGeneral (GString line, OsuLoadInfo* Info)
{
	GString Command = line.substr(0, line.find_first_of(" ")); // Lines are Information:<space>Content
	GString Content = line.substr(line.find_first_of(" ") + 1, line.length() - line.find_first_of(" "));

	if (Command == "AudioFilename:")
	{
		if (Content == "virtual")
		{
			Info->Diff->IsVirtual = true;
			return true;
		}
		else
		{
#ifdef VERBOSE_DEBUG
			printf("Audio filename found: %s\n", Content.c_str());
#endif
			Info->OsuSong->SongFilename = Content;
		}
	}else if (Command == "Mode:")
	{
		if (Content != "3") // It's not a osu!mania chart, so we can't use it.
			return false; // (What if we wanted to support taiko though?)
	}else if (Command == "SampleSet:")
	{
		boost::algorithm::to_lower(Content);
		Info->DefaultSampleset = Content;
	}

	return true;
}

void ReadMetadata (GString line, OsuLoadInfo* Info)
{
	GString Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
	GString Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

#ifdef VERBOSE_DEBUG
	printf("Command found: %s | Contents: %s\n", Command.c_str(), Content.c_str());
#endif

	if (Command == "Title")
	{
		Info->OsuSong->SongName = Content;
	}else if (Command == "Artist")
	{
		Info->OsuSong->SongAuthor = Content;
	}else if (Command == "Version")
	{
		Info->Diff->Name = Content;
	}else if (Command == "TitleUnicode")
	{
		if (Content.length() > 1)
			Info->OsuSong->SongName = Content;
	}else if (Command == "ArtistUnicode")
	{
		if (Content.length() > 1)
			Info->OsuSong->SongAuthor = Content;
	}
}

void ReadDifficulty (GString line, OsuLoadInfo* Info)
{
	GString Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
	GString Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

	// We ignore everything but the key count!
	if (Command == "CircleSize")
	{
		Info->Diff->Channels = atoi(Content.c_str());

		for (int i = 0; i < Info->Diff->Channels; i++) // Push a single measure
			Info->Diff->Measures.push_back(Measure());
	}else if (Command == "SliderMultiplier")
	{
		Info->SliderVelocity = latof(Content.c_str()) * 100;
	}
	else if (Command == "HPDrainRate")
	{
		Info->TimingInfo->HP = latof(Content.c_str());
	}
	else if (Command == "OverallDifficulty")
	{
		Info->TimingInfo->OD = latof(Content.c_str());
	}

}

void ReadEvents (GString line, OsuLoadInfo* Info)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));

	if (Spl.size() > 1)
	{
		if (Spl[0] == "0" && Spl[1] == "0")
		{
			boost::replace_all(Spl[2], "\"", "");
			Info->OsuSong->BackgroundFilename = Spl[2];
		}else if (Spl[0] == "5" || Spl[0] == "Sample")
		{
			boost::replace_all(Spl[3], "\"", "");

			if (Info->Sounds.find(Spl[3]) == Info->Sounds.end())
			{
				Info->Sounds[Spl[3]] = Info->last_sound_index;
				Info->last_sound_index++;
			}

			double Time = latof(Spl[1].c_str()) / 1000.0;
			int Evt = Info->Sounds[Spl[3]];
			AutoplaySound New;
			New.Time = Time;
			New.Sound = Evt;

			Info->Diff->BGMEvents.push_back(New);
		}
	}
}

void ReadTiming (GString line, OsuLoadInfo* Info)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));


	TimingSegment Time;
	Time.Time = latof(Spl[0].c_str()) / 1000.0;
	Time.Value = latof(Spl[1].c_str());

	if (Spl[6] == "1") // Non-inherited section
		Info->Diff->Timing.push_back(Time);
	else
	{
		// An inherited section would be added to a velocity changes vector which would later alter speeds.
		double OldValue = Time.Value;

		Time.Value = -100 / OldValue;

		Info->Diff->SpeedChanges.push_back(Time);
	}

	int Sampleset = -1;
	int Custom = 0;

	if (Spl.size() > 3)
		Sampleset = atoi(Spl[3].c_str());

	if (Spl.size() > 4)
		Custom = atoi(Spl[4].c_str());

	HitsoundSectionData SecData;
	SecData.Time = Time.Time;
	SecData.Sampleset = Sampleset;
	SecData.Custom = Custom;

	Info->HitsoundSections.push_back(SecData);
}

int GetInterval(float Position, int Channels)
{
	float Step = 512.0 / Channels;

	return (int)(Position / Step);
}

#define NOTE_SLIDER 2
#define NOTE_HOLD 128
#define NOTE_NORMAL 1

GString SamplesetToGString(int Sampleset)
{
	switch (Sampleset)
	{
	case SAMPLESET_SOFT:
		return "soft";
	case SAMPLESET_DRUM:
		return "drum";
	case SAMPLESET_NORMAL:
	default:
		return "normal";
	}
}

/* 
	This function is mostly correct; the main issue is that we'd have to know
	when custom = 0, that we should use 'per theme' default sounds.
	We don't have those, we don't use those, those are an osu!-ism
	so the sounds are not going to be 100% osu!-correct
	but they're going to be correct enough for virtual-mode charts to be accurate.
*/
GString GetSampleFilename(OsuLoadInfo *Info, SplitResult &Spl, int NoteType, int Hitsound, float Time)
{
	int SampleSet = 0, SampleSetAddition, CustomSample = 0;
	GString SampleFilename;

	if (!Spl.size()) // Handle this properly, eventually.
		return "normal-hitnormal.wav";

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
		if (Spl.size() > 1)
			SampleSetAddition = atoi(Spl[1].c_str());
		if (Spl.size() > 2)
			CustomSample = atoi(Spl[2].c_str());

		/*
		if (SplCnt > 3)
			Volume = atoi(Spl[3].c_str()); // ignored
			*/
	}else if (NoteType & NOTE_SLIDER)
	{
		SampleSet = SampleSetAddition = CustomSample = 0;
	}

	GString SampleSetGString;

	if (SampleSet)
	{
		// translate sampleset int into samplesetGString
		SampleSetGString = SamplesetToGString(SampleSet);
	}else
	{
		// get sampleset GString from sampleset active at starttime
		int Sampleset = -1;

		for (int i = 0; i < (int)Info->HitsoundSections.size() - (int)1; i++)
		{
			if (Info->HitsoundSections[i].Time <= Time && Info->HitsoundSections[i+1].Time < Time)
				Sampleset = Info->HitsoundSections[i].Sampleset;
			else if (Info->HitsoundSections[i+1].Time > Time)
				Sampleset = Info->HitsoundSections[i+1].Sampleset;
		}

		if (SampleSet == -1)
			SampleSetGString = Info->DefaultSampleset;
		else
			SampleSetGString = SamplesetToGString(Sampleset);
	}

	if (!CustomSample)
	{
		for (int i = 0; i < (int)Info->HitsoundSections.size()-(int)1; i++)
		{
			if (Info->HitsoundSections[i].Time <= Time && Info->HitsoundSections[i+1].Time < Time)
				CustomSample = Info->HitsoundSections[i].Custom;
			else if (Info->HitsoundSections[i+1].Time > Time)
				CustomSample = Info->HitsoundSections[i+1].Custom;
		}
	}

	GString CustomSampleGString;

	if (CustomSample)
	{
		std::stringstream ss;
		ss << CustomSample;
		CustomSampleGString = ss.str();
	}

	GString HitsoundGString;

	if (Hitsound)
	{
		switch (Hitsound)
		{
		case 1:
			HitsoundGString = "normal";
			break;
		case 2:
			HitsoundGString = "whistle";
			break;
		case 4:
			HitsoundGString = "finish";
			break;
		case 8:
			HitsoundGString = "clap";
		default:
			break;
		}
	}else
		HitsoundGString = "normal";

	if (CustomSample > 1)
	{
		SampleFilename = SampleSetGString + "-hit" + HitsoundGString + CustomSampleGString + ".wav";
	}
	else
		SampleFilename = SampleSetGString + "-hit" + HitsoundGString + ".wav";

	return SampleFilename;
}

void ReadObjects (GString line, OsuLoadInfo* Info)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));

	int Track = GetInterval(latof(Spl[0].c_str()), Info->Diff->Channels);
	int Hitsound;
	NoteData Note;

	SplitResult Spl2;

	/* 
		A few of these "ifs" are just since v11 and v12 store hold endtimes in different locations.
		Or not include some information at all...
	*/
	int splitType = 5;
	if (Spl.size() == 7)
		splitType = 6;
	else if (Spl.size() == 5)
		splitType = 4;
	
	if (splitType != 4) // only 5 entries
		boost::split(Spl2, Spl[splitType], boost::is_any_of(":"));


	double startTime = latof(Spl[2].c_str()) / 1000.0;
	int NoteType = atoi(Spl[3].c_str());

	if (NoteType & NOTE_HOLD)
	{
		float endTime;
		if (splitType == 5 && Spl2.size())
			endTime = latof(Spl2[0].c_str()) / 1000.0;
		else if (splitType == 6)
			endTime = latof(Spl[5].c_str()) / 1000.0;
		else // what really? a hold that doesn't bother to tell us when it ends?
			endTime = 0;

		Note.StartTime = startTime;
		Note.EndTime = endTime;

		if (startTime > endTime) { // Okay then, we'll transform this into a regular note..
			Log::Printf("NoteLoaderOM: object at track %d has startTime > endTime (%f and %f)\n", Track, startTime, endTime);
			Note.EndTime = 0;

			Info->Diff->TotalScoringObjects += 1;
			Info->Diff->TotalNotes++;
		}else {
			Info->Diff->TotalScoringObjects += 2;
			Info->Diff->TotalHolds++;
		}
	}else if (NoteType & NOTE_NORMAL)
	{
		Note.StartTime = startTime;
		Info->Diff->TotalNotes++;
		Info->Diff->TotalScoringObjects++;

	}else if (NoteType & NOTE_SLIDER)
	{
		// 6=repeats 7=length
		float sliderRepeats = latof(Spl[6].c_str());
		float sliderLength = latof(Spl[7].c_str());

		float Multiplier = 1;

		if (Info->Diff->SpeedChanges.size())
		{
			if (startTime >= Info->Diff->SpeedChanges.at(0).Time)
				Multiplier = SectionValue(Info->Diff->SpeedChanges, startTime);
		}

		float finalSize = sliderLength * sliderRepeats * Multiplier;
		float beatDuration = (finalSize / Info->SliderVelocity); 
		float bpm = (60000.0 / SectionValue(Info->Diff->Timing, startTime));
		float finalLength = beatDuration * spb(bpm);

		if (startTime > finalLength + startTime)
			printf("o!m loader warning: object at track %d has startTime > endTime (%f and %f)\n", Track, startTime, finalLength + startTime);

		Note.StartTime = startTime;
		Note.EndTime = finalLength + startTime;

		Info->Diff->TotalScoringObjects += 2;
		Info->Diff->TotalHolds++;
	}

	Hitsound = atoi(Spl[4].c_str());

	GString Sample = GetSampleFilename(Info, Spl2, NoteType, Hitsound, startTime);

	if (Sample.length())
	{
		if (Info->Sounds.find(Sample) == Info->Sounds.end())
		{
			Info->Sounds[Sample] = Info->last_sound_index;
			Info->last_sound_index++;
		}

		Note.Sound = Info->Sounds[Sample];
	}

	Info->Diff->TotalObjects++;
	Info->Diff->Measures[0].MeasureNotes[Track].push_back(Note);

	Info->Diff->Duration = max(max (Note.StartTime, Note.EndTime), Info->Diff->Duration);
}

void NoteLoaderOM::LoadObjectsFromFile(GString filename, GString prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif

	Difficulty *Diff = new Difficulty();
	OsuLoadInfo Info;

	Info.TimingInfo = new VSRG::OsuManiaTimingInfo;
	Info.OsuSong = Out;
	Info.SliderVelocity = 1.4;
	Info.Diff = Diff;
	Info.last_sound_index = 1;

	Diff->TimingInfo = Info.TimingInfo;

	// osu! stores bpm information as the time in ms that a beat lasts.
	Diff->BPMType = VSRG::Difficulty::BT_Beatspace;
	Out->SongDirectory = prefix;

	if (!filein.is_open())
	{
		delete Diff;
		return;
	}

	Diff->Filename = filename;
	Out->SongDirectory = prefix + "/";

	/* 
		Just like BMS, osu!mania charts have timing data separated by files
		and a set is implied using folders.
	*/

	GString Line;

	std::getline(filein, Line);
	int version;
	sscanf(Line.c_str(), "osu file format v%d", &version);

	if (version < 11) // why
	{
		delete Diff;
		return;
	}

	Info.Version = version;

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
						   delete Diff; 
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

	if (Diff->TotalObjects) 
	{
		Diff->Offset = Diff->Timing.begin()->Time;

		for (TimingData::iterator i = Diff->Timing.begin();
			i != Diff->Timing.end();
			i++)
		{
			i->Time -= Diff->Offset;
		}

		for (TimingData::iterator i = Diff->SpeedChanges.begin();
			i != Diff->SpeedChanges.end();
			i++)
		{
			i->Time -= Diff->Offset;
		}

		for (std::map<GString, int>::iterator i = Info.Sounds.begin(); i != Info.Sounds.end(); i++)
		{
			Diff->SoundList[i->second] = i->first;
		}

		Diff->Level = Diff->TotalScoringObjects / Diff->Duration;
		Out->Difficulties.push_back(Diff);
	}else
		delete Diff; // metadata-only difficulty
}
