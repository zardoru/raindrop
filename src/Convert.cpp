#include "pch.h"

#include "GameGlobal.h"
#include "Directory.h"
#include "Song7K.h"
#include "Converter.h"
#include "Logging.h"

int TrackToXPos(int totaltracks, int track)
{
	auto base = (512.0/totaltracks);
	auto minus = (256.0/totaltracks);
	return (base * (track + 1) - minus);
}

void ConvertToOM(VSRG::Song *Sng, Directory PathOut, GString Author)
{
	for (auto Difficulty : Sng->Difficulties)
	{
		char vf[1024];
		TimingData BPS;
		TimingData VSpeeds, Warps;
		GString Author = Sng->SongAuthor;
		GString Name  = Sng->SongName;
		GString DName = Difficulty->Name;
		GString Charter = Author;

		Utility::RemoveFilenameIllegalCharacters(Author, true);
		Utility::RemoveFilenameIllegalCharacters(Name, true);
		Utility::RemoveFilenameIllegalCharacters(DName, true);
		Utility::RemoveFilenameIllegalCharacters(Charter, true);

		Directory Str = Utility::Format("%s/%s - %s [%s] (%s).osu", PathOut.c_path(), Author.c_str(), Name.c_str(), DName.c_str(), Charter.c_str());
		Str.Normalize(true);
		std::ofstream out (Str.c_path());

		if (!out.is_open())
		{
			Log::Printf("Unable to open file %s for writing.\n", vf);
			return;
		}

		Difficulty->GetPlayableData(nullptr, BPS, VSpeeds, Warps, 0);

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
			<< "Title: " << Sng->SongName << "\n"
			<< "TitleUnicode: " << Sng->SongName << "\n"
			<< "Artist: " << Sng->SongAuthor << "\n"
			<< "ArtistUnicode: " << Sng->SongAuthor << "\n"
			<< "Creator: " << Author << "\n"
			<< "Version: " << Difficulty->Name << "\n"
			<< "Source: \nTags: \nBeatmapID:0\nBeatmapSetID:-1\n\n";

		out.flush();

		out
			<< "[Difficulty]\n"
			<< "HPDrainRate: 7\n"
			<< "CircleSize: " << int(Difficulty->Channels) << "\n"
			<< "OverallDifficulty: 7\n"
			<< "ApproachRate: 9\n"
			<< "SliderMultiplier: 1.4\n"
			<< "SliderTickRate: 1\n\n"
			<< "[Events]\n"
			<< "// Background and Video events\n"
			<< "0,0,\"" << Sng->BackgroundFilename << "\"\n"
			<< "// Storyboard Layer 0 (Background)\n// Storyboard Layer 1 (Fail)\n// Storyboard Layer 2 (Pass)\n"
			<< "// Storyboard Layer 3 (Foreground)\n// Storyboard Sound Samples\n";
			
		// Write BGM events here.
		for (auto BGM : Difficulty->Data->BGMEvents) {
			auto sndf = Difficulty->SoundList[BGM.Sound];
			out << "5," << int(round(BGM.Time * 1000)) << ",0,\"" << sndf << "\",100" << std::endl;
		}

		out
			<< "// Background Colour Transformations\n"
			<< "3,100,163,162,255\n\n";

		out.flush();

		// Then, timing points.
		out << "[TimingPoints]\n";


		for (auto t : BPS)
		{
			out << t.Time * 1000 << "," << 1000 / (t.Value ? t.Value : 0.00001) << ",4,1,0,15,1,0\n";
			out.flush();
		}

		for (auto t : Difficulty->Data->Scrolls)
		{
			out << (t.Time + Difficulty->Offset) * 1000 << "," << - 100/(t.Value ? t.Value : 0.00001) << ",4,1,0,15,0,0\n";
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
					if (Difficulty->IsWarpingAt(Note.StartTime)) continue;

					out << TrackToXPos(Difficulty->Channels, n) << ",0," << int(Note.StartTime * 1000.0) << ","
						<< (Note.EndTime ? "128" : "1") << ",0,";
					if (Note.EndTime)
						out << int(Note.EndTime * 1000.0) << ",";
					out << "1:0:0:0:";

					if (Note.Sound)
						out << (Difficulty->SoundList.find(Note.Sound) != Difficulty->SoundList.end() ? Difficulty->SoundList[Note.Sound] : "" );

					out << "\n";
					out.flush();
				}
			}
		}
	}
}

void ConvertToSMTiming(VSRG::Song *Sng, Directory PathOut)
{
	TimingData BPS, VSpeeds, Warps;
	VSRG::Difficulty* Diff = Sng->Difficulties[0].get();
	Diff->GetPlayableData (nullptr, BPS, VSpeeds, Warps);

	std::ofstream out (PathOut.c_path());

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
		case VSRG::Difficulty::BT_BEAT:
			Time = i->Time;
			Value = i->Value;
			break;
		case VSRG::Difficulty::BT_BEATSPACE:
			Time = i->Time;
			Value = 60000 / i->Value;
			break;
		case VSRG::Difficulty::BT_MS:
			Time = i->Time / 1000.0;
			Value = i->Value;
			break;
		}

		double Beat = QuantizeBeat(IntegrateToTime(BPS, Time + Diff->Offset, 0));

		out << Beat << "=" << Value;

		if ( (i+1) != Diff->Timing.end() ) // Only output comma if there's still stuff to output.
			out << "\n,";
	}

	out << ";";
}

void ExportToBMS(VSRG::Song *Sng, Directory PathOut);
void ConvertToBMS(VSRG::Song *Sng, Directory PathOut) {
	ExportToBMS(Sng, PathOut);
}