#include "gamecore-pch.h"
#include "fs.h"
#include "rmath.h"

#include "GameConstants.h"
#include "Song.h"
#include "TrackNote.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

#include "Utility.h"

#include "utf8.h"

#include <fstream>

/* Stepmania/SSC loader. Lacks delays and speeds for now. As well as keysounds. */

using namespace Game::VSRG;
using namespace NoteLoaderSM;

struct StepmaniaSpeed
{
    float Time, Duration, Value;
    enum ModeType : int { Beats, Seconds } Mode;
};

typedef std::vector<StepmaniaSpeed> SpeedData;

struct
{
    const char* name;
    uint8_t tracks;
} ModeTracks[] = {
    {"kb7-single", 7},
    {"dance-single", 4},
    {"dance-solo", 6},
    {"dance-couple", 8},
    {"dance-threepanel", 3},
    {"dance-double", 8},
    {"pump-single", 5},
    {"pump-double", 10},
    {"pump-halfdouble", 6}
};

int GetTracksByMode(std::string mode)
{
    for (auto v : ModeTracks)
    {
        if (mode == v.name)
            return v.tracks;
    }

    // Log::LogPrintf("Unknown track mode: %s, skipping difficulty\n", mode.c_str());
    return 0;
}

#undef ModeType

std::string RemoveComments(const std::string Str)
{
    std::string Result;
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
        }
        else
        {
            if (Str[i] == '/' && Str[i + 1] == '/')
            {
                AwatingEOL = true;
                continue;
            }
            else
            {
                Result.push_back(Str.at(i));
                k++;
            }
        }
    }

    Utility::ReplaceAll(Result, "[\\n\\r ]", "");
    return Result;
}

// See if Time is within a warp section. We use this after already having calculated the warp times.
bool IsTimeWithinWarp(Difficulty* Diff, double Time)
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

void LoadNotesSM(Song *Out, Difficulty *Diff, std::vector<std::string> &MeasureText)
{
    /* Hold data */
    int Keys = Diff->Channels;
	double KeyStartTime[16] = { 0 };
    double KeyBeat[16]; // heh
    if (!Keys)
        return;

    /* For each measure of the song */
    for (size_t i = 0; i < MeasureText.size(); i++) /* i = current measure */
    {
        ptrdiff_t MeasureSubdivisions = MeasureText[i].length() / Keys;
        Measure Msr;

        if (MeasureText[i].length())
        {
            /* For each fraction of the measure*/
            for (ptrdiff_t m = 0; m < MeasureSubdivisions; m++) /* m = current fraction */
            {
                double Beat = i * 4.0 + m * 4.0 / (double)MeasureSubdivisions; /* Current beat */
                double StopsTime = StopTimeAtBeat(Diff->Data->Stops, Beat);
                double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Beat, true) + StopsTime;
                bool InWarpSection = IsTimeWithinWarp(Diff, Time);

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
                        Msr.Notes[k].push_back(Note);
                        break;
                    case '2': /* Holds */
                    case '4':
                        KeyStartTime[k] = Time;
                        KeyBeat[k] /*heh*/ = Beat;

                        break;
                    case '3': /* Hold releases */
                        Note.StartTime = KeyStartTime[k];
                        Note.EndTime = Time;

                        if (!IsTimeWithinWarp(Diff, KeyStartTime[k]))
                            Note.NoteKind = NK_NORMAL; // Un-fake it.
                        Msr.Notes[k].push_back(Note);
                        break;
                    case 'F':
                        Note.StartTime = Time;
                        Note.NoteKind = NK_FAKE;

                        Msr.Notes[k].push_back(Note);
                    default:
                        break;
                    }

                    Diff->Duration = std::max(std::max(Note.StartTime, Note.EndTime), Diff->Duration);

                    if (MeasureText[i].length() > 0)
                        MeasureText[i].erase(0, 1);
                }
            }
        }

        Diff->Data->Measures.push_back(Msr);
    }
}

