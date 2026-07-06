#pragma once

namespace otoworm {
    class ChartGroup;
}

class Replay;

class Profile {
    void AssureProfilePathExistence();
public:
    std::string name;
    ScoreDatabase scores;

    std::filesystem::path get_path();

    // based off names
    bool load(std::string Name);
    bool save();

    void save_replay(
        const otoworm::ChartGroup* chart_group,
        const Replay &replay
    );

    // ScoreRow GetDifficultyScore(rd::Difficulty* diff);

    static std::vector<std::string> get_profile_list();
};
