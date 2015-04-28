#include <fstream>
#include <map>

#include "GameGlobal.h"
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

/* Stepmania/SSC loader. Lacks delays and speeds for now. As well as keysounds. */

using namespace VSRG;
using namespace NoteLoaderSM;
typedef std::vector<GString> SplitResult;

#define ModeType(m,keys) if(mode==#m) return keys;

int GetTracksByMode(GString mode)
{
	ModeType(kb7-single, 7);
	ModeType(dance-single, 4);
	ModeType(dance-solo, 6);
	ModeType(dance-couple, 8);
	ModeType(dance-threepanel, 3);
	ModeType(dance-double, 8);
	ModeType(pump-single, 5);
	ModeType(pump-double, 10);
	ModeType(pump-halfdouble, 6);

	// Utility::DebugBreak();
	return 0;
}

#undef ModeType

GString RemoveComments(const GString Str)
{
	GString Result;
	int k = 0;
	int AwatingEOL = 0;
	ptrdiff_t len = Str.length() - 1;

	for (ptrdiff_t i = 0; i < len; i++)
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

	boost::replace_all(Result, "\n", "");
	boost::replace_all(Result, "\r", "");
	boost::replace_all(Result, " ", "");

	return Result;
}

// See if Time is within a warp section. We use this after already having calculated the warp times.
bool CalculateIfInWarpSection(Difficulty* Diff, double Time)
{
	for (auto Warp : Diff->Data->Warps)
	{
		double LowerBound = Warp.Time;
		double HigherBound = Warp.Time + Warp.Value;
		if (Time >= LowerBound && Time < HigherBound)
			return true;
	}

	return false;
}

void LoadNotesSM(Song *Out, Difficulty *Diff, SplitResult &MeasureText)
{
	/* Hold data */
	int Keys = Diff->Channels;
	double KeyStartTime[16];
	double KeyBeat[16]; // heh
	if (!Keys)
		return;

	/* For each measure of the song */
	for (size_t i = 0; i < MeasureText.size(); i++) /* i = current measure */
	{
		ptrdiff_t MeasureFractions = MeasureText[i].length() / Keys;
		Measure Msr;

		if (MeasureText[i].length())
		{
			/* For each fraction of the measure*/
			for (ptrdiff_t m = 0; m < MeasureFractions; m++) /* m = current fraction */
			{
				double Beat = i * 4.0 + m * 4.0 / (double)MeasureFractions; /* Current beat */
				double StopsTime = StopTimeAtBeat(Diff->Data->StopsTiming, Beat);
				double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Beat, true) + StopsTime;
				bool InWarpSection = CalculateIfInWarpSection(Diff, Time); 

				/* For every track of the fraction */
				for (ptrdiff_t k = 0; k < Keys; k++) /* k = current track */
				{
					NoteData Note;

					if (InWarpSection)
						Note.NoteKind = NK_FAKE;

					switch (MeasureText[i].at(0))
					{
					case '1': /* Taps */
						Note.StartTime = Time;

						if (!InWarpSection) {
							Diff->TotalNotes++;
							Diff->TotalObjects++;
							Diff->TotalScoringObjects++;
						}

						Msr.MeasureNotes[k].push_back(Note);
						break;
					case '2': /* Holds */
					case '4':
						KeyStartTime[k] = Time;
						KeyBeat[k] /*heh*/ = Beat;
						
						if (!InWarpSection)
							Diff->TotalScoringObjects++;
						break;
					case '3': /* Hold releases */
						Note.StartTime = KeyStartTime[k];
						Note.EndTime = Time;

						if (!CalculateIfInWarpSection(Diff, KeyStartTime[k])) {
							Note.NoteKind = NK_NORMAL; // Un-fake it.
							Diff->TotalHolds++;
							Diff->TotalObjects++;
							Diff->TotalScoringObjects++;
						}
						Msr.MeasureNotes[k].push_back(Note);
						break;
					default:
						break;
					}

					Diff->Duration = max(max(Note.StartTime, Note.EndTime), Diff->Duration);

					if (MeasureText[i].length() > 0)
						MeasureText[i].erase(0, 1);
				}
			}
		}

		Diff->Data->Measures.push_back(Msr);
	}
}

