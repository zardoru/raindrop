#include <boost/algorithm/string/split.hpp>
#include <fstream>

#include "Global.h"
#include "NoteLoaderSM.h"
#include "NoteLoader.h"

/* Stepmania uses a format with a lot of information, so we'll start with the basics*/

using namespace NoteLoaderSM;
typedef std::vector<String> SplitResult;

#define ModeType(m,keys) if(mode==#m) return keys;

int GetTracksByMode(String mode)
{
	ModeType(kb7-single, 7);
	ModeType(dance-single, 4);
	ModeType(dance-solo, 6);
	ModeType(dance-double, 8);
	ModeType(pump-single, 5);
	ModeType(pump-double, 10);
	Utility::DebugBreak();
	return 1;
}

#undef ModeType

void LoadTracksSM(Song7K *Out, SongInternal::TDifficulty<TrackNote> *Difficulty, String line)
{
	String CommandContents = line.substr(line.find_first_of(":") + 1);
	SplitResult Mainline;

	/* Remove newlines */
	boost::replace_all(CommandContents, "\n", "");

	/* Split contents */
	boost::split(Mainline, CommandContents, boost::is_any_of(":"));

	/* What we'll work with */
	String NoteString = Mainline[5];
	int Keys = GetTracksByMode(Mainline[0]);

	Difficulty->Channels = Keys;
	Difficulty->Name = Mainline[2];
	
	/* Now we should have our notes within NoteString. 
	We'll split them by measure using , as a separator.*/
	SplitResult MeasureText;
	boost::split(MeasureText, NoteString, boost::is_any_of(","));

	/* Hold data */
	float KeyStartTime[16];
	int KeyMeasure[16];
	int KeyFraction[16];
	
	/* For each measure of the song */
	for (unsigned int i = 0; i < MeasureText.size(); i++) /* i = current measure */
	{
		int MeasureFractions = MeasureText[i].length() / Keys;
		SongInternal::Measure<TrackNote> Measure;
		
		/* For each fraction of the measure*/
		for (int m = 0; m < MeasureFractions; m++) /* m = current fraction */
		{
			float Beat = i * Out->MeasureLength + m / MeasureFractions; /* Current beat */

			/* For every track of the fraction */
			for (int k = 0; k < Keys; k++) /* k = current track */
			{
				TrackNote Note;
				Note.AssignTrack(k);
				switch (MeasureText[i].at(0))
				{
				case '1': /* Taps */
					Note.AssignSongPosition(i, m);
					Note.AssignTime(TimeAtBeat(*Difficulty, Beat), 0);
					Measure.MeasureNotes.push_back(Note);
					break;
				case '2': /* Holds */
				case '4':
					KeyStartTime[k] = TimeAtBeat(*Difficulty, Beat);
					KeyMeasure[k] = i;
					KeyFraction[k] = m;
					break;
				case '3': /* Hold releases */
					Note.AssignTime(KeyStartTime[k], TimeAtBeat(*Difficulty, Beat));
					Note.AssignSongPosition(KeyMeasure[k], KeyFraction[k]);
					Measure.MeasureNotes.push_back(Note);
					break;
				default:
					break;
				}
				MeasureText[i].erase(0);
			}
		}
		Difficulty->Measures.push_back(Measure);
	}
}

Song7K* NoteLoaderSM::LoadObjectsFromFile(String filename, String prefix)
{
	std::ifstream filein (filename.c_str());
	Song7K *Out = new Song7K();
	SongInternal::TDifficulty<TrackNote> *Difficulty = new SongInternal::TDifficulty<TrackNote>();

	if (!filein.is_open())
	{
#ifndef NDEBUG
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw std::exception(serr.str().c_str());
#else
		return NULL;
#endif
	}

	Out->SongDirectory = prefix;

	String line;
	while (!filein.eof())
	{
		std::getline(filein, line, ';'); 
		String command = line.substr(0, line.find_first_of(":"));

#define OnCommand(x) if(command.find(#x)!=String::npos)

		String CommandContents = line.substr(line.find_first_of(":") + 1);

		OnCommand(#TITLE)
		{
			Out->SongName = CommandContents;
		}

		OnCommand(#ARTIST)
		{
			Out->SongAuthor = CommandContents;
		}

		OnCommand(#BACKGROUND)
		{
			Out->BackgroundRelativeDir = CommandContents;
			Out->BackgroundDir = prefix + "/" + CommandContents;
		}

		OnCommand(#MUSIC)
		{
			Out->SongFilename = prefix + "/" + CommandContents;
			Out->SongRelativePath = CommandContents;
		}

		OnCommand(#OFFSET)
		{
			std::stringstream str (CommandContents);
			str >> Difficulty->Offset;
			Difficulty->Offset = -Difficulty->Offset;

			Out->LeadInTime = Difficulty->Offset < 0? abs(Difficulty->Offset) + 3 : 0;
		}

		OnCommand(#BPMS)
		{
			NoteLoader::LoadBPMs(Out, Difficulty, line);
		}

		/* Stops: TBD */

		OnCommand(#NOTES)
		{
			LoadTracksSM(Out, Difficulty, line);
			Out->Difficulties.push_back(Difficulty);
			Difficulty = new SongInternal::TDifficulty<TrackNote>();
		}
	}

	delete Difficulty;
	return Out;
}