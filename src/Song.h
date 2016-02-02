#pragma once

template <class T, class U>
struct TimeBased
{
	U Time;

	inline bool operator< (const T &rhs)
	{
		return Time < rhs.Time;
	}

	inline bool operator>(const T &rhs)
	{
		return Time > rhs.Time;
	}

	inline operator U()
	{
		return Time;
	}

	TimeBased(U val) : Time(val) {};
	TimeBased() = default;
};

struct TimingSegment : public TimeBased < TimingSegment, double >
{
	double Value; // in bpm
	TimingSegment(double T, double V) : TimeBased(T), Value(V) {};
	TimingSegment() : TimingSegment(0, 0) {};
};

inline bool operator<(double Beat, const TimingSegment &in) {
	return Beat < in.Time;
}

typedef std::vector<TimingSegment> TimingData;

struct AutoplaySound : public TimeBased < AutoplaySound, float >
{
	int Sound;

	AutoplaySound() : TimeBased(0), Sound(0) {};
	AutoplaySound(float T, int V) : TimeBased(T), Sound(V) {}
};

struct AutoplayBMP : public TimeBased < AutoplayBMP, float >
{
	int BMP;
	AutoplayBMP() : TimeBased(0), BMP(0) {};
	AutoplayBMP(float T, int V) : TimeBased(T), BMP(V) {};
};

enum ModeType
{
	MODE_DOTCUR,
	MODE_VSRG
};

struct SliceInfo
{
	double Start, End;
};

struct SliceContainer
{
    std::map<int, GString> AudioFiles; // int := snd index, GString := file 
    std::map<int, std::map<int, SliceInfo>> Slices; // 1st int := wav index, 2nd int := snd index, Slice Info, where to cut for 2nd int for wav 1st int
};

namespace Game
{

	class Song
	{
	public:

		int ID;
		struct Difficulty {

			TimingData Timing;

			double Offset;
			double Duration;

			// Meta
			GString Name;
			GString Filename;
			GString Author;

			uint32 TotalNotes;
			uint32 TotalHolds;
			uint32 TotalObjects;
			uint32 TotalScoringObjects;

            std::map<int, GString> SoundList;

			int ID;

			Difficulty() {
				ID = -1;
				Duration = 0;
				Offset = 0;
				TotalNotes = 0;
				TotalHolds = 0;
				TotalObjects = 0;
				TotalScoringObjects = 0;
			}
		};

		ModeType Mode;

		/* Song title */
		GString SongName;

		/* Song Author */
		GString SongAuthor;

		/* Directory where files are contained */
		Directory SongDirectory;

		/* Relative Paths */
		GString SongFilename, BackgroundFilename;

		/* Song Audio for Preview*/
		GString SongPreviewSource;

		/* Time to start preview */
		float PreviewTime;

		// Song subtitles
		GString Subtitle;

		// Song genre
		GString Genre;

		Song() { ID = -1; PreviewTime = 0; };
		virtual ~Song() {};
	};

}

#include "SongTiming.h"
