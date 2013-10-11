#ifndef SONG_H_
#define SONG_H_

#include "GameObject.h"

float spb(float bpm);

namespace SongInternal
{
	class Measure
	{
	public:
		uint32 Fraction;
		std::vector<GameObject> MeasureNotes;
	};

	class Difficulty
	{
	public:
		// Type
		struct TimingSegment
		{
			float Time; // in beats
			float Value; // in bpm
		};
		std::vector<TimingSegment> Timing;
		float Offset;

		// Notes
		std::vector<Measure> Measures;

		// Meta
		std::string Name;
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
	std::string SongFilename, BackgroundDir, SongRelativePath, BackgroundRelativeDir;
	std::string SongName;
	std::string SongDirectory;
	double		LeadInTime;
	void Process(bool CalculateXPos = true);
	void Repack();
	void Save(const char* Filename);
};

#endif