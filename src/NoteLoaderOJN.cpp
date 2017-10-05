#include "pch.h"


#include "Logging.h"
#include "Song7K.h"
#include "NoteLoader7K.h"

//#define lilswap(x) x=((x>>24)&0xFF)|((x<<8)&0xFF0000)|((x<<24)&0xFF00)|((x<<24)&0xFF000000)

// Anything below 8 is a note channel.
#define BPM_CHANNEL 10
#define AUTOPLAY_CHANNEL 9

const char *DifficultyNames[] = { "EX", "NX", "HX" };

// based from the ojn documentation at
// http://open2jam.wordpress.com/the-ojn-documentation/

struct OjnHeader
{
    int32_t songid;
    char signature[4];
    float encode_version;
    int32_t genre;
    float bpm;
    int16_t level[4];
    int32_t event_count[3];
    int32_t note_count[3];
    int32_t measure_count[3];
    int32_t package_count[3];
    int16_t old_encode_version;
    int16_t old_songid;
    char old_genre[20];
    int32_t bmp_size;
    int32_t old_file_version;
    char title[64];
    char artist[32];
    char noter[32];
    char ojm_file[32];
    int32_t cover_size;
    int32_t time[3];
    int32_t note_offset[3];
    int32_t cover_offset;
};

struct OjnPackageHeader
{
    int measure;
    short channel;
    short events;
};

union OjnPackage
{
    struct
    {
        short noteValue;
        char volume_pan;
        char type;
    };
    float floatValue;
};

struct OjnExpandedPackage
{
    int Channel;
    int noteKind; // undefined if channel is not note or autoplay channel
    float Fraction;
    union
    {
        float fValue;
        int iValue;
    };

	bool operator <(const OjnExpandedPackage& other) {
		return Fraction < other.Fraction;
	}
};

struct OjnMeasure
{
    float Len;

    std::vector<OjnExpandedPackage> Events;

    OjnMeasure()
    {
        Len = 4;
    }
};

class OjnLoadDifficultyContext
{
private:
	double BeatForMeasure(int Measure)
	{
		double Out = 0;

		for (int i = 0; i < Measure; i++)
		{
			Out += Measures[i].Len;
		}

		return Out;
	}

	std::vector<OjnMeasure> Measures;
public:
    Game::VSRG::Song* S;
    float BPM;

	void ReadPackages(OjnHeader &Head, int difficulty_index, std::fstream &ojnfile)
	{
		// Reserve measures.
		Measures.resize(Head.measure_count[difficulty_index] + 1);

		for (size_t package = 0U; package < Head.package_count[difficulty_index]; ++package)
		{
			OjnPackageHeader PackageHeader;
			ojnfile.read(reinterpret_cast<char*>(&PackageHeader), sizeof(OjnPackageHeader));

			for (size_t event_index = 0U; event_index < PackageHeader.events; ++event_index)
			{
				auto Fraction = float(event_index) / float(PackageHeader.events);
				OjnPackage package;
				OjnExpandedPackage o2evt;

				ojnfile.read(reinterpret_cast<char*>(&package), sizeof(OjnPackage));

				// this happens sometimes, oddly -az
				if (PackageHeader.measure >= Measures.size())
				{
					Measures.resize(PackageHeader.measure + 1);
				}

				switch (PackageHeader.channel)
				{
				case 0: // Fractional measure
					Measures[PackageHeader.measure].Len = 4 * package.floatValue;
					break;
				case 1: // BPM change
					o2evt.fValue = package.floatValue;
					o2evt.Channel = BPM_CHANNEL;
					o2evt.Fraction = Fraction;
					Measures[PackageHeader.measure].Events.push_back(o2evt);
					break;
				case 2: // note events (enginechannel = PackageHeader.channel - 2)
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
					if (package.noteValue == 0) continue;
					o2evt.Fraction = Fraction;
					o2evt.Channel = PackageHeader.channel - 2;
					o2evt.iValue = package.noteValue;
					o2evt.noteKind = package.type;
					Measures[PackageHeader.measure].Events.push_back(o2evt);
					break;
				default: // autoplay notes
					if (package.noteValue == 0) continue;
					o2evt.Channel = AUTOPLAY_CHANNEL;
					o2evt.Fraction = Fraction;
					o2evt.iValue = package.noteValue;
					o2evt.noteKind = package.type;
					Measures[PackageHeader.measure].Events.push_back(o2evt);
					break;
				}
			}
		}
	}


