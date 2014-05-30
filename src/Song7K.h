#include "Song.h"
#include "TrackNote.h"

namespace VSRG
{
	const uint8 MAX_CHANNELS = 16;

	struct Measure 
	{
		float MeasureLength; // In beats. 4 by default.
		std::vector<NoteData> MeasureNotes[MAX_CHANNELS];

		Measure() {
			MeasureLength = 4;
		}
	};

	typedef std::vector<Measure> MeasureVector;
	
	typedef std::vector<std::vector<TrackNote> > MeasureVectorTN;
	

	struct Difficulty : public Game::Song::Difficulty
	{
		TimingData StopsTiming;

		// For in-game effects.
		TimingData BPS;

		// For speed changes, as obvious as it sounds.
		TimingData SpeedChanges;

		// Vertical speeds. Same role as BarlineRatios.
		TimingData VerticalSpeeds;

		// Notes (Up to MAX_CHANNELS tracks)
		MeasureVector Measures;

		unsigned char Channels;
		std::vector<AutoplaySound> BGMEvents;
		bool IsVirtual;

		double PreviewTime;
	public:
		Difficulty() { 
			IsVirtual = false;
			Channels = 0;
			PreviewTime = 0;
		};
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
		double Offset;
		TimingData BPMData;
		TimingData StopsData; 
		bool UseSeparateTimingData;

		enum 
		{
			BT_Beat,
			BT_MS,
			BT_Beatspace
		} BPMType;

		std::vector<VSRG::Difficulty*> Difficulties;

		Song();
		~Song();

		void Process(VSRG::Difficulty* Which, MeasureVectorTN *NotesOut, float Drift = 0, double SpeedConstant = 0);
	};

}