#ifndef SONG_H_
#define SONG_H_

struct TimingSegment
{
	double Time; // in beats
	double Value; // in bpm
};

typedef std::vector<TimingSegment> TimingData;

struct AutoplaySound
{
	float Time;
	int Sound;
};

struct AutoplayBMP
{
	float Time;
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
		GString SongDirectory;

		/* Relative Paths */
		GString SongFilename, BackgroundFilename;

		/* When != 0 will look for this file and try and load it */
		GString SongFileSource;

		Song() { ID = -1;  };
		virtual ~Song() {};
	};

}

#include "SongTiming.h"

#endif
