#include "Song.h"
#include "TrackNote.h"

namespace VSRG
{
	struct Measure
	{
		vector<NoteData> Notes[MAX_CHANNELS];
		double Length; // In beats. 4 by default.

		Measure() {
			Length = 4;
		}
	};

	struct SpeedSection : TimeBased<SpeedSection, float>
	{
		float Duration;
		float Value;
	};

	typedef vector<SpeedSection> VectorSpeeds;

	typedef vector<Measure> VectorMeasure;

	typedef vector<TrackNote> VectorTN[MAX_CHANNELS];

	enum TimingInfoType {
		TI_NONE,
		TI_BMS,
		TI_OSUMANIA,
		TI_O2JAM,
		TI_STEPMANIA
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

	class BMSTimingInfo : public CustomTimingInfo
	{
	public:
		int JudgeRank;
		float GaugeTotal;

		// Whether this uses BMSON features.
		bool IsBMSON;

		BMSTimingInfo()
		{
			Type = TI_BMS;
			JudgeRank = 3;
			GaugeTotal = -1;
			IsBMSON = false;
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

	class O2JamTimingInfo : public CustomTimingInfo
	{
	public:
		enum {
			O2_EX,
			O2_NX,
			O2_HX
		} Difficulty;

		O2JamTimingInfo()
		{
			Type = TI_O2JAM;
			Difficulty = O2_HX;
		}
	};

	class StepmaniaTimingInfo : public CustomTimingInfo
	{
	public:
		StepmaniaTimingInfo()
		{
			Type = TI_STEPMANIA;
		}
	};

	struct BMPEventsDetail {
		map<int, GString> BMPList;
		vector<AutoplayBMP> BMPEventsLayerBase;
		vector<AutoplayBMP> BMPEventsLayer;
		vector<AutoplayBMP> BMPEventsLayer2;
		vector<AutoplayBMP> BMPEventsLayerMiss;
	};

	struct DifficultyLoadInfo
	{
		// Contains stops data.
		TimingData Stops;

		// For scroll changes, as obvious as it sounds.
		TimingData Scrolls;

		// At Time, warp Value seconds forward.
		TimingData Warps;

		// Notes (Up to MAX_CHANNELS tracks)
		VectorMeasure Measures;

		// For Speed changes.
		VectorSpeeds Speeds;

		// Autoplay Sounds
		vector<AutoplaySound> BGMEvents;

		// Autoplay BMP
		shared_ptr<BMPEventsDetail> BMPEvents;

		// Timing Info
		shared_ptr<CustomTimingInfo> TimingInfo;

		// Background/foreground to show when loading.
		GString StageFile;

		// Whether this difficulty uses the scratch channel (being channel/index 0 always used for this)
		bool Turntable;

		// Audio slicing data
		SliceContainer SliceData;

		DifficultyLoadInfo()
		{
			Turntable = false;
		}
	};

	struct Difficulty : Game::Song::Difficulty
	{
		shared_ptr<DifficultyLoadInfo> Data;

		enum ETimingType
		{
			BT_BEAT, // beat based timing
			BT_MS, // millisecond based timing
			BT_BEATSPACE // osu! ms/beat timing
		} BPMType;

		int Level;
		unsigned char Channels;
		bool IsVirtual;

		void ProcessVSpeeds(TimingData& BPS, TimingData& VSpeeds, double SpeedConstant);
		void ProcessSpeedVariations(TimingData& BPS, TimingData& VSpeeds, double Drift);
		double GetWarpAmountAtTime(double Time);
	public:

		bool IsWarpingAt(double start_time);
		// Get processed data for use on ScreenGameplay7K.
		void GetPlayableData(VectorTN NotesOut, 
							TimingData& BPS, 
							TimingData& VerticalSpeeds,
							TimingData& Warps,
							float Drift = 0, double SpeedConstant = 0);
		void ProcessBPS(TimingData& BPS, double Drift);

		// The floats are in vertical units; like the notes' vertical position.
		void GetMeasureLines(vector<float> &Out, TimingData& VerticalSpeeds, double WaitTime, double Drift);

		// Destroy all information that can be loaded from cache
		void Destroy();

		Difficulty() {
			IsVirtual = false;
			Channels = 0;
			Level = 0;
			Data = nullptr;
		};

		~Difficulty() {
			Destroy();
		};
	};


	class RowifiedDifficulty {
	public:
		struct Event {
			IFraction Sect;
			int Evt;
		};

		struct Measure {
			vector<Event> Objects[VSRG::MAX_CHANNELS];
			vector<Event> LNObjects[VSRG::MAX_CHANNELS];
			vector<Event> BGMEvents;
		};

	private:
		bool Quantizing;
		vector<double> MeasureAccomulation;

	protected:
		function <double(double)> QuantizeFunction;
		int GetRowCount(const vector<Event> &In);

		void CalculateMeasureAccomulation();
		IFraction FractionForMeasure(int Measure, double Beat);

		int MeasureForBeat(double Beat);
		void ResizeMeasures(size_t NewMaxIndex);

		void CalculateBGMEvents();
		void CalculateObjects();
		
		vector<Measure> Measures;
		TimingData BPS;

		Difficulty *Parent;

		RowifiedDifficulty(Difficulty *Source, bool Quantize, bool CalculateAll);

		friend class Song;
	public:
		double GetStartingBPM();
		bool IsQuantized();
	};

	/* 7K Song */
	class Song : public Game::Song
	{
	public:
		vector<shared_ptr<VSRG::Difficulty> > Difficulties;

		Song();
		~Song();

		VSRG::Difficulty* GetDifficulty(uint32 i);
	};

}
