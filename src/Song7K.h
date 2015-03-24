#include "Song.h"
#include "TrackNote.h"

namespace VSRG
{
	struct Measure
	{
		vector<NoteData> MeasureNotes[MAX_CHANNELS];
		double MeasureLength; // In beats. 4 by default.

		Measure() {
			MeasureLength = 4;
		}
	};

	typedef std::vector<Measure> MeasureVector;

	typedef std::vector<TrackNote> VectorTN[MAX_CHANNELS];

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
		std::map<int, GString> BMPList;
		std::vector<AutoplayBMP> BMPEventsLayerBase;
		std::vector<AutoplayBMP> BMPEventsLayer;
		std::vector<AutoplayBMP> BMPEventsLayer2;
		std::vector<AutoplayBMP> BMPEventsLayerMiss;
	};

	struct DifficultyLoadInfo
	{
		// Contains stops data.
		TimingData StopsTiming;

		// For speed changes, as obvious as it sounds.
		TimingData SpeedChanges;

		// Notes (Up to MAX_CHANNELS tracks)
		MeasureVector Measures;

		// Autoplay Sounds
		std::vector<AutoplaySound> BGMEvents;

		// Autoplay BMP
		BMPEventsDetail *BMPEvents;

		// Timing Info
		std::shared_ptr<CustomTimingInfo> TimingInfo;

		// Background/foreground to show when loading.
		GString StageFile;

		DifficultyLoadInfo()
		{
			BMPEvents = nullptr;
			TimingInfo = nullptr;
		}
	};

	struct Difficulty : public Game::Song::Difficulty
	{
		std::shared_ptr<DifficultyLoadInfo> Data;

		enum EBt
		{
			BT_Beat,
			BT_MS,
			BT_Beatspace
		} BPMType;

		int Level;
		unsigned char Channels;
		bool IsVirtual;

		void ProcessVSpeeds(TimingData& BPS, TimingData& VSpeeds, double SpeedConstant);
		void ProcessSpeedVariations(TimingData& BPS, TimingData& VSpeeds, double Drift);
	public:

		// Get processed data for use on ScreenGameplay7K.
		void Process(VectorTN NotesOut, TimingData &BPS, TimingData& VerticalSpeeds, float Drift = 0, double SpeedConstant = 0);
		void ProcessBPS(TimingData& BPS, double Drift);

		// The floats are in vertical units; like the notes' vertical position.
		void GetMeasureLines(std::vector<float> &Out, TimingData& VerticalSpeeds, double WaitTime = 0);

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

	protected:
		function <double(double)> QuantizeFunction;
		int GetRowCount(const vector<Event> &In);

		void CalculateMeasureAccomulation();
		IFraction FractionForMeasure(int Measure, double Beat);

		int MeasureForBeat(double Beat);
		void ResizeMeasures(size_t NewMaxIndex);

		void CalculateBGMEvents();
		void CalculateObjects();
		
		vector<double> MeasureAccomulation;
		vector<Measure> Measures;
		TimingData BPS;

		Difficulty *Parent;

		RowifiedDifficulty(Difficulty *Source, bool Quantize, bool CalculateAll);

		friend class Song;
	public:
		double GetStartingBPM();
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
