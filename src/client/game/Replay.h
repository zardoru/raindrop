#pragma once

#include <functional>
#include <queue>
#include <game/GameConstants.h>

class Replay {
public:

    // 16 bytes per entry
    struct Entry {
        double time;
        rd::LaneHandle lane;
        uint32_t down;
    };

    typedef std::function<void(Entry)> OnReplayEvent;

private:

    std::vector<Entry> replay_data_;
    std::queue<Entry> event_playback_queue_;

    std::vector<OnReplayEvent> playback_listeners_;

    std::string song_hash_;
    uint32_t diff_index_{};

    PlayscreenParameters user_parameters_;
    rd::ESpeedType speed_type_;

public:
    Replay();

    ~Replay();

    // Use the params after player processing
    // not the requested params, but effective ones
    void set_chart_data(
            PlayscreenParameters params, // params we're going ingame with
            rd::ESpeedType speed_type, // target speed type
            std::string sha256hash = "", // file hash, for locating within database
            uint32_t diffindex = 0 // difficulty index in the defined chart hash
    );

    PlayscreenParameters get_effective_parameters() const;

    std::string get_song_hash() const;

    uint32_t get_difficulty_index() const;

    bool is_loaded();


    /*
     Add these events _without_ the offset alterations
     of audio drift and user note displacement
     but, in unwarped time, and including judgetime
    */
    void add_event(Entry entry);

    bool load(std::filesystem::path input);

    bool save(std::filesystem::path input) const;

    // similar to auto, use judgetime - offset
    void update(double Time);

    /*
    function recieves time at which to pretend the judgement ocurred
    in unwarped time, without offset
    as well as lane.
    */
    void add_playback_listener(OnReplayEvent fn);
};
