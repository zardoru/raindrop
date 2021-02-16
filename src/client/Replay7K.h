#pragma once

#include <functional>

class Replay {
public:

    // 16 bytes per entry
    struct Entry {
        double Time;
        uint32_t Lane;
        uint32_t Down;
    };

    typedef std::function<void(Entry)> OnReplayEvent;

private:

    std::vector<Entry> ReplayData;
    std::queue<Entry> EventPlaybackQueue;

    std::vector<OnReplayEvent> PlaybackListeners;

    std::string SongHash;
    uint32_t DiffIndex{};

    PlayscreenParameters UserParameters;
    rd::ESpeedType SpeedType;

public:
    Replay();

    ~Replay();

    // Use the params after player processing
    // not the requested params, but effective ones
    void SetSongData(
            PlayscreenParameters params, // params we're going ingame with
            rd::ESpeedType SpeedType, // target speed type
            std::string sha256hash = "", // file hash, for locating within database
            uint32_t diffindex = 0 // difficulty index in the defined chart hash
    );

    PlayscreenParameters GetEffectiveParameters() const;

    std::string GetSongHash() const;

    uint32_t GetDifficultyIndex() const;

    bool IsLoaded();


    /*
     Add these events _without_ the offset alterations
     of audio drift and user note displacement
     but, in unwarped time, and including judgetime
    */
    void AddEvent(Entry entry);

    bool Load(std::filesystem::path input);

    bool Save(std::filesystem::path input) const;

    // similar to auto, use judgetime - offset
    void Update(double Time);

    /*
    function recieves time at which to pretend the judgement ocurred
    in unwarped time, without offset
    as well as lane.
    */
    void AddPlaybackListener(OnReplayEvent fn);
};
