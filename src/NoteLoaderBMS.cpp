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

typedef std::map<int, String> FilenameListIndex;
typedef std::map<int, double> BpmListIndex;
typedef std::vector<TrackNote> NoteVector;

int fromBase36(const char *txt)
{
	return strtoul(txt, NULL, 36);
}

// The first wav will always be WAV01, not WAV00 since 00 is a reserved value for "nothing"
// Pretty fitting, in my opinion.

struct BMSMeasure
{
	NoteVector Notes;
};

String CommandSubcontents (String Command, String Line)
{
	uint32 len = Command.length();
	return Line.substr(len);
}

void NoteLoaderBMS::LoadObjectsFromFile(String filename, String prefix, Song7K *Out)
{
	std::ifstream filein (filename.c_str());
	SongInternal::Difficulty7K *Difficulty = new SongInternal::Difficulty7K();
	FilenameListIndex Sounds;
	BpmListIndex BPMs;

	// BMS uses beat-based locations for stops and BPM. (Though the beat must be calculated.)
	Out->BPMType = Song7K::BT_Beat;

	if (!filein.is_open())
	{
		delete Difficulty;
		return;
	}

	Out->SongDirectory = prefix;

	/* 
		BMS files are separated always one file, one difficulty, so it'd make sense
		that every BMS 'set' might have different timing information per chart.
		While BMS specifies no 'set' support it's usually implied using folders.
	*/
	Out->UseSeparateTimingData = true;

	/* 
		The default BMS specifies is 8 channels when #PLAYER is unset, however
		the modern BMS standard specifies to ignore #PLAYER and try to figure it out
		from the amount of used channels.

		And that's what we're going to try to do.
	*/


	Difficulty->Channels = 8; 

	String Line;
	while (filein)
	{
		std::getline(filein, Line);

		if (Line[0] != '#' || Line.length() == 0)
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

		OnCommand(#BACKBMP)
		{
			Out->BackgroundRelativeDir = CommandContents;
			Out->BackgroundDir = prefix + "/" + CommandContents;
		}

		OnCommandSub(#WAV)
		{
			String IndexStr = CommandSubcontents("#WAV", command);
			int Index = fromBase36(IndexStr.c_str());
			Sounds[Index] = CommandContents;
		}

		OnCommandSub(#BPM)
		{
			String IndexStr = CommandSubcontents("#BPM", command);
			int Index = fromBase36(IndexStr.c_str());
			BPMs[Index] = atof(CommandContents.c_str());
		}

		/* Else... */
		String MeasureCommand = Line.substr(Line.find_first_of(":")+1);
		String MainCommand = Line.substr(1, 5);

		if (Utility::IsNumeric(MainCommand.c_str())) // We've got work to do.
		{
			int Measure = atoi(MainCommand.substr(0,3).c_str());
		}

	}
}
