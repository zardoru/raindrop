#ifndef SONG_H_
#define SONG_H_

#include "GameObject.h"

float spb(float bpm);

namespace SongInternal
{
	class Measure
	{
	public:
		Measure();
		uint32 Fraction;
		std::vector<GameObject> MeasureNotes;
	};

	struct Difficulty
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
		std::vector<Measure> Measures;

		// Meta
		String Name;
	};
}

// converts bpm into seconds per beat
float spb(float bpm);
uint32 SectionIndex(SongInternal::Difficulty *Diff, float Beat);

class Song 
{
public:
	Song();
	~Song();
	std::vector<SongInternal::Difficulty*> Difficulties;
	String SongFilename, BackgroundDir, SongRelativePath, BackgroundRelativeDir;
	String ChartFilename;
	String SongName;
	String SongDirectory;
	String SongAuthor;
	double		LeadInTime;
	int			MeasureLength;
	std::vector<String> SoundList;
	void Process(bool CalculateXPos = true);
	void Repack();
	bool Save(const char* Filename);
};

/* Song Timing */
double TimeAtBeat(SongInternal::Difficulty &Diff, float Beat);
double DifficultyDuration(Song &MySong, SongInternal::Difficulty &Diff);
void CalculateBarlineRatios(Song &MySong, SongInternal::Difficulty &Diff);

#endif