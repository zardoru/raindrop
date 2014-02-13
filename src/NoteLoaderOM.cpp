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

typedef SongInternal::TDifficulty<TrackNote> *SongDiff;
typedef std::vector<String> SplitResult;

// void prototype (String line, Song7K *Out, SongDiff Difficulty)

bool ReadGeneral (String line, Song7K *Out, SongDiff Difficulty)
{
	String Command = line.substr(0, line.find_first_of(" ")); // Lines are Information:<space>Content
	String Content = line.substr(line.find_first_of(" ") + 1, line.length() - line.find_first_of(" "));

	if (Command == "AudioFilename:")
	{
		if (Content == "virtual") // Virtual mode not yet supported.
			return false;
		else
		{
			Out->SongFilename = Out->SongDirectory + "/" + Content;
			Out->SongRelativePath = Content;
		}
	}else if (Command == "Mode:")
	{
		if (Content != "3") // It's not a osu!mania chart, so we can't use it.
			return false; // (What if we wanted to support taiko though?)
	}

	return true;
}

void ReadMetadata (String line, Song7K *Out, SongDiff Difficulty)
{
	String Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
	String Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

	if (Command == "Title")
	{
		Out->SongName = Content;
	}else if (Command == "Artist")
	{
		Out->SongAuthor = Content;
	}else if (Command == "Version")
	{
		Difficulty->Name = Content;
	}
}

void ReadDifficulty (String line, Song7K *Out, SongDiff Difficulty)
{
	String Command = line.substr(0, line.find_first_of(":")); // Lines are Information:Content
	String Content = line.substr(line.find_first_of(":") + 1, line.length() - line.find_first_of(":"));

	// We ignore everything but the key count!
	if (Command == "CircleSize")
	{
		Difficulty->Channels = atoi(Content.c_str());

		for (int i = 0; i < Difficulty->Channels; i++) // Push a single measure
			Difficulty->Measures[i].push_back(SongInternal::Measure<TrackNote>());
	}else if (Command == "SliderMultiplier")
	{
		Out->SliderVelocity = atof(Content.c_str()) * 100;
	}
}

void ReadEvents (String line, Song7K *Out, SongDiff Difficulty)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));

	if (Spl.size() > 1)
	{
		if (Spl[0] == "0" && Spl[1] == "0")
		{
			boost::replace_all(Spl[2], "\"", "");
			Out->BackgroundRelativeDir = Spl[2];
			Out->BackgroundDir = Out->SongDirectory + "/" + Spl[2];
		}
	}
}

void ReadTiming (String line, Song7K *Out, SongDiff Difficulty)
{
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));


	SongInternal::TimingSegment Time;
	Time.Time = atof(Spl[0].c_str()) / 1000;
	Time.Value = atof(Spl[1].c_str());

	if (Spl[6] == "1") // Non-inherited section
		Difficulty->Timing.push_back(Time);
	else
	{
		// An inherited section would be added to a velocity changes vector which would later alter speeds.
		float OldValue = Time.Value;

		Time.Value = -100 / OldValue;

		Difficulty->SpeedChanges.push_back(Time);
	}
}

int GetInterval(float Position, int Channels)
{
	float Step = 512.0 / Channels;
	float A = 0, B = 0;
	int It = 0;

	while (It < Channels)
	{
		A = B;
		B = (It + 1) * Step;

		if (Position > A && Position < B)
			return It;

		It++;
	}

	return 0;
}

void ReadObjects (String line, Song7K *Out, SongDiff Difficulty)
{
	// Ignoring holds for now for easier reading.
	SplitResult Spl;
	boost::split(Spl, line, boost::is_any_of(","));

	int Track = GetInterval(atof(Spl[0].c_str()), Difficulty->Channels);
	TrackNote Note;
	Note.AssignTrack(Track);

	SplitResult Spl2;
	boost::split(Spl2, Spl[5], boost::is_any_of(":"));
	float startTime = atof(Spl[2].c_str()) / 1000.0;

	if (Spl[3] == "128")
	{
		float endTime = atof(Spl2[0].c_str()) / 1000.0;

		Note.AssignTime( startTime, endTime );

		Difficulty->TotalScoringObjects += 2;
		Difficulty->TotalHolds++;
	}else if (Spl[3] == "1")
	{
		Note.AssignTime( startTime );
		Difficulty->TotalNotes++;
		Difficulty->TotalScoringObjects++;
	}else if (Spl[3] == "2")
	{
		// 6=repeats 7=length
		float sliderRepeats = atof(Spl[6].c_str());
		float sliderLength = atof(Spl[7].c_str());
		float finalSize = sliderLength * sliderRepeats * (Difficulty->SpeedChanges.size() ? BpmAtBeat(Difficulty->SpeedChanges, startTime) : 1);
		float beatDuration = (finalSize / Out->SliderVelocity); 
		float bpm = (60000.0 / BpmAtBeat(Difficulty->Timing, startTime));
		float finalLength = beatDuration * spb(bpm);

		Note.AssignTime( startTime, finalLength + startTime );

		Difficulty->TotalScoringObjects += 2;
		Difficulty->TotalHolds++;
	}

	Difficulty->TotalObjects++;
	Difficulty->Measures[Track].at(0).MeasureNotes.push_back(Note);

	Difficulty->Duration = std::max((double)Difficulty->Duration, (double)Note.GetTimeFinal());
}

void NoteLoaderOM::LoadObjectsFromFile(String filename, String prefix, Song7K *Out)
{
	std::ifstream filein (filename.c_str());
	SongDiff Difficulty = new SongInternal::TDifficulty<TrackNote>();

	// BMS uses beat-based locations for stops and BPM. (Though the beat must be calculated.)
	Out->BPMType = Song7K::BT_Beatspace;
	Out->SongDirectory = prefix;

	if (!filein.is_open())
	{
		delete Difficulty;
		return;
	}

	Out->SongDirectory = prefix;
	Difficulty->TotalNotes = Difficulty->TotalHolds = Difficulty->TotalObjects = Difficulty->TotalScoringObjects = 0;

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
		case RGeneral: ReadGeneral(Line, Out, Difficulty); break;
		case RMetadata: ReadMetadata(Line, Out, Difficulty); break;
		case RDifficulty: ReadDifficulty(Line, Out, Difficulty); break;
		case REvents: ReadEvents(Line, Out, Difficulty); break;
		case RTiming: ReadTiming(Line, Out, Difficulty); break;
		case RHitobjects: ReadObjects(Line, Out, Difficulty); break;
		default: break;
		}
	}

	Out->Difficulties.push_back(Difficulty);
}