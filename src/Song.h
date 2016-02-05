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

inline bool operator<(double Beat, const TimingSegment &in)
{
    return Beat < in.Time;
}

inline bool operator<(const TimingSegment &A, const TimingSegment &B) {
	return A.Time < B.Time;
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
            TimingData Timing;

            double Offset;
            double Duration;

            // Meta
            std::string Name;
            std::filesystem::path Filename;
            std::string Author;

            uint32_t TotalNotes;
            uint32_t TotalHolds;
            uint32_t TotalObjects;
            uint32_t TotalScoringObjects;

            std::map<int, std::string> SoundList;

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
        std::string SongFilename, BackgroundFilename;

        /* Song Audio for Preview*/
        std::string SongPreviewSource;

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

#include "SongTiming.h"
