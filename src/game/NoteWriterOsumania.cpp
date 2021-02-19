#include <fstream>
#include <rmath.h>
#include <game/Song.h>
#include <game/PlayerChartState.h>
#include "TextAndFileUtil.h"


class path;

int TrackToXPos(int totaltracks, int track)
{
    auto base = (512.0 / totaltracks);
    auto minus = (256.0 / totaltracks);
    return (base * (track + 1) - minus);
}

void ConvertToOM(rd::Song *Sng, std::filesystem::path PathOut, std::string Author)
{
	// Log::LogPrintf("Attempt to convert %d difficulties...\n", Sng->Difficulties.size());
    for (auto &Difficulty : Sng->Difficulties)
    {
        std::string Name = Sng->Title;
        std::string DName = Difficulty->Name;
        std::string Charter = Author;

        Utility::RemoveFilenameIllegalCharacters(Author, true);
        Utility::RemoveFilenameIllegalCharacters(Name, true);
        Utility::RemoveFilenameIllegalCharacters(DName, true);
        Utility::RemoveFilenameIllegalCharacters(Charter, true);

		std::filesystem::path Str = 
			PathOut / Utility::Format("%s - %s [%s] (%s).osu",
									  Author.c_str(), 
									  Name.c_str(), 
									  DName.c_str(), 
									  Charter.c_str());

		// Log::Printf("Converting into file %s...\n", Str.string().c_str());
        std::ofstream out(Str.string());

        if (!out.is_open())
        {
            // Log::Printf("Unable to open file %s for writing.\n", Str.string().c_str());
            return;
        }

		rd::PlayerChartState data = rd::PlayerChartState::FromDifficulty(Difficulty.get());

        // First, convert metadata.
        out
            << "osu file format v11\n\n"
            << "[General]\n"
            << "AudioFilename: " << (Difficulty->IsVirtual ? "virtual" : Sng->SongFilename) << "\n"
            << "AudioLeadIn: 1500\n"
            << "PreviewTime: " << int(Sng->PreviewTime) * 1000 << "\n"
            << "Countdown: 0\n"
            << "SampleSet: None\n"
            << "StackLeniency: 0.7\n"
            << "SpecialStyle: 0\n"
            << "WidescreenStoryboard: 0\n"
            << "Mode: 3\n"
            << "LetterboxInBreaks: 0\n\n"
            << "[Editor]\n"
            << "DistanceSnapping: 0.9\n"
            << "BeatDivisor: 4\n"
            << "GridSize: 16\n"
            << "CurrentTime: 0\n\n";

        out.flush();

        out
            << "[Metadata]\n"
            << "Title: " << Sng->Title << "\n"
            << "TitleUnicode: " << Sng->Title << "\n"
            << "Artist: " << Sng->Artist << "\n"
            << "ArtistUnicode: " << Sng->Artist << "\n"
            << "Creator: " << Author << "\n"
            << "Version: " << Difficulty->Name << "\n"
            << "Source: \nTags: \nBeatmapID:0\nBeatmapSetID:-1\n\n";

        out.flush();

        out
            << "[Difficulty]\n"
            << "HPDrainRate: 8\n"
            << "CircleSize: " << int(Difficulty->Channels) << "\n"
            << "OverallDifficulty: 8\n"
            << "ApproachRate: 9\n"
            << "SliderMultiplier: 1.4\n"
            << "SliderTickRate: 1\n\n"
            << "[Events]\n"
            << "// Background and Video events\n"
            << "0,0,\"" << Sng->BackgroundFilename << "\"\n"
            << "// Storyboard Layer 0 (Background)\n// Storyboard Layer 1 (Fail)\n// Storyboard Layer 2 (Pass)\n"
            << "// Storyboard Layer 3 (Foreground)\n// Storyboard Sound Samples\n";

        // Write BGM events here.
        for (auto BGM : Difficulty->Data->BGMEvents)
        {
            auto sndf = Difficulty->Data->SoundList[BGM.Sound];
            out << "5," << int(round(BGM.Time * 1000)) << ",0,\"" << sndf << "\",100" << std::endl;
        }

        out
            << "// Background Colour Transformations\n"
            << "3,100,163,162,255\n\n";

        out.flush();

        // Then, timing points.
        out << "[TimingPoints]\n";

        for (auto t : data.BPS)
        {
            out << t.Time * 1000 << "," << 1000 / (t.Value ? t.Value : 0.00001) << ",4,1,0,15,1,0\n";
            out.flush();
        }

        for (auto t : Difficulty->Data->Scrolls)
        {
            out << (t.Time + Difficulty->Offset) * 1000 << "," << -100 / (t.Value ? t.Value : 0.00001) << ",4,1,0,15,0,0\n";
            out.flush();
        }

        out << "\n\n[HitObjects]\n";

        // Then, objects.
        for (auto k : Difficulty->Data->Measures)
        {
            for (auto n = 0U; n < Difficulty->Channels; n++)
            {
                for (auto Note : k.Notes[n])
                {
                    if (data.IsWarpingAt(Note.StartTime)) continue;

                    out << TrackToXPos(Difficulty->Channels, n) << ",0," << int(round(Note.StartTime * 1000.0)) << ","
                        << (Note.EndTime ? "128" : "1") << ",0,";
                    if (Note.EndTime)
                        out << int(round(Note.EndTime * 1000.0)) << ",";
					// "sampleSet:additionSet:customIndex:sampleVolume:filename"
					out << "1:0:0:";
					

					if (Note.Sound)
					{
						out << "100:";
						out << (Difficulty->Data->SoundList.find(Note.Sound) != Difficulty->Data->SoundList.end() ?
							Difficulty->Data->SoundList[Note.Sound] : "");
					}
					else {
						out << "0:";
					}

                    out << "\n";
                    out.flush();
                }
            }
        }
    }
}

void ConvertToSMTiming(rd::Song *Sng, std::filesystem::path PathOut)
{
    rd::Difficulty* Diff = Sng->Difficulties[0].get();
	rd::PlayerChartState data = rd::PlayerChartState::FromDifficulty(Diff);

    std::ofstream out(PathOut.string());
    // Technically, stepmania's #OFFSET is actually #GAP, not #OFFSET.
    out << "#OFFSET:" << -Diff->Offset << ";\n";

    out << "#BPMS:";

    for (auto i = Diff->Timing.begin();
    i != Diff->Timing.end();
        ++i)
    {
        double Time = 0;
        double Value = 0;
        switch (Diff->BPMType)
        {
            case rd::Difficulty::BT_BEAT:
            Time = i->Time;
            Value = i->Value;
            break;
        case rd::Difficulty::BT_BEATSPACE:
            Time = i->Time;
            Value = 60000 / i->Value;
            break;
        case rd::Difficulty::BT_MS:
            Time = i->Time / 1000.0;
            Value = i->Value;
            break;
        }

        double Beat = QuantizeBeat(IntegrateToTime(data.BPS, Time + Diff->Offset));

        out << Beat << "=" << Value;

        if ((i + 1) != Diff->Timing.end()) // Only output comma if there's still stuff to output.
            out << "\n,";
    }

    out << ";";
}

void ExportToBMS(rd::Song *Sng, std::filesystem::path PathOut);
void ConvertToBMS(rd::Song *Sng, std::filesystem::path PathOut)
{
    ExportToBMS(Sng, PathOut);
}