bool LoadTracksSM(Song *Out, Difficulty *Diff, GString line)
{
	GString CommandContents = line.substr(line.find_first_of(":") + 1);
	SplitResult Mainline;

	/* Remove newlines and comments */
	CommandContents = RemoveComments(CommandContents);

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
	GString NoteGString = Mainline[5];
	int Keys = GetTracksByMode(Mainline[0]);

	if (!Keys)
		return false;

	Diff->Level = atoi(Mainline[3].c_str());
	Diff->Channels = Keys;
	Diff->Name = Mainline[2] + "(" + Mainline[0] + ")";
	
	/* Now we should have our notes within NoteGString. 
	We'll split them by measure using , as a separator.*/
	SplitResult MeasureText;
	boost::split(MeasureText, NoteGString, boost::is_any_of(","));

	LoadNotesSM(Out, Diff, MeasureText);

	/* 
		Through here we can make a few assumptions.
		->The measures are in order from start to finish
		->Each measure has all potential playable tracks, even if that track is empty during that measure.
		->Measures are internally ordered
	*/
	return true;
}
#define OnCommand(x) if(command == #x || command == #x + GString(":"))
#define _OnCommand(x) else if(command == #x || command == #x + GString(":"))

void DoCommonSMCommands(GString command, GString CommandContents, Song* Out)
{
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
				utf8::utf8to16(CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
			}
			catch (utf8::not_enough_room &e) {
				Out->SongName = Utility::SJIStoU8(CommandContents);
			}
#endif
		}
		else
			Out->SongName = Utility::SJIStoU8(CommandContents);
	}

	_OnCommand(#ARTIST)
	{
		if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
		{
#ifdef WIN32
			Out->SongAuthor = CommandContents;
#else
			Out->SongAuthor = CommandContents;
			try {
				std::vector<int> cp;
				utf8::utf8to16(CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
			}
			catch (utf8::not_enough_room &e) {
				Out->SongAuthor = Utility::SJIStoU8(CommandContents);
			}
#endif
		}
		else
			Out->SongAuthor = Utility::SJIStoU8(CommandContents);
	}

	_OnCommand(#BACKGROUND)
	{
		Out->BackgroundFilename = CommandContents;
	}

	_OnCommand(#MUSIC)
	{
		if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
			Out->SongFilename = CommandContents;
		else
			Out->SongFilename = Utility::SJIStoU8(CommandContents);

		Out->SongPreviewSource = Out->SongFilename;
	}

	_OnCommand(#SAMPLESTART)
	{
		Out->PreviewTime = latof(CommandContents);
	}
}

// Convert SSC warps into Raindrop warps.
TimingData CalculateRaindropWarpData(VSRG::Difficulty* Diff, const TimingData& Warps)
{
	TimingData Ret;
	for (auto Warp : Warps)
	{
		// Since we use real song time instead of warped time to calculate warped time
		// no need for worry about having to align these.
		double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Warp.Time) +
			StopTimeAtBeat(Diff->Data->StopsTiming, Warp.Time); 

		double Value = Warp.Value * 60 / SectionValue(Diff->Timing, Warp.Time);
		TimingSegment New;
		New.Time = Time;
		New.Value = Value;

		Ret.push_back(New);
	}

	return Ret;
}

void NoteLoaderSSC::LoadObjectsFromFile(GString filename, GString prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein(filename.c_str());
#else
	std::ifstream filein(Utility::Widen(filename).c_str());
#endif

	TimingData BPMData;
	TimingData StopsData;
	TimingData WarpsData;
	double Offset = 0;

	shared_ptr<VSRG::Difficulty> Diff = nullptr;

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

	GString Banner;

	Out->SongDirectory = prefix + "/";

	GString line;
	while (filein)
	{
		std::getline(filein, line, ';');

		if (line.length() < 3)
			continue;

		GString command;
		size_t iHash = line.find_first_of("#");
		size_t iColon = line.find_first_of(":");
		if (iHash != GString::npos && iColon != GString::npos)
			command = line.substr(iHash, iColon - iHash);
		else
			continue;

		boost::replace_all(command, "\n", "");

		GString CommandContents = line.substr(line.find_first_of(":") + 1);

		OnCommand(#NOTEDATA)
		{
			Diff = make_shared<VSRG::Difficulty>();
			Diff->Data = make_shared<VSRG::DifficultyLoadInfo>();
		}

		DoCommonSMCommands(command, CommandContents, Out);
		
		OnCommand(#OFFSET)
		{
			std::stringstream str(CommandContents);
			str >> Offset;
		}

		OnCommand(#BPMS)
		{
			if (!Diff)
				LoadTimingList(BPMData, line);
			else
				LoadTimingList(Diff->Timing, line);
		}

		OnCommand(#STOPS)
		{
			if (!Diff)
				LoadTimingList(StopsData, line);
			else
				LoadTimingList(Diff->Data->StopsTiming, line);
		}

		OnCommand(#BANNER)
		{
			Banner = CommandContents;
		}

		OnCommand(#WARPS)
		{
			if (!Diff)
				LoadTimingList(WarpsData, CommandContents);
			else
				LoadTimingList(Diff->Data->Warps, CommandContents);
		}

		// Notedata only here

		if (!Diff) continue;

		OnCommand(#CREDIT)
		{
			Diff->Author = CommandContents;
		}

		OnCommand(#METER)
		{
			std::stringstream str(CommandContents);
			str >> Diff->Level;
		}

		OnCommand(#DIFFICULTY)
		{
			Diff->Name = CommandContents;
		}

		OnCommand(#CHARTSTYLE)
		{
			Diff->Name += " " + CommandContents;
		}

		OnCommand(#STEPSTYPE)
		{
			Diff->Channels = GetTracksByMode(CommandContents);
		}

		OnCommand(#NOTES)
		{
			Diff->BPMType = VSRG::Difficulty::BT_Beat;
			if (!Diff->Timing.size())
				Diff->Timing = BPMData;
			
			if (!Diff->Data->StopsTiming.size())
				Diff->Data->StopsTiming = StopsData;

			if (!Diff->Data->Warps.size())
				Diff->Data->Warps = WarpsData;

			Diff->Offset = -Offset;
			Diff->Duration = 0;
			Diff->Filename = filename;
			Diff->BPMType = VSRG::Difficulty::BT_Beat;
			Diff->Data->TimingInfo = make_shared<VSRG::StepmaniaTimingInfo>();
			Diff->Data->StageFile = Banner;

			Diff->Data->Warps = CalculateRaindropWarpData(Diff.get(), Diff->Data->Warps);

			CommandContents = RemoveComments(CommandContents);
			SplitResult Measures;
			boost::split(Measures, CommandContents, boost::is_any_of(","));
			LoadNotesSM(Out, Diff.get(), Measures);
			Out->Difficulties.push_back(Diff);
		}
	}
}

// Convert all negative stops to warps.
void CleanStopsData(Difficulty* Diff)
{
	TimingData tmpWarps;
	TimingData &Stops = Diff->Data->StopsTiming;

	for (auto i = Stops.begin(); i != Stops.end(); )
	{
		if (i->Value < 0) // Warpahead.
		{
			tmpWarps.push_back(*i);
			i = Stops.erase(i);
			if (i == Stops.end()) break;
			else continue;
		}
		++i;
	}

	for (auto Warp: tmpWarps)
	{
		double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Warp.Time, true) +
			StopTimeAtBeat(Diff->Data->StopsTiming, Warp.Time);

		TimingSegment New;
		New.Time = Time;
		New.Value = -Warp.Value;
		Diff->Data->Warps.push_back(New);
	}
}

// We need stops to already be warps by this point. Like with the other times, no need to account for the warps themselves.
void WarpifyTiming(Difficulty* Diff)
{
	for (auto i = Diff->Timing.begin(); i != Diff->Timing.end(); i++)
	{
		if (i->Value < 0)
		{
			i->Value = -i->Value;

			auto k = i + 1;
			if (k == Diff->Timing.end()) break;
			else
			{
				// for all negative sections between i and the next positive section 
				// add up their duration in seconds
				double warpDuration = spb(i->Value) * (k->Time - i->Time);
				double warpDurationBeats = (k->Time - i->Time);
				while (k->Value < 0)
				{
					k++;
					if (k != Diff->Timing.end())
					{
						// add the duration of section k, if there's one to determine it.
						auto p = k + 1;
						if (p != Diff->Timing.end())
						{
							warpDuration += spb(abs(k->Value)) * (p->Time - k->Time);
							warpDurationBeats += p->Time - k->Time;
						}
					}
				}
				// Now since W = DurationInBeats(Dn) + TimeToBeatsAtBPM(DurationInTime(Dn), NextPositiveBPM)
				// DurationInBeats is warpDurationBeats, DurationInTime is warpDuration. k->Value is NextPositiveBPM
				double warpTime = TimeAtBeat(Diff->Timing, Diff->Offset, i->Time, true) + StopTimeAtBeat(Diff->Data->StopsTiming, i->Time);
				
				TimingSegment New;
				New.Time = warpTime;
				New.Value = warpDuration * 2;
				
				Diff->Data->Warps.push_back(New);
				// And now that we're done, there's no need to check the negative BPMs inbetween this one and the next positive BPM, so...
				i = k;
				if (i == Diff->Timing.end()) break;
			}
		}
	}
}

void NoteLoaderSM::LoadObjectsFromFile(GString filename, GString prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
	std::ifstream filein (filename.c_str());
#else
	std::ifstream filein (Utility::Widen(filename).c_str());
#endif

	TimingData BPMData;
	TimingData StopsData; 
	double Offset = 0;

	shared_ptr<VSRG::Difficulty> Diff = make_shared<VSRG::Difficulty>();

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

	GString Banner;

	Out->SongDirectory = prefix + "/";
	Diff->Offset = 0;
	Diff->Duration = 0;
	Diff->Data = make_shared<VSRG::DifficultyLoadInfo>();

	GString line;
	while (filein)
	{
		std::getline(filein, line, ';'); 

		if (line.length() < 3)
			continue;

		GString command;
		size_t iHash = line.find_first_of("#");
		size_t iColon = line.find_first_of(":");
		if (iHash != GString::npos && iColon != GString::npos)
			command = line.substr(iHash, iColon - iHash);
		else
			continue;

		boost::replace_all(command, "\n", "");

		GString CommandContents = line.substr(line.find_first_of(":") + 1);
		
		DoCommonSMCommands(command, CommandContents, Out);
		
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
			Diff->Timing = BPMData;
			Diff->Data = make_shared<VSRG::DifficultyLoadInfo>();
			Diff->Data->StopsTiming = StopsData;
			Diff->Offset = -Offset;
			Diff->Duration = 0;
			Diff->Filename = filename;
			Diff->BPMType = VSRG::Difficulty::BT_Beat;
			Diff->Data->TimingInfo = make_shared<VSRG::StepmaniaTimingInfo> ();
			Diff->Data->StageFile = Banner;
			CleanStopsData(Diff.get());
			WarpifyTiming(Diff.get());

			if (LoadTracksSM(Out, Diff.get(), line))
			{
				Out->Difficulties.push_back(Diff);
				Diff = make_shared<VSRG::Difficulty> ();
			}

		}
	}
}
