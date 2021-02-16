#pragma once

namespace rd {
    class Song;
}

class Replay;

class Profile {
    void AssureProfilePathExistence();
public:
    std::string Name;
    ScoreDatabase Scores;

    std::filesystem::path GetPath();

    // based off names
    bool Load(std::string Name);
    bool Save();

    void SaveReplay(
        const rd::Song* song,
        const Replay &replay
    );

    // ScoreRow GetDifficultyScore(rd::Difficulty* diff);

    static std::vector<std::string> GetProfileList();
};
