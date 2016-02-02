#include "pch.h"

#include "GameGlobal.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

using namespace VSRG;

/*
    This is pretty much the simplest possible loader.

    The important part is: all notes should contain a start time, possibly an end time (both in seconds)
    and you require at least a single BPM to calculate speed properly. And that's it, set the duration
    of the chart and the loading is done.
*/

void NoteLoaderFTB::LoadMetadata(std::string filename, std::string prefix, Song *Out)
{
#if (!defined _WIN32) || (defined STLP)
    std::ifstream filein(filename.c_str());
#else
    std::ifstream filein(Utility::Widen(filename).c_str());
#endif

    if (!filein.is_open())
        return;

    std::string Author;
    std::string Title;
    std::string musName;

    getline(filein, musName);
    getline(filein, Title);
    getline(filein, Author);

    Out->SongFilename = musName;
    Out->SongName = Title;
    Out->SongAuthor = Author;
    Out->SongDirectory = prefix + "/";

    filein.close();
}

void NoteLoaderFTB::LoadObjectsFromFile(std::string filename, std::string prefix, Song *Out)
{
    std::shared_ptr<VSRG::Difficulty> Diff(new Difficulty());
    Measure Msr;

#if (!defined _WIN32) || (defined STLP)
    std::ifstream filein(filename.c_str());
#else
    std::ifstream filein(Utility::Widen(filename).c_str());
#endif

    Diff->Filename = filename;

    if (!filein.is_open())
    {
    failed:
        return;
    }

    Diff->BPMType = VSRG::Difficulty::BT_MS; // MS using BPMs.
    Diff->Channels = 7;
    Diff->Name = Utility::RemoveExtension(Utility::RelativeToPath(filename));

    while (filein)
    {
        std::string Line;
        std::getline(filein, Line);

        if (Line[0] == '#' || Line.length() == 0)
            continue;

        auto LineContents = Utility::TokenSplit(Line, " ");

        if (LineContents.at(0) == "BPM")
        {
            TimingSegment Seg;
            Seg.Time = latof(LineContents[1].c_str()) / 1000.0;
            Seg.Value = latof(LineContents[2].c_str());
            Diff->Timing.push_back(Seg);
        }
        else
        {
            /* We'll make a few assumptions about the structure from now on
                ->The vertical speeds determine note position, not measure
                ->More than one measure is unnecessary when using BT_MS,
                    so we'll use a single measure for all the song, containing all notes.
            */

            NoteData Note;
            auto NoteInfo = Utility::TokenSplit(LineContents.at(0), "-");
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

            Diff->Duration = std::max(std::max(Note.StartTime, Note.EndTime), Diff->Duration);
            Msr.Notes[Track - 1].push_back(Note);
        }
    }

    filein.close();

    // Offsetize
    if (Diff->Timing.size())
    {
        Diff->Offset = Diff->Timing.begin()->Time;

        for (auto i : Diff->Timing)
            i.Time -= Diff->Offset;
    }
    else
        goto failed;

    Diff->Level = Diff->TotalScoringObjects / Diff->Duration;
    Out->Difficulties.push_back(Diff);
}