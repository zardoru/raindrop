#pragma once

#include <array>
#include "../TimingWindows.h"

namespace rd {
    class TimingWindowsOsuMania : public TimingWindows {
        std::array<float, MAX_CHANNELS> lane_hold_delta_time{};
        bool score_v2;
    public:

        explicit TimingWindowsOsuMania(bool _score_v2 = false);
        void reset() override;
        void default_setup() override;
        void setup(double strictness, double scale) override;
        ScoreKeeperJudgment get_judgment_for_time_offset(double time_delta, LaneHandle lane, NoteJudgmentPart part) override;
        double get_tick_interval() override;
        void add_judgment(ScoreKeeperJudgment skj, bool early_miss) override;
        bool uses_two_judges_per_hold() const override;
    };
}
