#include <fstream>
#include <map>

#include "Global.h"
#include "Song7K.h"
#include "NoteLoader7K.h"
#include "utf8.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>

#include "ScoreKeeper.h"

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

bool LoadTracksSM(Song *Out, Difficulty *Diff, String line)
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

	if (Mainline.size() < 6) // No, like HELL I'm loading this.
	{
		// The first time I found this it was because a ; was used as a separator instead of a :
		// Which means a rewrite is what probably should be done to fix that particular case.
		wprintf(L"Corrupt simfile (%d entries instead of 6)", Mainline.size());
		return false;
	}

	/* What we'll work with */
	String NoteString = Mainline[5];
	int Keys = GetTracksByMode(Mainline[0]);

	if (!Keys)
		return false;

	Diff->Channels = Keys;
	Diff->Name = Mainline[2] + "(" + Mainline[0] + ")";
	
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
	return true;
}

void NoteLoaderSM::LoadObjectsFromFile(String filename, String prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif

	TimingData BPMData;
	TimingData StopsData; 
	double Offset;

	Difficulty *Diff = new Difficulty();

	// Stepmania uses beat-based locations for stops and BPM.
	Diff->BPMType = VSRG::Difficulty::BT_Beat;

	if (!filein.is_open())
	{
#ifndef NDEBUG
		std::stringstream serr;
		serr << "couldn't open \"" << filename << "\" for reading";
		throw; // std::exception(serr.str().c_str());
#else
		return;
#endif
	}

	Out->SongDirectory = prefix + "/";
	Diff->Offset = 0;
	Diff->Duration = 0;

	String line;
	while (filein)
	{
		std::getline(filein, line, ';'); 

		if (line.length() < 3)
			continue;

		String command;
		size_t iHash = line.find_first_of("#");
		size_t iColon = line.find_first_of(":");
		if (iHash != String::npos && iColon != String::npos)
			command = line.substr(iHash, iColon - iHash);
		else
			continue;

		boost::replace_all(command, "\n", "");

#define OnCommand(x) if(command == #x || command == #x + std::string(":"))

		String CommandContents = line.substr(line.find_first_of(":") + 1);
		

		OnCommand(#TITLE)
		{
			if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
			{
#ifdef WIN32
				Out->SongName = CommandContents;
#else
				Out->SongName = CommandContents;
				try {
					std::vector<int> cp;
					utf8::utf8to16 (CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
				}catch (utf8::not_enough_room &e) {
					Out->SongName = Utility::SJIStoU8(CommandContents);
				}
#endif
			}
			else
				Out->SongName = Utility::SJIStoU8(CommandContents);
		}

		OnCommand(#ARTIST)
		{
			if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
			{
#ifdef WIN32
				Out->SongAuthor = CommandContents;
#else
				Out->SongAuthor = CommandContents;
				try {
					std::vector<int> cp;
					utf8::utf8to16 (CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
				}catch (utf8::not_enough_room &e) {
					Out->SongAuthor = Utility::SJIStoU8(CommandContents);
				}
#endif
			}
			else
				Out->SongAuthor = Utility::SJIStoU8(CommandContents);
		}

		OnCommand(#BACKGROUND)
		{
			Out->BackgroundFilename = CommandContents;
		}

		OnCommand(#MUSIC)
		{
			if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
				Out->SongFilename = CommandContents;
			else
				Out->SongFilename = Utility::SJIStoU8(CommandContents);
		}

		OnCommand(#OFFSET)
		{
			std::stringstream str (CommandContents);
			str >> Offset;
		}

		OnCommand(#BPMS)
		{
			LoadTimingList(BPMData, line);
		}

		OnCommand(#STOPS)
		{
			LoadTimingList(StopsData, line);
		}

		/* Stops: TBD */

		OnCommand(#NOTES)
		{
			Diff->LMT = Utility::GetLMT(filename);
			Diff->Timing = BPMData;
			Diff->StopsTiming = StopsData;
			Diff->Offset = -Offset;
			Diff->Duration = 0;
			Diff->Filename = filename;
			Diff->BPMType = VSRG::Difficulty::BT_Beat;

			if (LoadTracksSM(Out, Diff, line))
			{
				Out->Difficulties.push_back(Diff);
				Diff = new Difficulty();
			}

		}
	}

	delete Diff;
}
