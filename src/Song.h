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

	inline bool operator<(const U &rhs)
	{
		return Time < rhs;
	}

	inline bool operator>(const U &rhs)
	{
		return Time < rhs;
	}
	
	inline bool compareSegment (const U &lhs, const T &rhs)
    {
        return lhs < rhs.Time;
    }

	inline bool compareTime(const T &lhs, const U &rhs)
	{
		return lhs.Time < rhs;
	}

	operator U() const
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

typedef std::vector<TimingSegment> TimingData;

struct AutoplaySound : public TimeBased < AutoplaySound, double >
{
    int Sound;

    AutoplaySound() : TimeBased(0), Sound(0) {};
    AutoplaySound(double T, int V) : TimeBased(T), Sound(V) {}
};

struct AutoplayBMP : public TimeBased < AutoplayBMP, double >
{
    int BMP;
    AutoplayBMP() : TimeBased(0), BMP(0) {};
    AutoplayBMP(double T, int V) : TimeBased(T), BMP(V) {};
};

template<class T>
inline bool TimeSegmentCompare(const T &t, const double &v) {
	return t.Time < v;
}

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
    std::map<int, std::string> AudioFiles; // int := snd index, std::string := file
    std::map<int, std::map<int, SliceInfo>> Slices; // 1st int := wav index, 2nd int := snd index, Slice Info, where to cut for 2nd int for wav 1st int
};

namespace Game
{
    class Song
    {
    public:

        int ID;
        struct Difficulty
        {
			// todo: move to its own object
            TimingData Timing;

            double Offset;
            double Duration; // In warped time, not judgment time

            // Meta
            std::string Name;
            std::filesystem::path Filename;
            std::string Author;

            uint32_t TotalNotes;
            uint32_t TotalHolds;
            uint32_t TotalObjects;
            uint32_t TotalScoringObjects;

            int ID;

            Difficulty()
            {
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
        std::string SongName;

        /* Song Author */
        std::string SongAuthor;

        /* Directory where files are contained */
        std::filesystem::path SongDirectory;

        /* Relative Paths */
        std::filesystem::path SongFilename, BackgroundFilename;

        /* Song Audio for Preview*/
        std::filesystem::path SongPreviewSource;

        /* Time to start preview */
        float PreviewTime;

        // Song subtitles
        std::string Subtitle;

        // Song genre
        std::string Genre;

        Song() { ID = -1; PreviewTime = 0; };
        virtual ~Song() {};
    };
}

/* SongDC Timing functions. */

/* Song Timing */
inline double spb(double bpm) { return 60 / bpm; } // Return seconds per beat.
inline double bps(double bpm) { return bpm / 60; } // Return beats per second.

/* Assume these are sorted. */
// Return the vector's index Beat resides in.
int SectionIndex(const TimingData &Timing, double Beat);

/*
    Find the value of the interval function defined by Timing at the value Beat
*/
double SectionValue(const TimingData &Timing, double Beat);

/*
    Find the time at Beat given a constant function defined in intervals by Timing
*/
double TimeAtBeat(const TimingData &Timing, float Offset, double Beat, bool Abs = false);

/*
    Find the antiderivative of an interval defined function through the Timing variable
    from 0 to Time.
*/
double IntegrateToTime(const TimingData &Timing, double Time, float Drift = 0);

/*
    Find the sum of all stops that have happened in [0, Beat)
*/
double StopTimeAtBeat(const TimingData &StopsTiming, double Beat);

#define DifficultyDuration(MySong, Diff) \
	(Diff.Measures.size()) ? \
		TimeAtBeat(Diff.Timing, Diff.Offset, Diff.Measures.size() * MySong.MeasureLength); : \
	0

void GetTimingChangesInInterval(const TimingData &Timing,
    double PointA, double PointB,
    TimingData &Out);

void LoadTimingList(TimingData &Timing, std::string line, bool AllowZeros = false);

// Quantizes fraction to a beat's maximum resolution (1/48th of a beat)
double QuantizeFractionBeat(double Frac);

// Quantizes fraction to a measure's maximum resolution (1/192nd of a measure)
double QuantizeFractionMeasure(double Frac);

// Quantizes beat to a beat's maximum resolution (1/48th of a beat)
double QuantizeBeat(double Beat);