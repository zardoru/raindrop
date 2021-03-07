#pragma once

#include <game/Timing.h>
#include <game/GameConstants.h>
#include <game/TrackNote.h>
#include <vector>
#include <filesystem>
#include <map>

typedef std::vector<TimingSegment> TimingData;

struct AutoplaySound : public TimedEvent < AutoplaySound, double >
{
    uint32_t Sound;

    AutoplaySound() : TimedEvent(0), Sound(0) {};
    AutoplaySound(double T, uint32_t V) : TimedEvent(T), Sound(V) {}
};

struct AutoplayBMP : public TimedEvent < AutoplayBMP, double >
{
    int BMP;
    AutoplayBMP() : TimedEvent(0), BMP(0) {};
    AutoplayBMP(double T, int V) : TimedEvent(T), BMP(V) {};
};

template<class T>
inline bool TimeSegmentCompare(const T &t, const double &v) {
	return t.Time < v;
}

struct SliceInfo
{
    double Start, End;
};

struct SliceContainer
{
    std::map<int, std::string> AudioFiles; // int := snd index, std::string := file
    std::map<int, std::map<int, SliceInfo>> Slices; // 1st int := wav index, 2nd int := snd index, Slice Info, where to cut for 2nd int for wav 1st int
};

namespace rd
{


    struct Measure
    {
        std::vector<NoteData> Notes[MAX_CHANNELS];
        double Length; // In beats. 4 by default.

        Measure()
        {
            Length = 4;
        }
    };

    struct SpeedSection : TimedEvent<SpeedSection, double>
    {
        double Duration;
        double Value;
        bool IntegrateByBeats; // if true, integrate by beats, if false, by time.

        SpeedSection() : TimedEvent() {
            Duration = 0;
            Value = 0;
            IntegrateByBeats = false;
        }
    };

    typedef std::vector<SpeedSection> VectorInterpolatedSpeedMultipliers;

    typedef std::vector<Measure> VectorMeasure;

    typedef std::vector<TrackNote> VectorTrackNote[MAX_CHANNELS];
    typedef std::vector<TrackNote*> VectorTrackNotePointer[MAX_CHANNELS];

    class ChartInfo
    {
    protected:
        ChartType Type;
    public:
        ChartInfo()
        {
            Type = TI_NONE;
        }

        virtual ~ChartInfo() {}

        ChartType GetType() const;
    };

    class BMSChartInfo : public ChartInfo
    {
    public:
        float JudgeRank;

        float GaugeTotal;

        // neccesary because of regular BMS DEFEXRANK
        bool PercentualJudgerank;

        // Whether this uses BMSON features.
        // (Also makes GaugeTotal a rate instead of an absolute)
        bool IsBMSON;

        BMSChartInfo()
        {
            Type = TI_BMS;
            JudgeRank = 3;
            GaugeTotal = -1;
            IsBMSON = false;
            PercentualJudgerank = false;
        }
    };

    class OsumaniaChartInfo : public ChartInfo
    {
    public:
        float HP, OD;
        OsumaniaChartInfo()
        {
            Type = TI_OSUMANIA;
            HP = 5;
            OD = 5;
        }
    };

    class O2JamChartInfo : public ChartInfo
    {
    public:
        enum
        {
            O2_EX,
            O2_NX,
            O2_HX
        } Difficulty;

        O2JamChartInfo()
        {
            Type = TI_O2JAM;
            Difficulty = O2_HX;
        }
    };

    class StepmaniaChartInfo : public ChartInfo
    {
    public:
        StepmaniaChartInfo()
        {
            Type = TI_STEPMANIA;
        }
    };

    struct BMPEventsDetail
    {
        std::map<int, std::string> BMPList;
        std::vector<AutoplayBMP> BMPEventsLayerBase;
        std::vector<AutoplayBMP> BMPEventsLayer;
        std::vector<AutoplayBMP> BMPEventsLayer2;
        std::vector<AutoplayBMP> BMPEventsLayerMiss;
    };



    struct DifficultyLoadInfo
    {
        // Contains stops data.
        TimingData Stops;

        // For scroll changes, as obvious as it sounds.
        TimingData Scrolls;

        // At Time, warp Value seconds forward.
        TimingData Warps;

        // Notes (Up to MAX_CHANNELS tracks)
        VectorMeasure Measures;

        // For Speed changes.
        VectorInterpolatedSpeedMultipliers InterpoloatedSpeedMultipliers;

        // Autoplay Sounds
        std::vector<AutoplaySound> BGMEvents;

        // Autoplay BMP
        std::shared_ptr<BMPEventsDetail> BMPEvents;

        // o!m storyboard stuff: saved as a string for later parsing
        std::string osbSprites;

        // Timing Info
        std::shared_ptr<ChartInfo> TimingInfo;

        // id/file sound list map;
        std::map<int, std::string> SoundList;

        // Background/foreground to show when loading.
        std::string StageFile;

        // Genre (Display only, for the most part)
        std::string Genre;

        // Whether this difficulty uses the scratch channel (being channel/index 0 always used for this)
        bool Turntable;

        // Identification purposes
        std::string FileHash;
        int IndexInFile;

        // Audio slicing data
        SliceContainer SliceData;

        uint32_t GetObjectCount();
        uint32_t GetScoreItemsCount();

        DifficultyLoadInfo()
        {
            Turntable = false;
            IndexInFile = -1;
        }
    };

    struct Difficulty
    {
        // todo: move to its own object
        TimingData Timing;

        double Offset;
        double Duration; // In warped time, not judgment time

        // Metadata
        std::string Name;
        std::filesystem::path Filename;
        std::string Author;

        // Not-removed metadata
        int ID;

        // VSRG
        std::unique_ptr<DifficultyLoadInfo> Data;

        enum ETimingType
        {
            BT_BEAT, // beat based timing
            BT_MS, // millisecond based timing
            BT_BEATSPACE // osu! ms/beat timing
        } BPMType;

        long long Level;
        unsigned char Channels;
        bool IsVirtual;

        Difficulty()
        {
            ID = -1;
            Duration = 0;
            Offset = 0;
            IsVirtual = false;
            Channels = 0;
            Level = 0;
            Data = nullptr;
        }

        void Destroy(); // remove non-metadata
    };

    class Song
    {
    public:

        int ID;
        std::vector<std::shared_ptr<Difficulty>> Difficulties;

        /* Song title */
        std::string Title;

        /* Song Author */
        std::string Artist;

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

        // returns pointer owned by Song class, so don't delete.
        Difficulty* GetDifficulty(uint32_t i);

        uint8_t GetDifficultyCount();

        Song() { ID = -1; PreviewTime = 0; };
        virtual ~Song() {};
    };
}



/* Song Timing */
inline double spb(double bpm) { return 60 / bpm; } // Return seconds per beat.
inline double bpm_to_bps(double bpm) { return bpm / 60; } // Return beats per second.

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
double TimeAtBeat(const TimingData &Timing, double Offset, double Beat, bool Abs = false);

/*
    Find the antiderivative of an interval defined function through the Timing variable
    from 0 to Time.
*/
double IntegrateToTime(const TimingData &Timing, double Time);

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