	void FixOJNEvents()
	{
		auto CurrentMeasure = 0;
		OjnExpandedPackage prevIter[7] = { -1, -1, -1, -1 };

		for (auto &Measure : Measures)
		{
			// Sort events. This is very important, since we assume events are sorted!
			std::sort(Measure.Events.begin(), Measure.Events.end());

			for (auto Evt = Measure.Events.begin(); Evt != Measure.Events.end(); ++Evt)
			{
				if (Evt->Channel < AUTOPLAY_CHANNEL)
				{
					if (prevIter[Evt->Channel].Channel != -1) // There is a previous event
					{
						if (prevIter[Evt->Channel].noteKind == 2 &&
							(Evt->noteKind == 0 || Evt->noteKind == 2)) // This note or hold head is in between holds
						{
							Evt->Channel = AUTOPLAY_CHANNEL;
							Evt->noteKind = 0;
							continue;
						}

						if (prevIter[Evt->Channel].noteKind != 2 && // Hold tail without ongoing hold
							Evt->noteKind == 3)
						{
							Evt->Channel = AUTOPLAY_CHANNEL;
							Evt->noteKind = 0;
							continue;
						}
					}

					prevIter[Evt->Channel] = *Evt;
				}
			}

			CurrentMeasure++;
		}
	}

	void CopyOJNTimingData(Game::VSRG::Difficulty *Out)
	{
		auto CurrentMeasure = 0;
		for (auto Measure : Measures)
		{
			float MeasureBaseBeat = BeatForMeasure(CurrentMeasure);

			Out->Data->Measures.push_back(Game::VSRG::Measure());

			// All fractional measure events were already handled at read time.
			Out->Data->Measures[CurrentMeasure].Length = Measure.Len;

			for (auto Evt : Measure.Events)
			{
				if (Evt.Channel != BPM_CHANNEL) continue; // These are the only ones we directly handle.

														  /* We calculate beat multiplying fraction by 4 instead of the measure's length,
														  because o2jam's measure fractions don't compress the notes inside. */
				auto Beat = MeasureBaseBeat + Evt.Fraction * 4;
				TimingSegment Segment(Beat, Evt.fValue);

				// 0 values must be ignored.
				if (Evt.fValue == 0) continue;

				if (Out->Timing.size())
				{
					// For some reason, a few BPMs are redundant. Since our events are sorted, there's no need for worry..
					if (Out->Timing.back().Value == Evt.fValue) // ... We already have this BPM.
						continue;
				}

				Out->Timing.push_back(Segment);
			}

			CurrentMeasure++;
		}

		// The BPM info on the header is not for decoration. It's the very first BPM we should be using.
		// A few of the charts already have set BPMs at beat 0, so we only need to add information if it's missing.
		if (Out->Timing.size() == 0 || Out->Timing[0].Time > 0)
		{
			TimingSegment Seg(0, BPM);
			Out->Timing.push_back(Seg);

			// Since events and measures are ordered already, there's no need to sort
			// timing data unless we insert new information.
			sort(Out->Timing.begin(), Out->Timing.end(), TimeSegmentCompare<TimingSegment>);
		}
	}

	// Based off the O2JAM method at
	// https://github.com/open2jamorg/open2jam/blob/master/parsers/src/org/open2jam/parsers/EventList.java
	void OutputAllOJNEventsToDifficulty(Game::VSRG::Difficulty *Out)
	{
		// First, we sort and clear up invalid events.
		FixOJNEvents();

		// Then we need to have just as many measures going out as we've got in here.
		Out->Data->Measures.reserve(Measures.size());

		// Now to output, we need to process BPM changes and fractional measures.
		CopyOJNTimingData(Out);

		// Now, we can process notes and long notes.
		CopyOJNNoteData(Out);
	}

	void CopyOJNNoteData(Game::VSRG::Difficulty * Out)
	{
		auto CurrentMeasure = 0;
		float PendingLNs[7] = { 0 };
		float PendingLNSound[7] = { 0 };

		for (auto Measure : Measures)
		{
			auto MeasureBaseBeat = BeatForMeasure(CurrentMeasure);

			for (auto Evt : Measure.Events)
			{
				if (Evt.Channel == BPM_CHANNEL) continue;
				else
				{
					auto Beat = MeasureBaseBeat + Evt.Fraction * 4;
					auto Time = TimeAtBeat(Out->Timing, 0, Beat);

					if (Evt.noteKind % 8 > 3) // Okay... This is obscure. Big thanks to open2jam.
						Evt.iValue += 1000;

					if (Evt.Channel == AUTOPLAY_CHANNEL) // Ah, autoplay audio.
					{
						AutoplaySound Snd;

						Snd.Sound = Evt.iValue;
						Snd.Time = Time;
						Out->Data->BGMEvents.push_back(Snd);
					}
					else // A note! In this case, we already 'normalized' O2Jam channels into raindrop channels.
					{
						Game::VSRG::NoteData Note;

						if (Evt.Channel >= 7) continue; // Who knows... A buffer overflow may be possible.

						Note.StartTime = Time;
						Note.Sound = Evt.iValue;

						switch (Evt.noteKind)
						{
						case 0:
							Out->Data->Measures[CurrentMeasure].Notes[Evt.Channel].push_back(Note);
							break;
						case 2:
							PendingLNs[Evt.Channel] = Time;
							PendingLNSound[Evt.Channel] = Evt.iValue;
							break;
						case 3:
							Note.StartTime = PendingLNs[Evt.Channel];
							Note.EndTime = Time;
							Note.Sound = PendingLNSound[Evt.Channel];
							Out->Data->Measures[CurrentMeasure].Notes[Evt.Channel].push_back(Note);
							break;
						}
					}
				}
			}
			CurrentMeasure++;
		}
	}
};




