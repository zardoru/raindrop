#ifndef SONG_H_
#define SONG_H_

#include "GameObject.h"
#include "TrackNote.h"

#define MAX_CHANNELS 16

namespace SongInternal
{
	template <class T>
	class Measure
	{
	public:
		Measure()
		{
			Fraction = 0;
		}
		uint32 Fraction;
		std::vector<T> MeasureNotes;
	};

	template <class T>
	struct TDifficulty
	{
		int Dummy;
	};

	template <>
	struct TDifficulty <GameObject>
	{
		// Type
		struct TimingSegment
		{
			double Time; // in beats
			double Value; // in bpm
		};

		// Stores bpm at beat pairs
		std::vector<TimingSegment> Timing;

		float Offset;
		float Duration;

		// Notes
		std::vector<Measure<GameObject>> Measures;

		// Meta
		String Name;

		// Stores the ratio barline should move at a certain time
		std::vector<TimingSegment> BarlineRatios;
	};

	template <>
	struct TDifficulty <TrackNote>
	{
		// Type
		struct TimingSegment
		{
			double Time; // in beats
			double Value; // in bpm
		};

		// Stores bpm at beat pairs
		std::vector<TimingSegment> Timing;

		// Vertical speeds. Same role as BarlineRatios.
		std::vector<TimingSegment> VerticalSpeeds;

		float Offset;
		float Duration;

		// Notes (Up to MAX_CHANNELS tracks)
		std::vector<Measure<TrackNote>> Measures[MAX_CHANNELS];

		// Meta
		String Name;

		// 7k
		unsigned char Channels;
	};
}

template
<class T>
class TSong 
{
public:
	TSong() {};
	~TSong() {};
	std::vector<SongInternal::TDifficulty<T>*> Difficulties;
	
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
	std::vector<String> SoundList;
};

/* Dotcur Song */
class SongDC : public TSong < GameObject >
{
public:
	SongDC();
	~SongDC();
	void Process(bool CalculateXPos = true);
	void Repack();
	bool Save(const char* Filename);
};

/* 7K Song */
class Song7K : public TSong < TrackNote >
{
public:
	Song7K();
	~Song7K();
	void Process();
};

/* Song Timing */
float spb(float bpm);
float bps(float bpm);

#include "SongTiming.h"

#endif