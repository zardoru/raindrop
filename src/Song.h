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
	double Time;
	int Sound;
};

enum ModeType
{
	MODE_DOTCUR,
	MODE_7K
};

namespace Game
{

	class Song
	{
	public:

		struct Difficulty {

			TimingData Timing;

			double Offset;
			double Duration;

			// Meta
			String Name;
			String Filename;

			uint32 TotalNotes;
			uint32 TotalHolds;
			uint32 TotalObjects;
			uint32 TotalScoringObjects;

			std::map<int, String> SoundList;

			int ID;

			Difficulty() {
				ID = 0;
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
		String SongName;

		/* Song Author */
		String SongAuthor;

		/* Directory where files are contained */
		String SongDirectory;

		/* Relative Paths */
		String SongFilename, BackgroundFilename;

		Song() {};
		virtual ~Song() {};
	};

}

#include "SongTiming.h"

#endif
