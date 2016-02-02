#include "pch.h"

#include "utf8.h"
#include "GameGlobal.h"
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
	int32 songid;
	char signature[4];
	float encode_version;
	int32 genre;
	float bpm;
	int16 level[4];
	int32 event_count[3];
	int32 note_count[3];
	int32 measure_count[3];
	int32 package_count[3];
	int16 old_encode_version;
	int16 old_songid;
	char old_genre[20];
	int32 bmp_size;
	int32 old_file_version;
	char title[64];
	char artist[32];
	char noter[32];
	char ojm_file[32];
	int32 cover_size;
	int32 time[3];
	int32 note_offset[3];
	int32 cover_offset;
};

struct OjnPackage
{
	int measure;
	short channel;
	short events;
};

union OjnEvent
{
	struct {
		short noteValue;
		char volume_pan;
		char type;
	};
	float floatValue;
};

struct OjnInternalEvent
{
	int Channel;
	int noteKind; // undefined if channel is not note or autoplay channel
	float Fraction;
	union {
		float fValue;
		int iValue;
	};
};

struct OjnMeasure
{
	float Len;

    std::vector<OjnInternalEvent> Events;

	OjnMeasure()
	{
		Len = 4;
	}
};

class OjnLoadInfo
{
public:
    std::vector<OjnMeasure> Measures;
	VSRG::Song* S;
	float BPM;
};

static double BeatForMeasure(OjnLoadInfo *Info, int Measure)
{
	double Out = 0;

	for (int i = 0; i < Measure; i++)
	{
		Out += Info->Measures[i].Len;
	}

	return Out;
}