bool IsValidOJN(std::fstream &filein, OjnHeader *Head)
{
    filein.read(reinterpret_cast<char*>(Head), sizeof(OjnHeader));

    if (!filein)
    {
        if (filein.eofbit)
            Log::Printf("NoteLoaderOJN: EOF reached before header could be read\n");
        return false;
    }

    if (strcmp(Head->signature, "ojn"))
    {
        Log::Printf("NoteLoaderOJN: %s is not an ojn file.\n");
        return false;
    }

    return true;
}

const char *LoadOJNCover(std::filesystem::path filename, size_t &read)
{
    std::fstream filein(filename.string(), std::ios::binary | std::ios::in);
    OjnHeader Head;
    char* out;

	if (!filein)
	{
		Log::Printf("NoteLoaderOJN: %s could not be opened\n", filename.c_str());
		return 0;
	}

    if (!IsValidOJN(filein, &Head))
        return "";

    out = new char[Head.cover_size];

    filein.seekg(Head.cover_offset, std::ios::beg);
    filein.read(out, Head.cover_size);
    read = Head.cover_size;

    return out;
}


void NoteLoaderOJN::LoadObjectsFromFile(std::filesystem::path filename, Game::VSRG::Song *Out)
{
	CreateBinIfstream(filein, filename);

    OjnHeader Head;
	auto ufn = Utility::ToU8(filename.wstring());

	if (!filein)
	{
		auto s = Utility::Format("NoteLoaderOJN: %s could not be opened\n", ufn.c_str());
		throw std::runtime_error(s);
	}

	if (!IsValidOJN(filein, &Head))
	{
		auto s = Utility::Format("NoteLoaderOJN: %s is not a valid OJN.\n", ufn.c_str());
		throw std::runtime_error(s);
	}

    std::string vArtist;
    std::string vName;
    std::string Noter;
    /*
        These are the only values we display, so we should clean them up so that nobody cries.
        Of course, the right thing to do would be to iconv these, but unless
        I implement some way of detecting encodings, this is the best we can do in here.
    */
    utf8::replace_invalid(Head.artist, Head.artist + 32, std::back_inserter(vArtist));
    utf8::replace_invalid(Head.title, Head.title + 64, std::back_inserter(vName));
    utf8::replace_invalid(Head.noter, Head.noter + 32, std::back_inserter(Noter));

    Out->SongAuthor = vArtist;
    Out->SongName = vName;
    Out->SongFilename = Head.ojm_file;

    for (auto i = 0; i < 3; i++)
    {
        OjnLoadDifficultyContext Info;
        std::shared_ptr<Game::VSRG::Difficulty> Diff(new Game::VSRG::Difficulty());
        std::shared_ptr<Game::VSRG::O2JamChartInfo> TInfo(new Game::VSRG::O2JamChartInfo);
        std::shared_ptr<Game::VSRG::DifficultyLoadInfo> LInfo(new Game::VSRG::DifficultyLoadInfo);

        switch (i)
        {
        case 0:
            TInfo->Difficulty = Game::VSRG::O2JamChartInfo::O2_EX;
            break;
        case 1:
            TInfo->Difficulty = Game::VSRG::O2JamChartInfo::O2_NX;
            break;
        case 2:
            TInfo->Difficulty = Game::VSRG::O2JamChartInfo::O2_HX;
            break;
        }

        Diff->Data = LInfo;
        Diff->Level = Head.level[i];
        Diff->Data->TimingInfo = TInfo;
        Diff->Author = Noter;
        Diff->Data->StageFile = Utility::ToU8(filename.filename().wstring());

        Info.S = Out;
        filein.seekg(Head.note_offset[i]);

        // O2Jam files use Beat-Based notation.
        Diff->BPMType = Game::VSRG::Difficulty::BT_BEAT;
        Diff->Duration = Head.time[i];
        Diff->Name = DifficultyNames[i];
        Diff->Channels = 7;
        Diff->Filename = filename;
        Diff->IsVirtual = true;
        Info.BPM = Head.bpm;

        /*
            The implications of this structure are interesting.
            Measures may be unordered; but events may not, if only there's one package per channel per measure.
        */
		Info.ReadPackages(Head, i, filein);

        // Process Info... then push back difficulty.
        Info.OutputAllOJNEventsToDifficulty(Diff.get());
        Out->Difficulties.push_back(Diff);
    }
}

