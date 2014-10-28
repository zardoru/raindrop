#include <fstream>
#include <map>

#include "Global.h"
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
		char unk;
		char type;
	};
	float floatValue;
};

struct OjnInternalEvent
{
	int Channel;
	int noteKind; // undefined if channel is not note channel
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

bool OrderOJNEventsPredicate (OjnInternalEvent& A, OjnInternalEvent& B)
{
	return A.Fraction < B.Fraction;
}

void ProcessOJNEvents(OjnLoadInfo *Info, VSRG::Difficulty* Out)
{
	int CurrentMeasure = 0;
	Out->Measures.reserve(Info->Measures.size());


	// First of all, we need to process BPM changes and fractional measures.
	for (std::vector<OjnMeasure>::iterator Measure = Info->Measures.begin(); 
		Measure != Info->Measures.end(); 
		Measure++)
	{
		float MeasureBaseBeat = BeatForMeasure(Info, CurrentMeasure);

		std::sort(Measure->Events.begin(), Measure->Events.end(), OrderOJNEventsPredicate);
		Out->Measures.push_back(VSRG::Measure());

		Out->Measures[CurrentMeasure].MeasureLength = Measure->Len;

		for (std::vector<OjnInternalEvent>::iterator Evt = Measure->Events.begin();
			Evt != Measure->Events.end();
			Evt++)
		{
			if (Evt->Channel != BPM_CHANNEL) continue;
			float Beat = MeasureBaseBeat + Evt->Fraction * 4;
			TimingSegment Segment;

			if (Evt->fValue == 0) continue;

			Segment.Time = Beat;
			Segment.Value = Evt->fValue;

			if (Out->Timing.size())
			{
				if (Out->Timing.back().Value == Evt->fValue) // ... We already have this BPM.
					continue;
			}

			Out->Timing.push_back(Segment);
		}

		CurrentMeasure++;
	}

	// Now, we can process notes and long notes.
	CurrentMeasure = 0;
	float PendingLNs[7];
	float PendingLNSound[7];
	for (std::vector<OjnMeasure>::iterator Measure = Info->Measures.begin();
		Measure != Info->Measures.end();
		Measure++)
	{
		float MeasureBaseBeat = BeatForMeasure(Info, CurrentMeasure);

		for (std::vector<OjnInternalEvent>::iterator Evt = Measure->Events.begin();
			Evt != Measure->Events.end();
			Evt++)
		{
			if (Evt->Channel == BPM_CHANNEL) continue;
			else if (Evt->Channel == AUTOPLAY_CHANNEL)
			{
				float Beat = MeasureBaseBeat + Evt->Fraction * Measure->Len;
				float Time = TimeAtBeat(Out->Timing, 0, Beat);
				AutoplaySound Snd;

				Snd.Sound = Evt->iValue;
				Snd.Time = Time;
				Out->BGMEvents.push_back(Snd);
			}
			else
			{
				float Beat = MeasureBaseBeat + Evt->Fraction * Measure->Len;
				float Time = TimeAtBeat(Out->Timing, 0, Beat);
				VSRG::NoteData Note;

				Note.StartTime = Time;
				Note.Sound = Evt->iValue;
				switch (Evt->noteKind)
				{
				case 0:
					Out->TotalNotes++;
					Out->TotalObjects++;
					Out->TotalScoringObjects++;
					Out->Measures[CurrentMeasure].MeasureNotes[Evt->Channel].push_back(Note);
					break;
				case 2:
					Out->TotalScoringObjects++;
					PendingLNs[Evt->Channel] = Time;
					PendingLNSound[Evt->Channel] = Evt->iValue;
					break;
				case 3:
					Out->TotalObjects++;
					Out->TotalHolds++;
					Out->TotalScoringObjects++;
					Note.StartTime = PendingLNs[Evt->Channel];
					Note.EndTime = Time;
					Note.Sound = PendingLNSound[Evt->Channel];
					Out->Measures[CurrentMeasure].MeasureNotes[Evt->Channel].push_back(Note);
					break;
				}
			}
		}
		CurrentMeasure++;
	}
}

void NoteLoaderOJN::LoadObjectsFromFile(GString filename, GString prefix, VSRG::Song *Out)
{
#if (!defined _WIN32)
	std::ifstream filein(filename.c_str());
#else
	std::ifstream filein(Utility::Widen(filename).c_str(), std::ios::binary);
#endif

	OjnHeader Head;
	char hData[sizeof(OjnHeader)];
	size_t qz = sizeof(OjnHeader);
	filein.read(hData, qz);
	if (!filein)
	{
		if (filein.eofbit)
			Log::Printf("NoteLoaderOJN: EOF reached before header could be read\n");
		return;
	}

	memcpy(&Head, hData, sizeof(OjnHeader));

	if (strcmp(Head.signature, "ojn"))
	{
		Log::Printf("NoteLoaderOJN: %s is not an ojn file.\n");
		return;
	}

	Out->SongAuthor = Head.artist;
	Out->SongName = Head.title;
	Out->SongFilename = Head.ojm_file;
	for (int i = 0; i < 3; i++)
	{
		OjnLoadInfo Info;
		VSRG::Difficulty *Diff = new VSRG::Difficulty();
		Info.S = Out;
		filein.seekg(Head.note_offset[i]);

		// O2Jam files use Beat-Based notation.
		Diff->BPMType = VSRG::Difficulty::BT_Beat;
		Diff->LMT = Utility::GetLMT(filename);
		Diff->Duration = Head.time[i];
		Diff->Name = DifficultyNames[i];
		Diff->Channels = 7;
		Diff->Filename = filename;

		Info.Measures.reserve(Head.measure_count[i]);
		for (int k = 0; k <= Head.measure_count[i]; k++)
			Info.Measures.push_back(OjnMeasure());

		/* 
			The implications of this structure are interesting.
			Measures may be unordered; but events may not, if only there's one package per channel per measure.
		*/
		for (int package = 0; package < Head.package_count[i]; package++)
		{
			OjnPackage PackageHeader;
			filein.read((char*)&PackageHeader, sizeof(OjnPackage));

			for (int cevt = 0; cevt < PackageHeader.events; cevt++)
			{
				float Fraction = (float)cevt / (float)PackageHeader.events;
				OjnEvent Event;
				OjnInternalEvent IEvt;

				filein.read((char*)&Event, sizeof(OjnEvent));

				switch (PackageHeader.channel)
				{
				case 0: // Fractional measure
					Info.Measures[PackageHeader.measure].Len *= Event.floatValue;
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
					Info.Measures[PackageHeader.measure].Events.push_back(IEvt);
					break;
				}
				next_event:;
			}
		}

		// Process Info... then push back difficulty.
		ProcessOJNEvents(&Info, Diff);
		Out->Difficulties.push_back(Diff);
	}
}