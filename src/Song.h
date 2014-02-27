#ifndef SONG_H_
#define SONG_H_

#include "GameObject.h"
#include "TrackNote.h"

#define MAX_CHANNELS 16

#define SPEEDTYPE_FIRST 0
#define SPEEDTYPE_MMOD 1
#define SPEEDTYPE_CMOD 2

namespace SongInternal
{
	struct TimingSegment
	{
		double Time; // in beats
		double Value; // in bpm
	};
}

typedef std::vector<SongInternal::TimingSegment> TimingData;

namespace SongInternal
{
	class MeasureDC
	{
	public:
		MeasureDC()
		{
			Fraction = 0;
		}
		uint32 Fraction;
		std::vector<GameObject> MeasureNotes;
	};

	class Measure7K
	{
	public:
		Measure7K()
		{
			Fraction = 0;
		}
		uint32 Fraction;
		std::vector<TrackNote> MeasureNotes;
	};


	struct DifficultyDC
	{
		// Stores bpm at beat pairs
		TimingData Timing;

		float Offset;
		float Duration;

		// Notes
		std::vector<MeasureDC> Measures;

		// Meta
		String Name;

		// Stores the ratio barline should move at a certain time
		TimingData BarlineRatios;

		uint32 TotalNotes;
		uint32 TotalHolds;
		uint32 TotalObjects;
		uint32 TotalScoringObjects;
		std::vector<String> SoundList;
	};

	struct Difficulty7K
	{
		// Stores bpm at beat pairs
		TimingData Timing;
		TimingData StopsTiming;

		// For in-game effects.
		TimingData BPS;

		// For speed changes, as obvious as it sounds.
		TimingData SpeedChanges;

		// Vertical speeds. Same role as BarlineRatios.
		TimingData VerticalSpeeds;

		float Offset;
		float Duration;

		// Notes (Up to MAX_CHANNELS tracks)
		std::vector<Measure7K> Measures[MAX_CHANNELS];

		// Meta
		String Name;

		uint32 TotalNotes;
		uint32 TotalHolds;
		uint32 TotalObjects;
		uint32 TotalScoringObjects;

		// 7k
		unsigned char Channels;
		std::map<int, String> SoundList;
	};
}


template
<class T>
class TSong 
{
public:
	TSong() {};
	~TSong() {};
};

/* Dotcur Song */
class SongDC : public TSong < GameObject >
{
public:
	SongDC();
	~SongDC();

	std::vector<SongInternal::DifficultyDC*> Difficulties;
	
	/* chart filename*/
	String ChartFilename;

	/* path relative to  */
	String SongFilename, BackgroundDir, SongRelativePath, BackgroundRelativeDir;

	/* Song title */
	String SongName;
	
	/* Song Author */
	String SongAuthor;

	/* Directory where files are contained */
	String SongDirectory;

	double		LeadInTime; // default to 1.5 for 7K
	int			MeasureLength;
	void Process(bool CalculateXPos = true);
	void Repack();
	bool Save(const char* Filename);
};

/* 7K Song */
class Song7K : public TSong < TrackNote >
{
	bool Processed;
	double PreviousDrift;

	void ProcessBPS(SongInternal::Difficulty7K* Diff, double Drift);
	void ProcessVSpeeds(SongInternal::Difficulty7K* Diff, double SpeedConstant);
	void ProcessSpeedVariations(SongInternal::Difficulty7K* Diff, double Drift);
public:

	/* For osu!mania chart loading */
	double SliderVelocity;

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

	std::vector<SongInternal::Difficulty7K*> Difficulties;
	
	/* chart filename*/
	String ChartFilename;

	/* path relative to  */
	String SongFilename, BackgroundDir, SongRelativePath, BackgroundRelativeDir;

	/* Song title */
	String SongName;
	
	/* Song Author */
	String SongAuthor;

	/* Directory where files are contained */
	String SongDirectory;

	double		LeadInTime; // default to 1.5 for 7K
	int			MeasureLength;
	Song7K();
	~Song7K();

	void Process(float Drift = 0, double SpeedConstant = 0);
};

/* Song Timing */
float spb(float bpm);
float bps(float bpm);

#include "SongTiming.h"

#endif
