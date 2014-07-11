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

		double PreviewTime;

		int LMT; // Last modified time

		enum EBt
		{
			BT_Beat,
			BT_MS,
			BT_Beatspace
		} BPMType;

		// This information won't be saved into the cache file.
		unsigned char Channels;
		bool IsVirtual;

		// New variables for cache
		bool IsLoaded;
		bool ParseAgain;

	public:
		Difficulty() {
			ParseAgain = false;
			IsLoaded = false;
			IsVirtual = false;
			Channels = 0;
			PreviewTime = 0;
			LMT = 0;
		};

		// Load and save from cache
		bool SaveCache(String filename);
		bool LoadCache(String filename);

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

		/* For charting systems that use one declaration of timing for all difficulties only used at load time */
		String FilenameCache;

		TimingData BPMData;
		TimingData StopsData; 

		std::vector<VSRG::Difficulty*> Difficulties;
		double Offset;

		bool UseSeparateTimingData;

		Song();
		~Song();

		String DifficultyCacheFilename(VSRG::Difficulty * Diff);
		void Process(VSRG::Difficulty* Which, VectorTN NotesOut, float Drift = 0, double SpeedConstant = 0);
	};

}