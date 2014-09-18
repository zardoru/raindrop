#include <fstream>
#include <map>

#include "Global.h"
#include "Directory.h"
#include "Song7K.h"
#include "Converter.h"

int TrackToXPos(int totaltracks, int track)
{
	double base = (512.0/totaltracks);
	double minus = (256.0/totaltracks);
	return (base * (track + 1) - minus);
}

void ConvertToOM(VSRG::Song *Sng, Directory PathOut, String Author)
{
	for (std::vector<VSRG::Difficulty*>::iterator i = Sng->Difficulties.begin(); 
		i != Sng->Difficulties.end();
		i++)
	{
		std::stringstream ss;

		ss << PathOut.path() << "/" << Sng->SongAuthor << " - " << Sng->SongName << " [" << (*i)->Name << "] (" << Author << ").osu";
		std::ofstream out (ss.str().c_str());

		Sng->Process(*i, NULL);

		// First, convert metadata.
		out 
			<< "osu file format v11\n\n"
			<< "[General]\n"
			<< "AudioFilename: " << ((*i)->IsVirtual ? "virtual" : Sng->SongFilename) << "\n"
			<< "AudioLeadIn: 1500\n"
			<< "PreviewTime: " << (*i)->PreviewTime << "\n"
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
			<< "Version: " << (*i)->Name << "\n"
			<< "Source: \nTags: \nBeatmapID:0\nBeatmapSetID:-1\n\n";

		out.flush();

		out
			<< "[Difficulty]\n"
			<< "HPDrainRate: 7\n"
			<< "CircleSize: " << (int)(*i)->Channels << "\n"
			<< "OverallDifficulty: 7\n"
			<< "ApproachRate: 9\n"
			<< "SliderMultiplier: 1.4\n"
			<< "SliderTickRate: 1\n\n"
			<< "[Events]\n"
			<< "// Background and Video events\n"
			<< "0,0,\"" << Sng->BackgroundFilename << "\"\n"
			<< "// Storyboard Layer 0 (Background)\n// Storyboard Layer 1 (Fail)\n// Storyboard Layer 2 (Pass)\n"
			<< "// Storyboard Layer 3 (Foreground)\n// Storyboard Sound Samples\n// Background Colour Transformations\n"
			<< "3,100,163,162,255\n\n";

		out.flush();

		// Then, timing points.
		out << "[TimingPoints]\n";


		for (TimingData::iterator t = (*i)->BPS.begin(); 
			t != (*i)->BPS.end();
			t++)
		{
			out << t->Time * 1000 << "," << 1000 / (t->Value ? t->Value : 0.00001) << ",4,1,0,15,1,0\n";
			out.flush();
		}

		out << "\n\n[HitObjects]\n";

		// Then, objects.
		for (VSRG::MeasureVector::iterator k = (*i)->Measures.begin();
			k != (*i)->Measures.end();
			k++)
		{
			for (uint8 n = 0; n < (*i)->Channels; n++)
			{
				for (std::vector<VSRG::NoteData>::iterator Note = k->MeasureNotes[n].begin();
					Note != k->MeasureNotes[n].end();
					Note++)
				{
					out << TrackToXPos((*i)->Channels, n) << ",0," << int(Note->StartTime * 1000.0) << ","
						<< (Note->EndTime ? "128" : "1") << ",0,";
					if (Note->EndTime)
						out << int(Note->EndTime * 1000.0) << ",";
					out << "1:0:0";

					if (Note->Sound)
						out << ":" << ( Note->Sound ? ((*i)->SoundList.find(Note->Sound) != (*i)->SoundList.end() ? (*i)->SoundList[Note->Sound].c_str() : "" ) : "");

					out << "\n";
					out.flush();
				}
			}
		}
	}
}

void ConvertToSMTiming(VSRG::Song *Sng, Directory PathOut)
{
	std::stringstream ss;

	VSRG::Difficulty* Diff = Sng->Difficulties[0];
	Sng->Process (Diff, NULL);

	std::ofstream out (PathOut.c_path());

	// Technically, stepmania's #OFFSET is actually #GAP, not #OFFSET.
	out << "#OFFSET:" << -Diff->Offset << ";\n";

	out << "#BPMS:";

	for (TimingData::iterator i = Diff->Timing.begin();
		i != Diff->Timing.end();
		i++)
	{
		double Time;
		double Value;
		switch (Diff->BPMType)
		{
		case VSRG::Difficulty::BT_Beat:
			Time = i->Time;
			Value = i->Value;
			break;
		case VSRG::Difficulty::BT_Beatspace:
			Time = i->Time;
			Value = 60000 / i->Value;
			break;
		case VSRG::Difficulty::BT_MS:
			Time = i->Time / 1000.0;
			Value = i->Value;
			break;
		}

		double Beat = QuantizeBeat(BeatAtTime(Diff->BPS, Time + Diff->Offset, 0));

		out << Beat << "=" << Value;

		if ( (i+1) != Diff->Timing.end() ) // Only output comma if there's still stuff to output.
			out << "\n,";
	}

	out << ";";
}