#include "Song.h"
#include "TrackNote.h"

namespace VSRG
{
	const uint8 MAX_CHANNELS = 16;

	struct Measure
	{
		std::vector<NoteData> MeasureNotes[MAX_CHANNELS];
		float MeasureLength; // In beats. 4 by default.

		Measure() {
			MeasureLength = 4;
		}
	};

	typedef std::vector<Measure> MeasureVector;

	typedef std::vector<TrackNote> VectorTN[MAX_CHANNELS];

	enum TimingInfoType {
		TI_NONE,
		TI_BMS,
		TI_OSUMANIA
	};

	class CustomTimingInfo {
	protected:
		TimingInfoType Type;
	public:
		CustomTimingInfo()
		{
			Type = TI_NONE;
		}

		virtual ~CustomTimingInfo() {}

		TimingInfoType GetType();
	};

	class BmsTimingInfo : public CustomTimingInfo
	{
	public:
		int judge_rank;
		float life_total;

		BmsTimingInfo()
		{
			Type = TI_BMS;
			judge_rank = 3;
			life_total = -1;
		}
	};

	class OsuManiaTimingInfo : public CustomTimingInfo
	{
	public:
		float HP, OD;
		OsuManiaTimingInfo()
		{
			Type = TI_OSUMANIA;
			HP = 5;
			OD = 5;
		}
	};

	struct Difficulty : public Game::Song::Difficulty
	{
		// This is saved to the cache file.
		TimingData StopsTiming;

		// For in-game effects.
		TimingData BPS;

		// For speed changes, as obvious as it sounds.
		TimingData SpeedChanges;

		// Vertical speeds. Same role as BarlineRatios.
		TimingData VerticalSpeeds;

		// Notes (Up to MAX_CHANNELS tracks)
		MeasureVector Measures;

		// Autoplay Sounds
		std::vector<AutoplaySound> BGMEvents;

		// Autoplay BMP
		std::map<int, String> BMPList;
		std::vector<AutoplayBMP> BMPEvents;
		std::vector<AutoplayBMP> BMPEventsMiss;
		std::vector<AutoplayBMP> BMPEventsLayer;
		std::vector<AutoplayBMP> BMPEventsLayer2;

		double PreviewTime;

		int LMT; // Last modified time

		enum EBt
		{
			BT_Beat,
			BT_MS,
			BT_Beatspace
		} BPMType;

		unsigned char Channels;
		bool IsVirtual;


	public:
	
		CustomTimingInfo* TimingInfo;

		Difficulty() {
			IsVirtual = false;
			Channels = 0;
			PreviewTime = 0;
			LMT = 0;
			TimingInfo = NULL;
		};

		// The floats are in vertical units; like the notes' vertical position.
		void GetMeasureLines(std::vector<float> &Out, float Drift);

		// Destroy all information that can be loaded from cache
		void Destroy();
	};


	/* 7K Song */
	class Song : public Game::Song
	{
		double PreviousDrift;
		bool Processed;

		void ProcessBPS(VSRG::Difficulty* Diff, double Drift);
		void ProcessVSpeeds(VSRG::Difficulty* Diff, double SpeedConstant);
		void ProcessSpeedVariations(VSRG::Difficulty* Diff, double Drift);
	public:

		std::vector<VSRG::Difficulty*> Difficulties;

		Song();
		~Song();

		void Process(VSRG::Difficulty* Which, VectorTN NotesOut, float Drift = 0, double SpeedConstant = 0);		
	};

}
