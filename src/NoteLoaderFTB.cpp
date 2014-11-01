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

typedef std::vector<GString> SplitResult;

void NoteLoaderFTB::LoadMetadata(GString filename, GString prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif


	if (!filein.is_open())
		return;

	GString Author;
	GString Title;
	GString musName;

	std::getline(filein, musName);
	std::getline(filein, Title);
	std::getline(filein, Author);

	Out->SongFilename = musName;
	Out->SongName = Title;
	Out->SongAuthor = Author;
	Out->SongDirectory = prefix + "/";

	filein.close();
}

void NoteLoaderFTB::LoadObjectsFromFile(GString filename, GString prefix, Song *Out)
{
	Difficulty *Diff = new Difficulty();
	Measure Msr;

#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif

	Diff->Filename = filename;

	if (!filein.is_open())
	{
failed:
		delete Diff;
		return;
	}

	Diff->BPMType = VSRG::Difficulty::BT_MS; // MS using BPMs.
	Diff->Channels = 7;
	Diff->Name = Utility::RemoveExtension(Utility::RelativeToPath(filename));

	while (filein)
	{
		SplitResult LineContents;
		GString Line;
		std::getline(filein, Line);

		if (Line[0] == '#' || Line.length() == 0)
			continue;

		boost::split(LineContents, Line, boost::is_any_of(" "));

		if (LineContents.at(0) == "BPM")
		{
			TimingSegment Seg;
			Seg.Time = latof(LineContents[1].c_str()) / 1000.0;
			Seg.Value = latof(LineContents[2].c_str());
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
				Note.StartTime = latof(NoteInfo.at(0).c_str()) / 1000.0;
				Note.EndTime = latof(NoteInfo.at(1).c_str()) / 1000.0;
				Diff->TotalHolds++;
				Diff->TotalScoringObjects += 2;
			}
			else
			{
				Note.StartTime = latof(NoteInfo.at(0).c_str()) / 1000.0;
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
	{
		Diff->Offset = Diff->Timing.begin()->Time;

		for (TimingData::iterator i = Diff->Timing.begin();
			i != Diff->Timing.end();
			i++)
		{
			i->Time -= Diff->Offset;
		}
	}
	else
		goto failed;

	Out->Difficulties.push_back(Diff);
}
