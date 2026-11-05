#pragma once

namespace otoworm {
    class ChartGroup;
}

class Replay;

class Profile {
    void ensure_profile_path_existence() const;
public:
    std::string name;
    ScoreDatabase scores;

    std::filesystem::path get_path() const;

    // based off names
    bool load(std::string Name);
    bool save() const;

    void save_replay(
        const otoworm::ChartGroup* chart_group,
        const Replay &replay
    ) const;


    static std::vector<std::string> get_profile_list();
};
