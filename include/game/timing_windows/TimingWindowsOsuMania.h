#pragma once

#include <array>
#include "../TimingWindows.h"

namespace rd {
    class TimingWindowsOsuMania : public TimingWindows {
        std::array<float, MAX_CHANNELS> lane_hold_delta_time{};
        bool score_v2;
    public:

        explicit TimingWindowsOsuMania(bool _score_v2 = false);
        void Reset() override;
        void DefaultSetup() override;
        void Setup(double strictness, double scale) override;
        ScoreKeeperJudgment GetJudgmentForTimeOffset(double time_delta, uint32_t lane, NoteJudgmentPart part) override;
        double GetTickInterval() override;
        void AddJudgment(ScoreKeeperJudgment skj) override;
        bool UsesTwoJudgesPerHold() const override;
    };
}