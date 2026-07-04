#pragma once

namespace otoworm {
    class ChartGroup;
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
        const otoworm::ChartGroup* chart_group,
        const Replay &replay
    );

    // ScoreRow GetDifficultyScore(rd::Difficulty* diff);

    static std::vector<std::string> GetProfileList();
};