// Based off the O2JAM method at 
// https://github.com/open2jamorg/open2jam/blob/master/parsers/src/org/open2jam/parsers/EventList.java
void FixOJNEvents(OjnLoadInfo *Info)
{
	auto CurrentMeasure = 0;
	OjnInternalEvent prevIter[7] = {-1, -1, -1, -1};

	for (auto Measure: Info->Measures)
	{
		// Sort events. This is very important, since we assume events are sorted!
        std::sort(Measure.Events.begin(), Measure.Events.end(),
			[&](const OjnInternalEvent A, const OjnInternalEvent B) -> bool 
			{ return A.Fraction < B.Fraction; });

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

void ProcessOJNEvents(OjnLoadInfo *Info, VSRG::Difficulty* Out)
{
	ptrdiff_t CurrentMeasure = 0;

	// First, we sort and clear up invalid events.
	FixOJNEvents(Info);

	// Then we need to have just as many measures going out as we've got in here.
	Out->Data->Measures.reserve(Info->Measures.size());


	// First of all, we need to process BPM changes and fractional measures.
	for (auto Measure: Info->Measures)
	{
		float MeasureBaseBeat = BeatForMeasure(Info, CurrentMeasure);

		Out->Data->Measures.push_back(VSRG::Measure());

		// All fractional measure events were already handled at read time.
		Out->Data->Measures[CurrentMeasure].Length = Measure.Len;

		for (auto Evt: Measure.Events)
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
		TimingSegment Seg(0, Info->BPM);
		Out->Timing.push_back(Seg);

		// Since events and measures are ordered already, there's no need to sort
		// timing data unless we insert new information.
		sort(Out->Timing.begin(), Out->Timing.end());
	}


	// Now, we can process notes and long notes.
	CurrentMeasure = 0;
	float PendingLNs[7] = {0};
	float PendingLNSound[7] = {0};

	for (auto Measure : Info->Measures)
	{
		float MeasureBaseBeat = BeatForMeasure(Info, CurrentMeasure);

		for (auto Evt : Measure.Events)
		{
			if (Evt.Channel == BPM_CHANNEL) continue;
			else
			{
				auto Beat = MeasureBaseBeat + Evt.Fraction * 4;
				float Time = TimeAtBeat(Out->Timing, 0, Beat);

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
					VSRG::NoteData Note;

					if (Evt.Channel >= 7) continue; // Who knows... A buffer overflow may be possible.

					Note.StartTime = Time;
					Note.Sound = Evt.iValue;

					switch (Evt.noteKind)
					{
					case 0:
						Out->TotalNotes++;
						Out->TotalObjects++;
						Out->TotalScoringObjects++;
						Out->Data->Measures[CurrentMeasure].Notes[Evt.Channel].push_back(Note);
						break;
					case 2:
						Out->TotalScoringObjects++;
						PendingLNs[Evt.Channel] = Time;
						PendingLNSound[Evt.Channel] = Evt.iValue;
						break;
					case 3:
						Out->TotalObjects++;
						Out->TotalHolds++;
						Out->TotalScoringObjects++;
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



bool IsValidOJN(std::fstream &filein, GString filename, OjnHeader *Head)
{
	if (!filein)
	{
		Log::Printf("NoteLoaderOJN: %s could not be opened\n", filename.c_str());
		return false;
	}

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

const char *LoadOJNCover(GString filename, size_t &read)
{
#if (!defined _WIN32)
	std::fstream filein(filename.c_str());
#else
	std::fstream filein(Utility::Widen(filename).c_str(), std::ios::binary | std::ios::in);
#endif
	OjnHeader Head;
	char* out;

	if (!IsValidOJN(filein, filename, &Head))
		return "";

	out = new char[Head.cover_size];

	filein.seekg(Head.cover_offset, std::ios::beg);
	filein.read(out, Head.cover_size);
	read = Head.cover_size;

	return out;
}

void NoteLoaderOJN::LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out)
{
#if (!defined _WIN32)
	std::fstream filein(filename.c_str());
#else
	std::fstream filein(Utility::Widen(filename).c_str(), std::ios::binary | std::ios::in);
#endif
	OjnHeader Head;

	if (!IsValidOJN(filein, filename, &Head))
		return;

	GString vArtist;
	GString vName;
	GString Noter;
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
		OjnLoadInfo Info;
		std::shared_ptr<VSRG::Difficulty> Diff (new VSRG::Difficulty());
		std::shared_ptr<VSRG::O2JamTimingInfo> TInfo (new VSRG::O2JamTimingInfo);
		std::shared_ptr<VSRG::DifficultyLoadInfo> LInfo (new VSRG::DifficultyLoadInfo);

		switch (i)
		{
		case 0:
			TInfo->Difficulty = VSRG::O2JamTimingInfo::O2_EX;
			break;
		case 1:
			TInfo->Difficulty = VSRG::O2JamTimingInfo::O2_NX;
			break;
		case 2:
			TInfo->Difficulty = VSRG::O2JamTimingInfo::O2_HX;
			break;
		}

		Diff->Data = LInfo;
		Diff->Level = Head.level[i];
		Diff->Data->TimingInfo = TInfo;
		Diff->Author = Noter;
		Diff->Data->StageFile = Directory(filename).Filename();

		Info.S = Out;
		filein.seekg(Head.note_offset[i]);

		// O2Jam files use Beat-Based notation.
		Diff->BPMType = VSRG::Difficulty::BT_BEAT;
		Diff->Duration = Head.time[i];
		Diff->Name = DifficultyNames[i];
		Diff->Channels = 7;
		Diff->Filename = filename;
		Diff->IsVirtual = true;
		Info.BPM = Head.bpm;

		Info.Measures.reserve(Head.measure_count[i]);
		for (auto k = 0; k <= Head.measure_count[i]; k++)
			Info.Measures.push_back(OjnMeasure());

		/* 
			The implications of this structure are interesting.
			Measures may be unordered; but events may not, if only there's one package per channel per measure.
		*/
		for (auto package = 0; package < Head.package_count[i]; package++)
		{
			OjnPackage PackageHeader;
			filein.read(reinterpret_cast<char*>(&PackageHeader), sizeof(OjnPackage));

			for (auto cevt = 0; cevt < PackageHeader.events; cevt++)
			{
				auto Fraction = float(cevt) / float(PackageHeader.events);
				OjnEvent Event;
				OjnInternalEvent IEvt;

				filein.read(reinterpret_cast<char*>(&Event), sizeof(OjnEvent));

				switch (PackageHeader.channel)
				{
				case 0: // Fractional measure
					Info.Measures[PackageHeader.measure].Len = 4 * Event.floatValue;
					break;
				case 1: // BPM change
					IEvt.fValue = Event.floatValue;
					IEvt.Channel = BPM_CHANNEL;
					IEvt.Fraction = Fraction;
					Info.Measures[PackageHeader.measure].Events.push_back(IEvt);
					break;
				case 2: // note events (enginechannel = PackageHeader.channel - 2)
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
					if (Event.noteValue == 0) goto next_event;
					IEvt.Fraction = Fraction;
					IEvt.Channel = PackageHeader.channel - 2;
					IEvt.iValue = Event.noteValue;
					IEvt.noteKind = Event.type;
					Info.Measures[PackageHeader.measure].Events.push_back(IEvt);
					break;
				default: // autoplay notes
					if (Event.noteValue == 0) goto next_event;
					IEvt.Channel = AUTOPLAY_CHANNEL;
					IEvt.Fraction = Fraction;
					IEvt.iValue = Event.noteValue;
					IEvt.noteKind = Event.type;
					Info.Measures[PackageHeader.measure].Events.push_back(IEvt);
					break;
				}
				next_event:;
			}
		}

		// Process Info... then push back difficulty.
		ProcessOJNEvents(&Info, Diff.get());
		Out->Difficulties.push_back(Diff);
	}
}
