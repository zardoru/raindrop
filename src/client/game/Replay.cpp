
#include <game/GameConstants.h>
#include <queue>
#include <fstream>
#include <filesystem>

#include <json.hpp>
#include "PlayscreenParameters.h"
#include "../serialize/PlayscreenParameters.h"

#include "Replay.h"


using json = nlohmann::json;

Replay::Replay() {

}

Replay::~Replay() {

}

void Replay::set_chart_data(
        PlayscreenParameters params,
        rd::ESpeedType speedType,
        std::string sha256hash,
        uint32_t diffindex) {
    user_parameters_ = params;
    song_hash_ = sha256hash;
    diff_index_ = diffindex;
    speed_type_ = speedType;
}

PlayscreenParameters Replay::get_effective_parameters() const {
    return user_parameters_;
}

std::string Replay::get_song_hash() const {
    return song_hash_;
}

uint32_t Replay::get_difficulty_index() const {
    return diff_index_;
}

bool Replay::is_loaded() {
    return !event_playback_queue_.empty();
}

void Replay::add_event(Entry entry) {
    replay_data_.push_back(entry);
}

bool Replay::load(std::filesystem::path input) {
    std::ifstream in(input, std::ios::in | std::ios::binary);

    json root;
    root = json::from_cbor(in);

    std::vector<Entry> events;

    // copy potentially unsorted events
    for (auto jsonentry: root["replayEvents"]) {
        events.push_back(Entry{
                jsonentry["t"],
                jsonentry["l"],
                jsonentry["d"]
        });
    }

    // put all events ordered on the queue. now we're sure it's sorted
    std::sort(events.begin(), events.end(), [](const Entry &A, const Entry &B) {
        return A.time < B.time;
    });

    for (auto evt: events) {
        event_playback_queue_.push(evt);
    }

    song_hash_ = root["song"]["hash"];
    diff_index_ = root["song"]["index"];
    deserialize(user_parameters_, root["userParameters"]);

    return true;
}

bool Replay::save(std::filesystem::path outputpath) const {
    json root = {
        {"song",
            {
                {"hash", song_hash_},
                {"index", diff_index_}
            }
        },
        {
         "userParameters", serialize(user_parameters_)
        }
    };

    for (auto entry : replay_data_) {
        json jsonentry = {
            {"t", entry.time},
            {"l", static_cast<uint32_t>(entry.lane)},
            {"d", entry.down != 0}
        };

        root["replayEvents"].push_back(jsonentry);
    }


    std::ofstream out(outputpath, std::ios::out | std::ios::binary);
    auto buf = json::to_cbor(root);
    out.write((const char *) buf.data(), buf.size());

    return true;
}

void Replay::update(double Time) {
    while (!event_playback_queue_.empty() &&
           event_playback_queue_.front().time <= Time) {
        auto evt = event_playback_queue_.front();

        // push to all listeners
        for (auto &listener : playback_listeners_)
            listener(evt);

        event_playback_queue_.pop();
    }
}

void Replay::add_playback_listener(OnReplayEvent fn) {
    playback_listeners_.push_back(fn);
}