bool LoadTracksSM(Song *Out, Difficulty *Diff, std::string line)
{
    std::string CommandContents = line.substr(line.find_first_of(":") + 1);

    /* Remove newlines and comments */
    CommandContents = RemoveComments(CommandContents);

    auto Mainline = Utility::TokenSplit(CommandContents, ":");

    if (Mainline.size() < 6) // No, like HELL I'm loading this.
    {
        // The first time I found this it was because a ; was used as a separator instead of a :
        // Which means a rewrite is what probably should be done to fix that particular case.
        wprintf(L"Corrupt simfile (%d entries instead of 6)", Mainline.size());
        return false;
    }

    /* What we'll work with */
    std::string NoteString = Mainline[5];
    int Keys = GetTracksByMode(Mainline[0]);

    if (!Keys)
        return false;

    Diff->Level = atoi(Mainline[3].c_str());
    Diff->Channels = Keys;
    Diff->Name = Mainline[2] + "(" + Mainline[0] + ")";

    /* Now we should have our notes within NoteGString.
    We'll split them by measure using , as a separator.*/
    auto MeasureText = Utility::TokenSplit(NoteString);

    LoadNotesSM(Out, Diff, MeasureText);

    /*
        Through here we can make a few assumptions.
        ->The measures are in order from start to finish
        ->Each measure has all potential playable tracks, even if that track is empty during that measure.
        ->Measures are internally ordered
        */
    return true;
}
#define OnCommand(x) if(command == #x || command == #x + std::string(":"))
#define _OnCommand(x) else if(command == #x || command == #x + std::string(":"))

