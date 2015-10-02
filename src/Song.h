#ifndef SONG_H_
#define SONG_H_

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

struct TimingSegment : public TimeBased<TimingSegment, double>
{
	double Value; // in bpm
	TimingSegment(double T, double V) : TimeBased(T), Value(V) {};
	TimingSegment() : TimingSegment(0, 0) {};
};

inline bool operator<(double Beat, const TimingSegment &in) {
	return Beat < in.Time;
}

typedef std::vector<TimingSegment> TimingData;

struct AutoplaySound : public TimeBased<AutoplaySound, float>
{
	int Sound;
};

struct AutoplayBMP : public TimeBased<AutoplayBMP, float>
{
	int BMP;
};

enum ModeType
{
	MODE_DOTCUR,
	MODE_VSRG
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

			map<int, GString> SoundList;

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

#endif
