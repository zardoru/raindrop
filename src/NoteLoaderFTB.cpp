#include <fstream>
#include <map>

#include "Global.h"
#include "Song7K.h"
#include "NoteLoader7K.h"


#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace VSRG;

/*
	This is pretty much the simplest possible loader.

	The important part is: all notes should contain a start time, possibly an end time (both in seconds)
	and you require at least a single BPM to calculate speed properly. And that's it, set the duration
	of the chart and the loading is done.
*/

typedef std::vector<String> SplitResult;

void NoteLoaderFTB::LoadMetadata(String filename, String prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif


	if (!filein.is_open())
		return;

	String Author;
	String Title;
	String musName;

	std::getline(filein, musName);
	std::getline(filein, Title);
	std::getline(filein, Author);

	Out->SongFilename = musName;
	Out->SongName = Title;
	Out->SongAuthor = Author;
	Out->SongDirectory = prefix + "/";
	Out->UseSeparateTimingData = true;

	filein.close();
}

void NoteLoaderFTB::LoadObjectsFromFile(String filename, String prefix, Song *Out)
{
	Difficulty *Diff = new Difficulty();
	Measure Msr;

#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif

	if (!filein.is_open())
	{
failed:
		delete Diff;
		return;
	}

	Out->BPMType = Song::BT_MS; // MS using BPMs.
	Diff->Channels = 7;
	Diff->LMT = Utility::GetLMT(filename);

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
			TimingSegment Seg;
			Seg.Time = atof(LineContents[1].c_str()) / 1000.0;
			Seg.Value = atof(LineContents[2].c_str());
			Diff->Timing.push_back(Seg);
		}else
		{

		/* We'll make a few assumptions about the structure from now on
			->The vertical speeds determine note position, not measure
			->More than one measure is unnecessary when using BT_MS, 
				so we'll use a single measure for all the song, containing all notes.
		*/

			NoteData Note;
			SplitResult NoteInfo;
			boost::split(NoteInfo, LineContents.at(0), boost::is_any_of("-"));
			if (NoteInfo.size() > 1)
			{
				Note.StartTime = atof(NoteInfo.at(0).c_str()) / 1000.0;
				Note.EndTime = atof(NoteInfo.at(1).c_str()) / 1000.0;
				Diff->TotalHolds++;
				Diff->TotalScoringObjects += 2;
			}
			else
			{
				Note.StartTime = atof(NoteInfo.at(0).c_str()) / 1000.0;
				Diff->TotalNotes++;
				Diff->TotalScoringObjects++;
			}

			/* index 1 is unused */
			int Track = atoi(LineContents[2].c_str()); // Always > 1
			Diff->TotalObjects++;

			Diff->Duration = max(max(Note.StartTime, Note.EndTime), Diff->Duration);
			Msr.MeasureNotes[Track-1].push_back(Note);
		}
	}

	filein.close();

	if (Diff->Timing.size())
		Diff->Timing[0].Time = 0;
	else
		goto failed;

	Out->Difficulties.push_back(Diff);
}
