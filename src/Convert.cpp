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
			<< "CircleSize: " << (*i)->Channels << "\n"
			<< "OverallDifficulty: 7\n"
			<< "ApproachRate: 9\n"
			<< "SliderMultiplier: 1.4\n"
			<< "SliderTickRate: 1\n\n"
			<< "[Events]\n"
			<< "// Background and Video events\n"
			<< "0,0,\"" << Sng->BackgroundFilename << "\""
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
			out << t->Time << "," << 1000 / (t->Value ? t->Value : 0.00001) << ",4,1,0,15,0,0\n";
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
					out << TrackToXPos((*i)->Channels, n) << ",192," << Note->StartTime * 1000 << ","
						<< (Note->EndTime ? "128" : "1") << ",0,";
					if (Note->EndTime)
						out << Note->EndTime * 1000 << ":";
					out << "1:0:0:0:" << 
						( Note->Sound ? ((*i)->SoundList.find(Note->Sound) != (*i)->SoundList.end() ? (*i)->SoundList[Note->Sound].c_str() : "" ) : "") << "\n";
					out.flush();
				}
			}
		}
	}
}