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
			float Time; // in beats
			float Value; // in bpm
		};
		std::vector<TimingSegment> Timing;
		float Offset;

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
	String SongName;
	String SongDirectory;
	String SongAuthor;
	double		LeadInTime;
	int			MeasureLength;
	std::vector<String> SoundList;
	void Process(bool CalculateXPos = true);
	void Repack();
	void Save(const char* Filename);
};

#endif