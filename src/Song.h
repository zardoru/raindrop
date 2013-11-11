#ifndef SONG_H_
#define SONG_H_

#include "GameObject.h"
#include "TrackNote.h"

float spb(float bpm);

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
		// Type
		struct TimingSegment
		{
			double Time; // in beats
			double Value; // in bpm
		};

		// Stores bpm at beat pairs
		std::vector<TimingSegment> Timing;

		// Stores the ratio barline should move at a certain time
		std::vector<TimingSegment> BarlineRatios;

		float Offset;
		float Duration;

		// Notes
		std::vector<Measure<T>> Measures;

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
	
	/* .dcf file filename*/
	String ChartFilename;

	/* path relative to  */
	String SongFilename, BackgroundDir, SongRelativePath, BackgroundRelativeDir;

	/* Song title */
	String SongName;
	
	/* Song Author */
	String SongAuthor;

	/* Directory where files are contained */
	String SongDirectory;

	double		LeadInTime;
	int			MeasureLength;
	std::vector<String> SoundList;
};

/* Dotcur Song */
class Song : public TSong < GameObject >
{
public:
	Song();
	~Song();
	void Process(bool CalculateXPos = true);
	void Repack();
	bool Save(const char* Filename);
};

class Song7K : public TSong < TrackNote >
{
public:
	Song7K();
};

/* Song Timing */
float spb(float bpm);
float bps(float bpm);

#include "SongTiming.h"

#endif