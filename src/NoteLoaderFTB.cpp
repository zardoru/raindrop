#include <fstream>

#include "Global.h"
#include "NoteLoader7K.h"


#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

/*
	This is pretty much the simplest possible loader.

	The important part is: all notes should contain a start time, possibly an end time (both in seconds)
	and you require at least a single BPM to calculate speed properly. And that's it, set the duration
	of the chart and the loading is done.
*/

typedef std::vector<String> SplitResult;

void NoteLoaderFTB::LoadMetadata(String filename, String prefix, Song7K *Out)
{
	std::fstream filein ((filename).c_str());

	if (!filein.is_open())
		return;

	String Author;
	String Title;
	String musName;

	std::getline(filein, musName);
	std::getline(filein, Title);
	std::getline(filein, Author);

	Out->SongRelativePath = musName;
	Out->SongFilename = prefix + "/" + musName;
	Out->SongName = Title;
	Out->SongAuthor = Author;
	Out->SongDirectory = prefix;
	Out->UseSeparateTimingData = true;

	filein.close();
}

void NoteLoaderFTB::LoadObjectsFromFile(String filename, String prefix, Song7K *Out)
{
	SongInternal::Difficulty7K *Difficulty = new SongInternal::Difficulty7K ();
	SongInternal::Measure7K Measure[7];

	std::fstream filein ((filename).c_str());

	if (!filein.is_open())
	{
		delete Difficulty;
		return;
	}

	Out->BPMType = Song7K::BT_MS; // MS using BPMs.
	Difficulty->Channels = 7;
	Difficulty->Offset = 0;
	Difficulty->TotalNotes = Difficulty->TotalHolds = Difficulty->TotalObjects = Difficulty->TotalScoringObjects = 0;

	for (int i = 0; i < 7; i++)
		Measure[i].Fraction = -1;

	while (filein)
	{
		SplitResult LineContents;
		String Line;
		std::getline(filein, Line);

		if (Line[0] == '#' || Line.length() == 0)
			continue;

		boost::split(LineContents, Line, boost::is_any_of(" "));

		if (LineContents.at(0) == "BPM")
		{
			SongInternal::TimingSegment Seg;
			Seg.Time = atof(LineContents[1].c_str()) / 1000.0;
			Seg.Value = atof(LineContents[2].c_str());
			Difficulty->Timing.push_back(Seg);
		}else
		{

		/* We'll make a few assumptions about the structure from now on
			->The vertical speeds determine note position, not measure
			->More than one measure is unnecessary when using BT_MS, 
				so we'll use a single measure for all the song, containing all notes.
		*/

			TrackNote Note;
			SplitResult NoteInfo;
			boost::split(NoteInfo, LineContents.at(0), boost::is_any_of("-"));
			if (NoteInfo.size() > 1)
			{
				Note.AssignTime(atof(NoteInfo.at(0).c_str()) / 1000.0, atof(NoteInfo.at(1).c_str()) / 1000.0);
				Difficulty->TotalNotes++;
				Difficulty->TotalScoringObjects++;
			}
			else
			{
				Note.AssignTime(atof(NoteInfo.at(0).c_str()) / 1000.0, 0);
				Difficulty->TotalHolds++;
				Difficulty->TotalScoringObjects += 2;
			}

			/* index 1 is unused */
			int Track = atoi(LineContents[2].c_str()); // Always > 1
			Note.AssignTrack(Track-1);
			Difficulty->TotalObjects++;
			Measure[Track-1].MeasureNotes.push_back(Note);
		}
	}
	filein.close();

	Difficulty->Duration = 0;
	for (int i = 0; i < 7; i++)
	{
		Difficulty->Measures[i].push_back(Measure[i]);
		Difficulty->Duration = std::max(Difficulty->Measures[i][0].MeasureNotes.at(Difficulty->Measures[i][0].MeasureNotes.size()-1).GetTimeFinal(), Difficulty->Duration);
	}

	Difficulty->Timing[0].Time = 0;

	Out->Difficulties.push_back(Difficulty);
}