void DoCommonSMCommands(std::string command, std::string CommandContents, Song* Out)
{
    OnCommand(#TITLE)
    {
        if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
        {
#ifdef WIN32
            Out->Title = CommandContents;
#else
            Out->Title = CommandContents;
            try
            {
                std::vector<int> cp;
                utf8::utf8to16(CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
            }
            catch (utf8::not_enough_room &e)
            {
                Out->Title = Conversion::SJIStoU8(CommandContents);
            }
#endif
        }
        else
            Out->Title = Conversion::SJIStoU8(CommandContents);
    }

    _OnCommand(#SUBTITLE)
    {
        if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
        {
#ifdef WIN32
            Out->Subtitle = CommandContents;
#else
            Out->Subtitle = CommandContents;
            try
            {
                std::vector<int> cp;
                utf8::utf8to16(CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
            }
            catch (utf8::not_enough_room &e)
            {
                Out->Subtitle = Conversion::SJIStoU8(CommandContents);
            }
#endif
        }
        else
            Out->Artist = Conversion::SJIStoU8(CommandContents);
    }

    _OnCommand(#ARTIST)
    {
        if (utf8::is_valid(CommandContents.begin(), CommandContents.end()))
        {
#ifdef WIN32
            Out->Artist = CommandContents;
#else
            Out->Artist = CommandContents;
            try
            {
                std::vector<int> cp;
                utf8::utf8to16(CommandContents.begin(), CommandContents.end(), std::back_inserter(cp));
            }
            catch (utf8::not_enough_room &e)
            {
                Out->Artist = Conversion::SJIStoU8(CommandContents);
            }
#endif
        }
        else
            Out->Artist = Conversion::SJIStoU8(CommandContents);
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
            Out->SongFilename = Conversion::SJIStoU8(CommandContents);

        Out->SongPreviewSource = Out->SongFilename;
    }

    _OnCommand(#SAMPLESTART)
    {
        Out->PreviewTime = latof(CommandContents);
    }
}

// Convert SSC warps into Raindrop warps.
// Warning: This doesn't follow the beat-integration model dtinth programmed into Stepmania.
// It's not interpreted as "infinite bpm" but as literally jumping around.
// Should work for most cases. Haven't seen anyone put stops in the middle
// of their warps, fortunately.
TimingData CalculateRaindropWarpData(Difficulty* Diff, const TimingData& Warps)
{
	TimingData Ret;
	for (auto Warp : Warps)
	{
		// Since we use real song time instead of warped time to calculate warped time
		// no need for worry about having to align these.
		double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Warp.Time, true) +
			StopTimeAtBeat(Diff->Data->Stops, Warp.Time);

		double Value = Warp.Value * 60 / SectionValue(Diff->Timing, Warp.Time);

		Ret.push_back(TimingSegment(Time, Value));
	}

	return Ret;
}

// DRY etc I'll see it later.
TimingData CalculateRaindropScrollData(Difficulty* Diff, const TimingData& SCd)
{
	TimingData Ret;
	for (auto SC : SCd)
	{
		// No need to align these either, but since offset is applied at processing time for speed change
		// we need to set the offset at 0.
		double Time = TimeAtBeat(Diff->Timing, 0, SC.Time, true) +
			StopTimeAtBeat(Diff->Data->Stops, SC.Time);

		Ret.push_back(TimingSegment(Time, SC.Value));
	}

	return Ret;
}

// Transform the time data from beats to seconds
VectorInterpolatedSpeedMultipliers CalculateRaindropSpeedData(Difficulty *Diff, const SpeedData& In)
{
	VectorInterpolatedSpeedMultipliers Ret;

	for (auto scroll : In)
	{
		double Time = TimeAtBeat(Diff->Timing, 0, scroll.Time, true) +
			StopTimeAtBeat(Diff->Data->Stops, scroll.Time);
		double TimeEnd;

		if (scroll.Mode == StepmaniaSpeed::Beats)
		{
			TimeEnd = TimeAtBeat(Diff->Timing, 0, scroll.Time + scroll.Duration, true) +
				StopTimeAtBeat(Diff->Data->Stops, scroll.Time + scroll.Duration);
		}
		else
			TimeEnd = Time + scroll.Duration;

		SpeedSection newscroll;
		newscroll.Time = Time;
		newscroll.Duration = TimeEnd - Time;
		newscroll.Value = scroll.Value;

		Ret.push_back(newscroll);
	}

	return Ret;
}

SpeedData ParseScrolls(std::string line)
{
    auto ScrollLines = Utility::TokenSplit(line);
    SpeedData Ret;

    for (auto segment : ScrollLines)
    {
        auto data = Utility::TokenSplit(segment, "=");

        if (data.size() == 4)
        {
            StepmaniaSpeed newscroll;
            newscroll.Time = latof(data[0]);
            newscroll.Value = latof(data[1]);
            newscroll.Duration = latof(data[2]);
            newscroll.Mode = StepmaniaSpeed::ModeType(int(latof(data[3])));

            Ret.push_back(newscroll);
        }
    }

    return Ret;
}

void NoteLoaderSSC::LoadObjectsFromFile(std::filesystem::path filename, Song *Out)
{
    std::ifstream filein(filename.string());

    TimingData BPMData;
    TimingData StopsData;
    TimingData WarpsData;
    TimingData ScrollData;
    SpeedData speedData;
    SpeedData diffSpeedData;
    double Offset = 0;

    std::shared_ptr<Difficulty> Diff = nullptr;

    if (!filein.is_open())
        throw std::runtime_error(Utility::Format("couldn't open %s for reading", filename.c_str()).c_str());

    std::string Banner;

    std::string line;
    while (filein)
    {
        getline(filein, line, ';');

        if (line.length() < 3)
            continue;

        std::string command;
        size_t iHash = line.find_first_of("#");
        size_t iColon = line.find_first_of(":");
        if (iHash != std::string::npos && iColon != std::string::npos)
            command = line.substr(iHash, iColon - iHash);
        else
            continue;

        Utility::ReplaceAll(command, "\n", "");

        std::string CommandContents = line.substr(line.find_first_of(":") + 1);

        OnCommand(#NOTEDATA)
        {
            Diff = std::make_shared<Difficulty>();
            Diff->Data = std::make_unique<DifficultyLoadInfo>();
            diffSpeedData.clear(); // Clear this in particular since we're using a temp for diffs unlike the rest of the data.
        }

        DoCommonSMCommands(command, CommandContents, Out);

        OnCommand(#OFFSET)
        {
            Offset = latof(CommandContents);
        }

        _OnCommand(#BPMS)
        {
            if (!Diff)
                LoadTimingList(BPMData, line);
            else
                LoadTimingList(Diff->Timing, line);
        }

        _OnCommand(#STOPS)
        {
            if (!Diff)
                LoadTimingList(StopsData, line);
            else
                LoadTimingList(Diff->Data->Stops, line);
        }

        _OnCommand(#BANNER)
        {
            Banner = CommandContents;
        }

        _OnCommand(#WARPS)
        {
            if (!Diff)
                LoadTimingList(WarpsData, CommandContents);
            else
                LoadTimingList(Diff->Data->Warps, CommandContents);
        }

        _OnCommand(#SCROLLS)
        {
            if (!Diff)
                LoadTimingList(ScrollData, CommandContents, true);
            else
                LoadTimingList(Diff->Data->Scrolls, CommandContents, true);
        }

        _OnCommand(#SPEEDS)
        {
            if (!Diff)
                speedData = ParseScrolls(CommandContents);
            else
                diffSpeedData = ParseScrolls(CommandContents);
        }

        // Notedata only here

        if (!Diff) continue;

        _OnCommand(#CREDIT)
        {
            Diff->Author = CommandContents;
        }

        _OnCommand(#METER)
        {
            Diff->Level = atoi(CommandContents.c_str());
        }

        _OnCommand(#DIFFICULTY)
        {
            Diff->Name = CommandContents;
        }

        _OnCommand(#CHARTSTYLE)
        {
            Diff->Name += " " + CommandContents;
        }

        _OnCommand(#STEPSTYPE)
        {
            Diff->Channels = GetTracksByMode(CommandContents);
            if (Diff->Channels == 0)
                Diff = nullptr;
        }

        _OnCommand(#NOTES)
        {
            Diff->BPMType = Difficulty::BT_BEAT;
            if (!Diff->Timing.size())
                Diff->Timing = BPMData;

            if (!Diff->Data->Stops.size())
                Diff->Data->Stops = StopsData;

            if (!Diff->Data->Warps.size())
                Diff->Data->Warps = WarpsData;

            if (!Diff->Data->Scrolls.size())
                Diff->Data->Scrolls = ScrollData;

            if (!diffSpeedData.size())
                Diff->Data->InterpoloatedSpeedMultipliers = CalculateRaindropSpeedData(Diff.get(), speedData);
            else
                Diff->Data->InterpoloatedSpeedMultipliers = CalculateRaindropSpeedData(Diff.get(), diffSpeedData);

            Diff->Offset = -Offset;
            Diff->Duration = 0;
            Diff->Filename = filename;
            Diff->BPMType = Difficulty::BT_BEAT;
            Diff->Data->TimingInfo = std::make_shared<StepmaniaChartInfo>();
            Diff->Data->StageFile = Banner;

            Diff->Data->Warps = CalculateRaindropWarpData(Diff.get(), Diff->Data->Warps);
            Diff->Data->Scrolls = CalculateRaindropScrollData(Diff.get(), Diff->Data->Scrolls);

            CommandContents = RemoveComments(CommandContents);
            auto Measures = Utility::TokenSplit(CommandContents);
            LoadNotesSM(Out, Diff.get(), Measures);
            Out->Difficulties.push_back(Diff);
        }
    }
}
// Convert all negative stops to warps.
void CleanStopsData(Difficulty* Diff)
{
    TimingData tmpWarps;
    TimingData &Stops = Diff->Data->Stops;

    for (auto i = Stops.begin(); i != Stops.end();)
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

    for (auto Warp : tmpWarps)
    {
        double Time = TimeAtBeat(Diff->Timing, Diff->Offset, Warp.Time, true) +
            StopTimeAtBeat(Diff->Data->Stops, Warp.Time);

        TimingSegment New;
        New.Time = Time;
        New.Value = -Warp.Value;
        Diff->Data->Warps.push_back(New);
    }
}

// We need stops to already be warps by this point. Like with the other times, no need to account for the warps themselves.
void WarpifyTiming(Difficulty* Diff)
{
    for (auto i = Diff->Timing.begin(); i != Diff->Timing.end(); ++i)
    {
        if (i->Value < 0)
        {
            auto currentSection = i;

            // for all negative sections between i and the next positive section
            // add up their duration in seconds
            double totalWarpDuration = 0;
            while (currentSection->Value < 0)
            {
                if (currentSection != Diff->Timing.end())
                {
                    // add the duration of section, if there's one to determine it.
                    auto nextSection = currentSection + 1;
                    if (nextSection != Diff->Timing.end())
                    {
						auto sectionDurationBeats = (nextSection->Time - currentSection->Time);
						auto sectionSecondsPerBeat = spb(abs(currentSection->Value));
                        totalWarpDuration += sectionSecondsPerBeat * sectionDurationBeats;
                    }

					/* 
					todo: 
					dtinth's interpretation for sm5 warps 
					infinite bpm, implies we have to split the warp when
					there's a stop.
					*/ 

					// negative bpm means backward scrolling to raindrop
					// therefore, to scroll forwards and warp over this section properly
					// we make it positive
                    currentSection->Value = -currentSection->Value;
                }
                else break;
                ++currentSection;
            }
            
			// diff->timing is in "no-warp" time
			// which means it's in "the time if warps were not considered"
			// or "chart time" used for display
			// the audio time, or time used for judgement
			// is usually behind the chart time.
			// notedata is in chart time.
			// therefore, tracknote time data is in audio time
			// and vertical position is in chart time.
			// chart time is calculated by adding up all the
			// time scrolled by bpm ignoring everything else
			// plus time scrolled by stops 
			// plus time scrolled by warps.
            double warpTime = TimeAtBeat(Diff->Timing, Diff->Offset, i->Time, true) + 
				StopTimeAtBeat(Diff->Data->Stops, i->Time);

            Diff->Data->Warps.push_back(TimingSegment(warpTime, totalWarpDuration * 2));
            // And now that we're done, there's no need to check the negative BPMs inbetween this one and the next positive BPM, so...
            i = currentSection;
            if (i == Diff->Timing.end()) break;
        }
    }
}

void NoteLoaderSM::LoadObjectsFromFile(std::filesystem::path filename, Song *Out)
{
	CreateIfstream(filein, filename);

    TimingData BPMData;
    TimingData StopsData;
    double Offset = 0;

    std::shared_ptr<Difficulty> Diff = std::make_shared<Difficulty>();

    // Stepmania uses beat-based locations for stops and BPM.
    Diff->BPMType = Difficulty::BT_BEAT;

    if (!filein.is_open())
        throw std::runtime_error(Utility::Format("couldn't open %s for reading", filename.string().c_str()).c_str());

    std::string Banner;

    Diff->Offset = 0;
    Diff->Duration = 0;
    Diff->Data = std::make_unique<DifficultyLoadInfo>();

    std::string line;
    while (filein)
    {
        std::getline(filein, line, ';');

        if (line.length() < 3)
            continue;

        std::string command;
        size_t iHash = line.find_first_of("#");
        size_t iColon = line.find_first_of(":");
        if (iHash != std::string::npos && iColon != std::string::npos)
            command = line.substr(iHash, iColon - iHash);
        else
            continue;

        Utility::ReplaceAll(command, "\n", "");

        std::string CommandContents = line.substr(line.find_first_of(":") + 1);

        DoCommonSMCommands(command, CommandContents, Out);

        OnCommand(#OFFSET)
        {
            Offset = latof(CommandContents);
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
            Diff->Data = std::make_unique<DifficultyLoadInfo>();
            Diff->Data->Stops = StopsData;
            Diff->Offset = -Offset;
            Diff->Duration = 0;
            Diff->Filename = filename;
            Diff->BPMType = Difficulty::BT_BEAT;
            Diff->Data->TimingInfo = std::make_shared<StepmaniaChartInfo>();
            Diff->Data->StageFile = Banner;
            CleanStopsData(Diff.get());
            WarpifyTiming(Diff.get());

            if (LoadTracksSM(Out, Diff.get(), line))
            {
                Out->Difficulties.push_back(Diff);
                Diff = std::make_shared<Difficulty>();
            }
        }
    }
}
