
#include <game/GameConstants.h>
#include <queue>
#include <fstream>
#include <filesystem>

#include <json.hpp>
#include "PlayscreenParameters.h"
#include "../serialize/PlayscreenParameters.h"

#include "Replay7K.h"


using json = nlohmann::json;

Replay::Replay() {

}

Replay::~Replay() {

}

void Replay::SetSongData(
        PlayscreenParameters params,
        rd::ESpeedType speedType,
        std::string sha256hash,
        uint32_t diffindex) {
    UserParameters = params;
    SongHash = sha256hash;
    DiffIndex = diffindex;
    SpeedType = speedType;
}

PlayscreenParameters Replay::GetEffectiveParameters() const {
    return UserParameters;
}

std::string Replay::GetSongHash() const {
    return SongHash;
}

uint32_t Replay::GetDifficultyIndex() const {
    return DiffIndex;
}

bool Replay::IsLoaded() {
    return !EventPlaybackQueue.empty();
}

void Replay::AddEvent(Entry entry) {
    ReplayData.push_back(entry);
}

bool Replay::Load(std::filesystem::path input) {
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
        return A.Time < B.Time;
    });

    for (auto evt: events) {
        EventPlaybackQueue.push(evt);
    }

    SongHash = root["song"]["hash"];
    DiffIndex = root["song"]["index"];
    deserialize(UserParameters, root["userParameters"]);

    return true;
}

bool Replay::Save(std::filesystem::path outputpath) const {
    json root = {
        {"song",
            {
                {"hash", SongHash},
                {"index", DiffIndex}
            }
        },
        {
         "userParameters", serialize(UserParameters)
        }
    };

    for (auto entry : ReplayData) {
        json jsonentry = {
            {"t", entry.Time},
            {"l", entry.Lane},
            {"d", entry.Down != 0}
        };

        root["replayEvents"].push_back(jsonentry);
    }


    std::ofstream out(outputpath, std::ios::out | std::ios::binary);
    auto buf = json::to_cbor(root);
    out.write((const char *) buf.data(), buf.size());

    return true;
}

void Replay::Update(double Time) {
    while (!EventPlaybackQueue.empty() &&
           EventPlaybackQueue.front().Time <= Time) {
        auto evt = EventPlaybackQueue.front();

        // push to all listeners
        for (auto &listener : PlaybackListeners)
            listener(evt);

        EventPlaybackQueue.pop();
    }
}

void Replay::AddPlaybackListener(OnReplayEvent fn) {
    PlaybackListeners.push_back(fn);
}

