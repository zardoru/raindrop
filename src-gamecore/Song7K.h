#pragma once

namespace Game {
	namespace VSRG {
		struct Measure
		{
			std::vector<NoteData> Notes[MAX_CHANNELS];
			double Length; // In beats. 4 by default.

			Measure()
			{
				Length = 4;
			}
		};

		struct SpeedSection : TimeBased<SpeedSection, double>
		{
			double Duration;
			double Value;
			bool IntegrateByBeats; // if true, integrate by beats, if false, by time.

			SpeedSection() {
				Duration = 0;
				Value = 0;
				IntegrateByBeats = false;
			}
		};

		typedef std::vector<SpeedSection> VectorInterpolatedSpeedMultipliers;

		typedef std::vector<Measure> VectorMeasure;

		typedef std::vector<TrackNote> VectorTN[MAX_CHANNELS];
		typedef std::vector<TrackNote*> VectorTNP[MAX_CHANNELS];

		class ChartInfo
		{
		protected:
			ChartType Type;
		public:
			ChartInfo()
			{
				Type = TI_NONE;
			}

			virtual ~ChartInfo() {}

			ChartType GetType() const;
		};

		class BMSChartInfo : public ChartInfo
		{
		public:
			float JudgeRank;
			
			float GaugeTotal;

			// neccesary because of regular BMS DEFEXRANK
			bool PercentualJudgerank;

			// Whether this uses BMSON features.
			// (Also makes GaugeTotal a rate instead of an absolute)
			bool IsBMSON;

			BMSChartInfo()
			{
				Type = TI_BMS;
				JudgeRank = 3;
				GaugeTotal = -1;
				IsBMSON = false;
				PercentualJudgerank = false;
			}
		};

		class OsumaniaChartInfo : public ChartInfo
		{
		public:
			float HP, OD;
			OsumaniaChartInfo()
			{
				Type = TI_OSUMANIA;
				HP = 5;
				OD = 5;
			}
		};

		class O2JamChartInfo : public ChartInfo
		{
		public:
			enum
			{
				O2_EX,
				O2_NX,
				O2_HX
			} Difficulty;

			O2JamChartInfo()
			{
				Type = TI_O2JAM;
				Difficulty = O2_HX;
			}
		};

		class StepmaniaChartInfo : public ChartInfo
		{
		public:
			StepmaniaChartInfo()
			{
				Type = TI_STEPMANIA;
			}
		};

		struct BMPEventsDetail
		{
			std::map<int, std::string> BMPList;
			std::vector<AutoplayBMP> BMPEventsLayerBase;
			std::vector<AutoplayBMP> BMPEventsLayer;
			std::vector<AutoplayBMP> BMPEventsLayer2;
			std::vector<AutoplayBMP> BMPEventsLayerMiss;
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
			VectorInterpolatedSpeedMultipliers InterpoloatedSpeedMultipliers;

			// Autoplay Sounds
			std::vector<AutoplaySound> BGMEvents;

			// Autoplay BMP
			std::shared_ptr<BMPEventsDetail> BMPEvents;

			// o!m storyboard stuff
			// std::shared_ptr<osb::SpriteList> osbSprites;

			// Timing Info
			std::shared_ptr<ChartInfo> TimingInfo;

			// id/file sound list map;
            std::map<int, std::string> SoundList;

			// Background/foreground to show when loading.
			std::string StageFile;

			// Genre (Display only, for the most part)
			std::string Genre;

			// Whether this difficulty uses the scratch channel (being channel/index 0 always used for this)
			bool Turntable;

			// Identification purposes
			std::string FileHash;
			int IndexInFile;

			// Audio slicing data
			SliceContainer SliceData;

			uint32_t GetObjectCount();
			uint32_t GetScoreItemsCount();

			DifficultyLoadInfo()
			{
				Turntable = false;
				IndexInFile = -1;
			}
		};

		struct Difficulty : Game::Song::Difficulty
		{
			std::unique_ptr<DifficultyLoadInfo> Data;

			enum ETimingType
			{
				BT_BEAT, // beat based timing
				BT_MS, // millisecond based timing
				BT_BEATSPACE // osu! ms/beat timing
			} BPMType;

			long long Level;
			unsigned char Channels;
			bool IsVirtual;
		public:

			// Destroy all information that can be loaded from cache
			void Destroy();

			Difficulty()
			{
				IsVirtual = false;
				Channels = 0;
				Level = 0;
				Data = nullptr;
			};

			~Difficulty()
			{
				Destroy();
			};
		};

		class RowifiedDifficulty
		{
		public:
			struct Event
			{
				IFraction Sect;
				uint32_t Evt;
			};

			struct Measure
			{
				std::vector<Event> Objects[MAX_CHANNELS];
				std::vector<Event> LNObjects[MAX_CHANNELS];
				std::vector<Event> BGMEvents;
			};

		private:
			bool Quantizing;
			std::vector<double> MeasureAccomulation;

		protected:
			std::function <double(double)> QuantizeFunction;
			int GetRowCount(const std::vector<Event> &In);

			void CalculateMeasureAccomulation();
			IFraction FractionForMeasure(int Measure, double Beat);

			int MeasureForBeat(double Beat);
			void ResizeMeasures(size_t NewMaxIndex);

			void CalculateBGMEvents();
			void CalculateObjects();

			std::vector<Measure> Measures;
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
			std::vector<std::shared_ptr<VSRG::Difficulty> > Difficulties;

			Song();
			~Song();

			VSRG::Difficulty* GetDifficulty(uint32_t i);
			uint8_t GetDifficultyCount() override;
		};

	}
}