#include <fstream>
#include <map>

#include "Global.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>


/* Stepmania uses a format with a lot of information, so we'll start with the basics*/

using namespace VSRG;
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
	// Utility::DebugBreak();
	return 0;
}

#undef ModeType

String RemoveComments(const String Str)
{
	String Result;
	int k = 0;
	int AwatingEOL = 0;
	for (uint32 i = 0; i < Str.length()-1; i++)
	{
		if (AwatingEOL)
		{
			if (Str[i] != '\n')
				continue;
			else
			{
				AwatingEOL = 0;
				continue;
			}
		}else
		{
			if (Str[i] == '/' && Str[i+1] == '/')
			{
				AwatingEOL = true;
				continue;
			}else
			{
				Result.push_back(Str.at(i));
				k++;
			}
		}
	}
	return Result;
}

void LoadTracksSM(Song *Out, Difficulty *Diff, String line)
{
	String CommandContents = line.substr(line.find_first_of(":") + 1);
	SplitResult Mainline;

	/* Remove newlines and comments */
	CommandContents = RemoveComments(CommandContents);
	boost::replace_all(CommandContents, "\n", "");
	boost::replace_all(CommandContents, "\r", "");
	boost::replace_all(CommandContents, " ", "");

	/* Split contents */
	boost::split(Mainline, CommandContents, boost::is_any_of(":"));

	/* What we'll work with */
	String NoteString = Mainline[5];
	int Keys = GetTracksByMode(Mainline[0]);

	if (!Keys)
		return;

	Diff->Channels = Keys;
	Diff->Name = Mainline[2];
	
	/* Now we should have our notes within NoteString. 
	We'll split them by measure using , as a separator.*/
	SplitResult MeasureText;
	boost::split(MeasureText, NoteString, boost::is_any_of(","));

	/* Hold data */
	double KeyStartTime[16];
	double KeyBeat[16]; // heh
	
	/* For each measure of the song */
	for (uint32 i = 0; i < MeasureText.size(); i++) /* i = current measure */
	{
		int MeasureFractions = MeasureText[i].length() / Keys;
		Measure Msr;

		if (MeasureText[i].length())
		{
			/* For each fraction of the measure*/
			for (int32 m = 0; m < MeasureFractions; m++) /* m = current fraction */
			{
				double Beat = i * 4.0 + m * 4.0 / (double)MeasureFractions; /* Current beat */
				double StopsTime = StopTimeAtBeat(Diff->StopsTiming, Beat);
				double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Beat) + StopsTime;
				/* For every track of the fraction */
				for (int k = 0; k < Keys; k++) /* k = current track */
				{
					NoteData Note;
					
					switch (MeasureText[i].at(0))
					{
					case '1': /* Taps */
						Note.StartTime = Time;

						Diff->TotalNotes++;
						Diff->TotalObjects++;
						Diff->TotalScoringObjects++;

						Msr.MeasureNotes[k].push_back(Note);
						break;
					case '2': /* Holds */
					case '4':
						KeyStartTime[k] = Time;
						KeyBeat[k] /*heh*/ = Beat;
						Diff->TotalScoringObjects++;
						break;
					case '3': /* Hold releases */
						Note.StartTime = KeyStartTime[k];
						Note.EndTime = Time;
						
						Diff->TotalHolds++;
						Diff->TotalObjects++;
						Diff->TotalScoringObjects++;
						Msr.MeasureNotes[k].push_back(Note);
						break;
					default:
						break;
					}

					Diff->Duration = max(max (Note.StartTime, Note.EndTime), Diff->Duration);

					if (MeasureText[i].length())
						MeasureText[i].erase(0, 1);
				}
			}
		}

		Diff->Measures.push_back(Msr);
	}

	/* 
		Through here we can make a few assumptions.
		->The measures are in order from start to finish
		->Each measure has all potential playable tracks, even if that track is empty during that measure.
		->Measures are internally ordered
	*/
}

Song* NoteLoaderSM::LoadObjectsFromFile(String filename, String prefix)
{
	std::ifstream filein (filename.c_str());
	Song *Out = new Song();
	Difficulty *Diff = new Difficulty();

	// Stepmania uses beat-based locations for stops and BPM.
	Out->BPMType = Song::BT_Beat;

	if (!filein.is_open())
	{
#ifndef NDEBUG
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw; // std::exception(serr.str().c_str());
#else
		return NULL;
#endif
	}

	Out->SongDirectory = prefix + "/";
	Out->UseSeparateTimingData = false;
	Diff->Offset = 0;
	Diff->Duration = 0;

	String line;
	while (filein)
	{
		std::getline(filein, line, ';'); 

		if (line.size() < 3)
			continue;

		String command = line.substr(line.find_first_of("#"), line.find_first_of(":") - line.find_first_of("#"));

		boost::replace_all(command, "\n", "");

#define OnCommand(x) if(command == #x || command == #x + std::string(":"))

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
			Out->BackgroundFilename = CommandContents;
		}

		OnCommand(#MUSIC)
		{
			Out->SongFilename = CommandContents;
		}

		OnCommand(#OFFSET)
		{
			std::stringstream str (CommandContents);
			str >> Diff->Offset;
			Out->Offset = Diff->Offset;
		}

		OnCommand(#BPMS)
		{
			LoadTimingList(Out->BPMData, line);
		}

		OnCommand(#STOPS)
		{
			LoadTimingList(Out->StopsData, line);
		}

		/* Stops: TBD */

		OnCommand(#NOTES)
		{
			Diff->Timing = Out->BPMData;
			Diff->StopsTiming = Out->StopsData;
			Diff->Offset = -Out->Offset;
			Diff->Duration = 0;

			LoadTracksSM(Out, Diff, line);
			Out->Difficulties.push_back(Diff);

			Diff = new Difficulty();
		}
	}

	delete Diff;
	return Out;
}
